#!/usr/bin/env python3
import sys, torch, unicodedata
from threading import Thread
from fastapi import FastAPI
from pydantic import BaseModel
from transformers import (
    AutoTokenizer, AutoModelForCausalLM,
    BitsAndBytesConfig, TextIteratorStreamer
)
from peft import PeftModel
import uvicorn

# ------------ Config ------------
BASE_MODEL = "Qwen/Qwen3-8B"
ADAPTER_DIR = "qwen3-luna-qlora"   # change to your adapter path
SYSTEM_PROMPT = "あなたは【桜小路ルナ】として話してください。台詞は日本語で、原作の表記（「…」）を守ります。"

# ------------ Model Load ------------
def ensure_chat_template(tok):
    if tok.chat_template and tok.chat_template.strip():
        return tok
    tok.chat_template = r"""
{% for message in messages %}
{% if message['role'] == 'system' -%}
<|im_start|>system
{{ message['content'] }}<|im_end|>
{% elif message['role'] == 'user' -%}
<|im_start|>user
{{ message['content'] }}<|im_end|>
{% elif message['role'] == 'assistant' -%}
<|im_start|>assistant
{% generation %}{{ message['content'] }}{% endgeneration %}<|im_end|>
{% endif %}
{% endfor %}
{% if add_generation_prompt -%}
<|im_start|>assistant
{% endif -%}
""".strip() + "\n"
    return tok

def load_model_and_tokenizer(base_id, adapter_dir, load_in_4bit=True):
    tok = AutoTokenizer.from_pretrained(base_id, trust_remote_code=True, use_fast=False)
    tok = ensure_chat_template(tok)
    if tok.pad_token is None:
        tok.pad_token = tok.eos_token

    if load_in_4bit:
        bnb = BitsAndBytesConfig(
            load_in_4bit=True,
            bnb_4bit_quant_type="nf4",
            bnb_4bit_use_double_quant=True,
            bnb_4bit_compute_dtype=torch.bfloat16,
        )
        model = AutoModelForCausalLM.from_pretrained(
            base_id,
            device_map="auto",
            torch_dtype=torch.bfloat16,
            attn_implementation="sdpa",
            quantization_config=bnb,
            trust_remote_code=True,
        )
    else:
        model = AutoModelForCausalLM.from_pretrained(
            base_id,
            device_map="auto",
            torch_dtype=torch.float16,
            attn_implementation="sdpa",
            trust_remote_code=True,
        )

    model = PeftModel.from_pretrained(model, adapter_dir)
    model.eval()
    return model, tok

def format_inputs(tokenizer, messages):
    return tokenizer.apply_chat_template(
        messages, add_generation_prompt=True, tokenize=True,
        return_tensors="pt"
    )

print("Loading model…", file=sys.stderr)
model, tok = load_model_and_tokenizer(BASE_MODEL, ADAPTER_DIR, load_in_4bit=True)

# ------------ API Server ------------
app = FastAPI()

class ChatRequest(BaseModel):
    user: str

class ChatResponse(BaseModel):
    emotion: str
    sentence: str

@app.post("/chat", response_model=ChatResponse)
def chat(req: ChatRequest):
    # conversation: system + user
    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user", "content": req.user}
    ]
    input_ids = format_inputs(tok, messages).to(model.device)

    eos_id = tok.convert_tokens_to_ids("<|im_end|>")
    gen_kwargs = dict(
        max_new_tokens=128,
        do_sample=True,
        temperature=0.3,
        top_p=0.9,
        repetition_penalty=1.1,
        eos_token_id=[tok.eos_token_id, eos_id],
        pad_token_id=tok.pad_token_id,
    )

    streamer = TextIteratorStreamer(tok, skip_special_tokens=True, skip_prompt=True)
    th = Thread(target=model.generate, kwargs={
        "inputs": input_ids,
        "streamer": streamer,
        **{k:v for k,v in gen_kwargs.items() if v is not None}
    })
    th.start()

    buf = []
    newline_count = 0
    for piece in streamer:
        # count newlines in this piece and keep only up to the 2nd newline
        if newline_count < 2 and "\n" in piece:
            parts = piece.split("\n")
            for i, part in enumerate(parts):
                if i < len(parts) - 1:            # this sub-part ends with a newline
                    buf.append(part + "\n")
                    newline_count += 1
                    if newline_count >= 2:
                        break
                else:
                    if newline_count < 2:
                        buf.append(part)          # last fragment (no newline)
            if newline_count >= 2:
                break
        else:
            buf.append(piece)

    # drain so the background thread can finish cleanly
    for _ in streamer:
        pass

    text = "".join(buf).rstrip() + "\n"           # ensure final newline


    # ---- Split into emotion line + one sentence ----
    lines = [ln for ln in text.split("\n") if ln.strip()]
    emotion, sentence = "", ""
    if len(lines) >= 1:
        emotion = lines[0]
    if len(lines) >= 2:
        sentence = lines[1]

    return ChatResponse(emotion=emotion, sentence=sentence)

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
