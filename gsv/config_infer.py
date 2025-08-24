import torch

# ---- Default model paths (you can override with CLI flags) ----
sovits_path = ""        # trained SoVITS .pth; leave empty to pass -s
gpt_path    = ""        # trained GPT .ckpt;  leave empty to pass -g

# Feature extractors (HF-style folders)
cnhubert_path = "GPT_SoVITS/pretrained_models/chinese-hubert-base"
bert_path     = "GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large"

# Optional fallbacks if -s / -g are omitted
pretrained_sovits_path = "GPT_SoVITS/pretrained_models/s2G488k.pth"
pretrained_gpt_path    = "GPT_SoVITS/pretrained_models/s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt"

# ---- Device / precision defaults ----
infer_device = "cuda" if torch.cuda.is_available() else "cpu"
is_half      = infer_device == "cuda"

class Config:
    def __init__(self):
        self.sovits_path = sovits_path
        self.gpt_path    = gpt_path

        self.cnhubert_path = cnhubert_path
        self.bert_path     = bert_path
        self.pretrained_sovits_path = pretrained_sovits_path
        self.pretrained_gpt_path    = pretrained_gpt_path

        self.infer_device = infer_device
        self.is_half      = is_half
