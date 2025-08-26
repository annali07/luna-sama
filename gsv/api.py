# gsv/api.py
import os, time, uuid, argparse, threading
import soundfile as sf
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles

from .config_infer import Config
from .service import TTSService

def make_app(args: argparse.Namespace) -> FastAPI:
    app = FastAPI(title="SoVITS API", version="1.0")
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"], allow_credentials=True, allow_methods=["*"], allow_headers=["*"]
    )
    os.makedirs(args.out_dir, exist_ok=True)
    app.mount("/audio", StaticFiles(directory=args.out_dir), name="audio")

    is_half = (args.device.startswith("cuda") and not args.full_precision)
    sovits = args.sovits_path or args._cfg.pretrained_sovits_path
    gpt    = args.gpt_path    or args._cfg.pretrained_gpt_path
    svc = TTSService(args.device, is_half, args.hubert_path, args.bert_path, gpt, sovits)

    # Reference fixed at launch (can be changed later via /set_ref if desired)
    defaults = {
        "ref_wav":  args.ref_wav,
        "ref_text": args.ref_text,
        "ref_lang": args.ref_lang,
        "text_lang": args.text_lang or "zh",
        "basename": args.basename or "utt",
    }

    # Validate reference now so /speak never needs it
    if not os.path.exists(defaults["ref_wav"]):
        raise RuntimeError(f"ref_wav not found: {defaults['ref_wav']}")
    if not (defaults["ref_text"] and defaults["ref_lang"]):
        raise RuntimeError("ref_text/ref_lang must be provided at launch.")

    synth_lock = threading.Lock()

    @app.get("/health")
    def health():
        return {"ok": True}

    @app.get("/config")
    def get_config():
        return {
            "device": args.device,
            "out_dir": os.path.abspath(args.out_dir),
            "defaults": defaults,
        }

    # Optional hot-swap of reference later
    @app.get("/set_ref")
    def set_ref(
        ref_wav: str = Query(None), ref_text: str = Query(None),
        ref_lang: str = Query(None), text_lang: str = Query(None),
        basename: str = Query(None),
    ):
        if ref_wav is not None:
            if not os.path.exists(ref_wav):
                raise HTTPException(400, f"not found: {ref_wav}")
            defaults["ref_wav"] = ref_wav
        if ref_text  is not None: defaults["ref_text"]  = ref_text
        if ref_lang  is not None: defaults["ref_lang"]  = ref_lang
        if text_lang is not None: defaults["text_lang"] = text_lang
        if basename  is not None: defaults["basename"]  = basename
        return {"ok": True, "defaults": defaults}

    # Inference: only text (+ optional text_lang)
    @app.get("/speak")
    def speak(
        text: str = Query(..., description="Target text to synthesize"),
        text_lang: str = Query(None, description="Override default text language"),
        speed: float = 1.0,
        top_k: int = 15, top_p: float = 0.6, temperature: float = 0.6,
        sample_steps: int = 32,
        basename: str = Query(None, description="Override output filename stem"),
    ):
        _text_lang = (text_lang or defaults["text_lang"]).strip()
        _ref_wav, _ref_text, _ref_lang = defaults["ref_wav"], defaults["ref_text"], defaults["ref_lang"]

        stem = (basename or defaults["basename"] or "utt")
        fname = f"{stem}_{int(time.time()*1000)}_{uuid.uuid4().hex[:6]}.wav"
        out_path = os.path.join(args.out_dir, fname)

        try:
            with synth_lock:
                sr, wav = svc.synth(
                    _ref_wav, _ref_text, _ref_lang, text, _text_lang,
                    top_k=top_k, top_p=top_p, temperature=temperature,
                    speed=speed, sample_steps=sample_steps,
                )
            sf.write(out_path, wav, sr)
        except Exception as e:
            raise HTTPException(500, f"synthesis failed: {e}")

        return {
            "ok": True,
            "sample_rate": sr,
            "url": f"/audio/{fname}",
            "path": os.path.abspath(out_path),
            "text_lang": _text_lang,
        }

    return app

def parse_args():
    g = Config()
    ap = argparse.ArgumentParser("gsv-api")

    # model/runtime
    ap.add_argument("-s","--sovits_path", default=g.sovits_path)
    ap.add_argument("-g","--gpt_path",    default=g.gpt_path)
    ap.add_argument("-hb","--hubert_path",default=g.cnhubert_path)
    ap.add_argument("-b","--bert_path",   default=g.bert_path)
    ap.add_argument("-d","--device",      default=g.infer_device)
    ap.add_argument("--fp","--full_precision", dest="full_precision", action="store_true")

    ap.add_argument("-dr","--default_ref_wav", default="")
    ap.add_argument("-dt","--default_ref_text", default="")
    ap.add_argument("-dl","--default_ref_lang", default="")
    
    # output
    ap.add_argument("--out_dir", default="out_api")
    ap.add_argument("--basename", default="utt")

    # reference (REQUIRED: pass via shell script at startup)
    ap.add_argument("--ref_wav",  required=True, help="Reference wav path")
    ap.add_argument("--ref_text", required=True, help="Reference transcript")
    ap.add_argument("--ref_lang", required=True, help="ja|zh|en|ko|yue or 中文/日文/…")

    # default text language for /speak
    ap.add_argument("--text_lang", default="zh")

    # server
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=9880)

    args = ap.parse_args()
    args._cfg = g
    return args

def main():
    import uvicorn
    args = parse_args()
    app = make_app(args)
    uvicorn.run(app, host=args.host, port=args.port, workers=1)

if __name__ == "__main__":
    main()
