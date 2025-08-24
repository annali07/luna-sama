import numpy as np
import torch
import torchaudio
import librosa

from .dsp import get_spepc, resample, mel_fn_v3, mel_fn_v4, norm_spec, denorm_spec
from .textproc import TextFrontend, dict_language
from .loaders import get_gpt_weights, get_sovits_weights, load_bert, load_ssl

HZ = 50  # semantic frame rate used in early-stop calc

# Optional SV (only for v2Pro / v2ProPlus)
try:
    from .sv import SV as _SV
    _HAS_SV = True
    print("SV: ", _HAS_SV, flush=True)
    
except Exception:
    _HAS_SV = False

class TTSService:
    """
    One object that loads everything once and synthesizes repeatedly.
    """

    def __init__(self, device: str, is_half: bool,
                 hubert_dir: str, bert_dir: str,
                 gpt_ckpt: str, sovits_ckpt: str, sv=None, require_sv=False):
        self.device = device
        self.is_half = is_half

        # models
        self.tokenizer, self.bert = load_bert(bert_dir, device, is_half)
        self.ssl = load_ssl(hubert_dir, device, is_half)
        self.gpt = get_gpt_weights(gpt_ckpt, device, is_half)
        self.sovits = get_sovits_weights(sovits_ckpt, device, is_half)
        self.textfe = TextFrontend(self.tokenizer, self.bert, device, is_half)

        # v2Pro/Plus SV encoder if needed
        self.sv = _SV(device, is_half) if (_HAS_SV and self.sovits.version in {"v2Pro","v2ProPlus"}) else None

    # --- internals ---
    @torch.no_grad()
    def _encode_prompt_semantics(self, ref_wav_path: str):
        wav16k, _ = librosa.load(ref_wav_path, sr=16000)
        w = torch.from_numpy(wav16k).to(self.device)
        if self.is_half: w = w.half()
        ssl_content = self.ssl.model(w.unsqueeze(0))["last_hidden_state"].transpose(1, 2)
        codes = self.sovits.vq_model.extract_latent(ssl_content)
        return codes[0, 0].unsqueeze(0).to(self.device)   # [1, T, D]? -> used by infer_panel

    @staticmethod
    def _ensure_sentence_final_punc(text: str, lang_key: str) -> str:
        if not text: return text
        if text[-1] in {"，","。","？","！",",",".","?","!","~",":","：","—","…"}:
            return text
        return text + ("。" if lang_key != "en" else ".")

    # --- public API ---
    @torch.no_grad()
    def synth(self,
              ref_wav_path: str,
              prompt_text: str,
              prompt_lang: str,
              text: str,
              text_lang: str,
              *,
              top_k=15, top_p=0.6, temperature=0.6,
              speed=1.0,
              sample_steps=32,
              if_sr=False,
              extra_ref_wavs: list[str] | None = None) -> tuple[int, np.ndarray]:
        """
        Synthesize once.
        Returns (sample_rate, mono float32 waveform in [-1,1]).
        """

        # normalize language labels to internal keys
        if prompt_lang not in dict_language: prompt_lang = prompt_lang.lower()
        if text_lang   not in dict_language: text_lang   = text_lang.lower()
        prompt_lang = dict_language.get(prompt_lang, "zh")
        text_lang   = dict_language.get(text_lang, "zh")

        # guard punctuation
        prompt_text = self._ensure_sentence_final_punc(prompt_text.strip(), prompt_lang)
        text        = self._ensure_sentence_final_punc(text.strip(), text_lang)

        # prompt semantic
        prompt_sem = self._encode_prompt_semantics(ref_wav_path)

        # phones & bert
        version = self.sovits.version
        phones1, bert1, _ = self.textfe.get_phones_and_bert(prompt_text, prompt_lang, version)
        phones2, bert2, _ = self.textfe.get_phones_and_bert(text,        text_lang,   version)
        bert = torch.cat([bert1, bert2], dim=1).unsqueeze(0).to(self.device)
        all_phoneme_ids = torch.LongTensor(phones1 + phones2).unsqueeze(0).to(self.device)
        all_phoneme_len = torch.tensor([all_phoneme_ids.shape[-1]]).to(self.device)

        # GPT inference
        pred_sem, idx = self.gpt.t2s_model.model.infer_panel(
            all_phoneme_ids, all_phoneme_len, prompt_sem, bert,
            top_k=top_k, top_p=top_p, temperature=temperature,
            early_stop_num=HZ * self.gpt.max_sec,
        )
        pred_sem = pred_sem[:, -idx:].unsqueeze(0)

        v = version
        # ------- v1/v2/Pro/ProPlus path (SoVITS decode) -------
        if v not in {"v3", "v4"}:
            dtype = torch.float16 if self.is_half else torch.float32
            refers = []
            sv_emb = None
            is_v2pro = v in {"v2Pro", "v2ProPlus"}

            if extra_ref_wavs:
                for p in extra_ref_wavs:
                    refer, audio_tensor = get_spepc(self.sovits.hps, p, dtype, self.device, is_v2pro)
                    refers.append(refer)
                if is_v2pro:
                    if not self.sv:
                        raise RuntimeError("v2Pro/Plus requires 'sv' module/weights.")
                    sv_emb = [self.sv.compute_embedding3(audio_tensor)]

            if len(refers) == 0:
                refer, audio_tensor = get_spepc(self.sovits.hps, ref_wav_path, dtype, self.device, is_v2pro)
                refers = [refer]
                if is_v2pro:
                    if not self.sv:
                        raise RuntimeError("v2Pro/Plus requires 'sv' module/weights.")
                    sv_emb = [self.sv.compute_embedding3(audio_tensor)]

            x = self.sovits.vq_model.decode(
                pred_sem,
                torch.LongTensor(phones2).unsqueeze(0).to(self.device),
                refers,
                speed=speed,
                sv_emb=sv_emb if is_v2pro else None,
            ).detach().cpu().numpy()[0, 0]
            sr = 32000
            # clamp
            m = np.abs(x).max()
            if m > 1: x = x / m
            return sr, x.astype("float32")

        # ------- v3/v4 path (CFM + vocoder) -------
        # Build phoneme ids
        pid0 = torch.LongTensor(phones1).unsqueeze(0).to(self.device)
        pid1 = torch.LongTensor(phones2).unsqueeze(0).to(self.device)

        # ref enc
        refer, _ = get_spepc(self.sovits.hps, ref_wav_path,
                             torch.float16 if self.is_half else torch.float32,
                             self.device, False)
        fea_ref, ge = self.sovits.vq_model.decode_encp(prompt_sem.unsqueeze(0), pid0, refer)

        # prepare mel from reference audio at target sr
        ref_audio, sr_in = torchaudio.load(ref_wav_path)
        ref_audio = ref_audio.to(self.device).float()
        if ref_audio.shape[0] == 2:
            ref_audio = ref_audio.mean(0, keepdim=True)
        tgt_sr = 24000 if v == "v3" else 32000
        if sr_in != tgt_sr:
            ref_audio = resample(ref_audio, sr_in, tgt_sr, self.device)
        mel2 = mel_fn_v3(ref_audio) if v == "v3" else mel_fn_v4(ref_audio)
        mel2 = norm_spec(mel2)

        # align ref features
        T_min = min(mel2.shape[2], fea_ref.shape[2])
        mel2 = mel2[:, :, :T_min]
        fea_ref = fea_ref[:, :, :T_min]
        # chunking constants
        Tref   = 468 if v == "v3" else 500
        Tchunk = 934 if v == "v3" else 1000
        if T_min > Tref:
            mel2 = mel2[:, :, -Tref:]
            fea_ref = fea_ref[:, :, -Tref:]
            T_min = Tref
        chunk_len = Tchunk - T_min
        mel2 = mel2.to(torch.float16 if self.is_half else torch.float32)

        fea_todo, ge = self.sovits.vq_model.decode_encp(pred_sem, pid1, refer, ge, speed)
        csegs = []
        idx0 = 0
        while True:
            chunk = fea_todo[:, :, idx0: idx0 + chunk_len]
            if chunk.shape[-1] == 0: break
            idx0 += chunk_len
            fea = torch.cat([fea_ref, chunk], dim=2).transpose(2, 1)
            cfm_res = self.sovits.vq_model.cfm.inference(
                fea, torch.LongTensor([fea.size(1)]).to(fea.device), mel2, sample_steps, inference_cfg_rate=0
            )
            cfm_res = cfm_res[:, :, mel2.shape[2]:]   # strip ref part
            mel2 = cfm_res[:, :, -T_min:]
            fea_ref = chunk[:, :, -T_min:]
            csegs.append(cfm_res)
        cfm_res = torch.cat(csegs, dim=2)
        cfm_res = denorm_spec(cfm_res)

        voc = self.sovits.vocoder
        if voc is None:
            raise RuntimeError("Missing vocoder for v3/v4 path.")
        with torch.inference_mode():
            wav = voc(cfm_res)[0][0].cpu().detach().numpy()

        # sample rate
        sr = 24000 if v == "v3" else 48000
        m = np.abs(wav).max()
        if m > 1: wav = wav / m
        return sr, wav.astype("float32")
