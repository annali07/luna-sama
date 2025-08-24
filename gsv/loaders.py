import os
import torch
from dataclasses import dataclass
from transformers import AutoModelForMaskedLM, AutoTokenizer
from .feature_extractor import cnhubert
from .module.models import Generator, SynthesizerTrn, SynthesizerTrnV3
from .AR.models.t2s_lightning_module import Text2SemanticLightningModule
from .process_ckpt import get_sovits_version_from_path_fast, load_sovits_new

import sys
from . import utils as gsv_utils
sys.modules.setdefault("utils", gsv_utils) 
from . import text as text
sys.modules.setdefault("text", text) 

# optional lora for v3
try:
    from peft import LoraConfig, get_peft_model
    _HAS_PEFT = True
except Exception:
    _HAS_PEFT = False

@dataclass
class GptWeights:
    max_sec: int
    t2s_model: torch.nn.Module

@dataclass
class SovitsWeights:
    vq_model: torch.nn.Module
    hps: any        # DictToAttrRecursive-like
    version: str
    vocoder: torch.nn.Module | None  # BigVGAN for v3, HiFiGAN for v4 (None for v1/v2/Pro)

# ---------------- BERT / SSL ----------------
def load_bert(bert_dir: str, device: str, is_half: bool):
    tok = AutoTokenizer.from_pretrained(bert_dir)
    mdl = AutoModelForMaskedLM.from_pretrained(bert_dir)
    if is_half: mdl = mdl.half()
    mdl = mdl.to(device)
    return tok, mdl

def load_ssl(hubert_dir: str, device: str, is_half: bool):
    cnhubert.cnhubert_base_path = hubert_dir
    ssl = cnhubert.get_model()
    if is_half: ssl = ssl.half()
    return ssl.to(device)

# ---------------- GPT ----------------
def get_gpt_weights(gpt_path: str, device: str, is_half: bool) -> GptWeights:
    s1 = torch.load(gpt_path, map_location="cpu", weights_only=False)
    config = s1["config"]; max_sec = config["data"]["max_sec"]
    t2s = Text2SemanticLightningModule(config, "****", is_train=False)
    t2s.load_state_dict(s1["weight"])
    if is_half: t2s = t2s.half()
    t2s = t2s.to(device).eval()
    return GptWeights(max_sec=max_sec, t2s_model=t2s)

# ---------------- vocoders for v3/v4 ----------------
def _init_bigvgan(device: str, is_half: bool):
    from BigVGAN import bigvgan
    root = "GPT_SoVITS/pretrained_models/models--nvidia--bigvgan_v2_24khz_100band_256x"
    mdl = bigvgan.BigVGAN.from_pretrained(root, use_cuda_kernel=False)
    mdl.remove_weight_norm(); mdl.eval()
    if is_half: mdl = mdl.half()
    return mdl.to(device)

def _init_hifigan(device: str, is_half: bool):
    g = Generator(
        initial_channel=100, resblock="1",
        resblock_kernel_sizes=[3,7,11],
        resblock_dilation_sizes=[[1,3,5],[1,3,5],[1,3,5]],
        upsample_rates=[10,6,2,2,2],
        upsample_initial_channel=512,
        upsample_kernel_sizes=[20,12,4,4,4],
        gin_channels=0, is_bias=True
    )
    g.eval(); g.remove_weight_norm()
    sd = torch.load("GPT_SoVITS/pretrained_models/gsv-v4-pretrained/vocoder.pth",
                    map_location="cpu", weights_only=False)
    g.load_state_dict(sd)
    if is_half: g = g.half()
    return g.to(device)

# ---------------- SoVITS ----------------
def get_sovits_weights(sovits_path: str, device: str, is_half: bool) -> SovitsWeights:
    version_tag, model_version, if_lora_v3 = get_sovits_version_from_path_fast(sovits_path)

    d = load_sovits_new(sovits_path)
    hps = d["config"]

    # Dict -> attribute access
    class D2A(dict):
        def __init__(self, x):
            super().__init__(x)
            for k, v in x.items():
                if isinstance(v, dict): v = D2A(v)
                self[k] = v; setattr(self, k, v)
    hps = D2A(hps)
    hps.model.semantic_frame_rate = "25hz"

    # figure internal version (v1/v2 vs v3/v4)
    if "enc_p.text_embedding.weight" not in d["weight"]:
        hps.model.version = "v2"
    elif d["weight"]["enc_p.text_embedding.weight"].shape[0] == 322:
        hps.model.version = "v1"
    else:
        hps.model.version = "v2"

    model_params = vars(hps.model)

    vocoder = None
    if model_version not in {"v3", "v4"}:
        # v1/v2/Pro/ProPlus
        if "Pro" in model_version:
            hps.model.version = model_version
        vq = SynthesizerTrn(
            hps.data.filter_length // 2 + 1,
            hps.train.segment_size // hps.data.hop_length,
            n_speakers=hps.data.n_speakers,
            **model_params
        )
    else:
        hps.model.version = model_version
        vq = SynthesizerTrnV3(
            hps.data.filter_length // 2 + 1,
            hps.train.segment_size // hps.data.hop_length,
            n_speakers=hps.data.n_speakers,
            **model_params
        )
        vocoder = _init_bigvgan(device, is_half) if model_version == "v3" else _init_hifigan(device, is_half)

    # move, eval, and load weights (+ optional LoRA for v3)
    if "pretrained" not in sovits_path:
        try: del vq.enc_q
        except Exception: pass

    if is_half: vq = vq.half()
    vq = vq.to(device).eval()

    if not if_lora_v3:
        vq.load_state_dict(d["weight"], strict=False)
    else:
        if not _HAS_PEFT:
            raise RuntimeError("LoRA v3 weights given but 'peft' package is not installed.")
        # base path is inferred by process_ckpt; load base v3/v4 then apply lora
        # (your original code loads base from pretrained_sovits_name; here we assume
        # the LoRA checkpoint already contains full state dict entries for target modules)
        lora_rank = d["lora_rank"]
        cfg = LoraConfig(target_modules=["to_k","to_q","to_v","to_out.0"],
                         r=lora_rank, lora_alpha=lora_rank, init_lora_weights=True)
        vq.cfm = get_peft_model(vq.cfm, cfg)
        vq.load_state_dict(d["weight"], strict=False)
        vq.cfm = vq.cfm.merge_and_unload()
        vq.eval()

    return SovitsWeights(vq_model=vq, hps=hps, version=hps.model.version, vocoder=vocoder)


# --- OPTIONAL: Speaker-embedding (SV) loader for v2Pro/Plus ---
def load_sv(device, is_half):
    """
    Try to load the SV (speaker embedding) module/weights.
    Returns (sv, err): 'sv' is an object exposing .compute_embedding3(audio_tensor),
    or None if unavailable. 'err' is a short string you can log.
    """
    try:
        from sv import SV  # uses the project's sv.py & its internal weight path
    except Exception as e:
        return None, f"sv module import failed: {e}"

    try:
        sv = SV(device, is_half)
        return sv, None
    except FileNotFoundError as e:
        # Weights missing at the hard-coded path inside sv.py
        return None, f"SV weights missing: {e}"
    except Exception as e:
        return None, f"SV load error: {e}"
