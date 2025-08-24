import os
import sys
import argparse
import soundfile as sf
from .config_infer import Config
from .service import TTSService

def main():
    g = Config()
    ap = argparse.ArgumentParser("gsv-infer-repl")
    ap.add_argument("-s","--sovits_path", default=g.sovits_path)
    ap.add_argument("-g","--gpt_path",    default=g.gpt_path)
    ap.add_argument("-hb","--hubert_path",default=g.cnhubert_path)
    ap.add_argument("-b","--bert_path",   default=g.bert_path)
    ap.add_argument("-d","--device",      default=g.infer_device)
    ap.add_argument("--fp","--full_precision", dest="full_precision", action="store_true")
    ap.add_argument("--out_dir", default="out_repl")
    ap.add_argument("--basename", default="utt")
    # optional defaults for reference
    ap.add_argument("-dr","--default_ref_wav", default="")
    ap.add_argument("-dt","--default_ref_text", default="")
    ap.add_argument("-dl","--default_ref_lang", default="")
    ap.add_argument("--ref_wav",   required=True, help="Reference wav path")
    ap.add_argument("--ref_text",  required=True, help="Reference transcript")
    ap.add_argument("--ref_lang",  required=True, help="ja|zh|en|ko|yue or 中文/日文/…")
    ap.add_argument("--out",       default="out.wav", help="Output wav")

    ap.add_argument("--text_lang", default="zh")
    args = ap.parse_args()

    is_half = (args.device.startswith("cuda") and not args.full_precision)
    sovits = args.sovits_path or g.pretrained_sovits_path
    gpt    = args.gpt_path or g.pretrained_gpt_path

    svc = TTSService(args.device, is_half, args.hubert_path, args.bert_path, gpt, sovits)

    ref_wav = args.default_ref_wav
    ref_txt = args.default_ref_text
    ref_lng = args.default_ref_lang
    text_lang = args.text_lang

    os.makedirs(args.out_dir, exist_ok=True)
    n = 1

    print("REPL ready. Type text and press Enter.")
    print("Commands:")
    print("  /ref <wav> | <prompt_text> | <lang>")
    print("  /lang <lang>")
    print("  /quit")
    while True:
        try:
            line = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nBye."); sys.exit(0)

        if not line: continue
        if line.lower() in ("/quit","quit","exit","/exit"):
            print("Bye."); sys.exit(0)

        if line.startswith("/lang "):
            _, val = line.split(" ", 1)
            text_lang = val.strip()
            print(f"[OK] text_lang = {text_lang}")
            continue

        if line.startswith("/ref "):
            try:
                body = line[5:]
                wav, txt, lng = [p.strip() for p in body.split("|")]
                if not os.path.exists(wav):
                    print(f"[ERR] not found: {wav}")
                    continue
                ref_wav, ref_txt, ref_lng = wav, txt, lng
                print(f"[OK] ref set\n  wav: {wav}\n  text: {txt}\n  lang: {lng}")
            except Exception:
                print("Usage: /ref <wav> | <prompt_text> | <lang>")
            continue

        if not (ref_wav and ref_txt and ref_lng):
            print("[ERR] set a reference first with /ref or start with -dr/-dt/-dl")
            continue

        try:
            sr, wav = svc.synth(ref_wav, ref_txt, ref_lng, line, text_lang)
            out_path = os.path.join(args.out_dir, f"{args.basename}_{n:04d}.wav")
            sf.write(out_path, wav, sr)
            print("[OK]", out_path); n += 1
        except Exception as e:
            print("[ERR] synthesis failed:", e)

if __name__ == "__main__":
    main()
