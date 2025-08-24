import argparse
import soundfile as sf
from .config_infer import Config
from .service import TTSService

def main():
    g = Config()
    ap = argparse.ArgumentParser("gsv-infer-cli")
    ap.add_argument("-s","--sovits_path", default=g.sovits_path, help="SoVITS .pth")
    ap.add_argument("-g","--gpt_path",    default=g.gpt_path,    help="GPT .ckpt")
    ap.add_argument("-hb","--hubert_path",default=g.cnhubert_path)
    ap.add_argument("-b","--bert_path",   default=g.bert_path)
    ap.add_argument("-d","--device",      default=g.infer_device, help="cuda|cpu")
    ap.add_argument("--fp","--full_precision", dest="full_precision", action="store_true",
                    help="Force full precision (disable half).")

    ap.add_argument("--say",       required=True, help="Text to speak")
    ap.add_argument("--text_lang", required=True, help="ja|zh|en|ko|yue or 中文/日文/…")
    ap.add_argument("--ref_wav",   required=True, help="Reference wav path")
    ap.add_argument("--ref_text",  required=True, help="Reference transcript")
    ap.add_argument("--ref_lang",  required=True, help="ja|zh|en|ko|yue or 中文/日文/…")
    ap.add_argument("--out",       default="out.wav", help="Output wav")

    args = ap.parse_args()
    is_half = (args.device.startswith("cuda") and not args.full_precision)

    # fallbacks if config left empty
    sovits = args.sovits_path or g.pretrained_sovits_path
    gpt    = args.gpt_path or g.pretrained_gpt_path

    svc = TTSService(args.device, is_half, args.hubert_path, args.bert_path, gpt, sovits)
    sr, wav = svc.synth(
        ref_wav_path=args.ref_wav,
        prompt_text=args.ref_text,
        prompt_lang=args.ref_lang,
        text=args.say,
        text_lang=args.text_lang,
    )
    sf.write(args.out, wav, sr)
    print("Wrote:", args.out)

if __name__ == "__main__":
    main()
