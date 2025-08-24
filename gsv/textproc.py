import re
import torch
from .text.LangSegmenter import LangSegmenter
from .text import cleaned_text_to_sequence
from .text.cleaner import clean_text

# Language mapping (same keys your API accepted)
dict_language = {
    "中文": "all_zh", "粤语": "all_yue", "英文": "en", "日文": "all_ja", "韩文": "all_ko",
    "中英混合": "zh", "粤英混合": "yue", "日英混合": "ja", "韩英混合": "ko",
    "多语种混合": "auto", "多语种混合(粤语)": "auto_yue",
    "all_zh": "all_zh", "all_yue": "all_yue", "en": "en",
    "all_ja": "all_ja", "all_ko": "all_ko",
    "zh": "zh", "yue": "yue", "ja": "ja", "ko": "ko",
    "auto": "auto", "auto_yue": "auto_yue",
}

class TextFrontend:
    """Holds tokenizer/bert and provides phones/bert extraction."""
    def __init__(self, tokenizer, bert_model, device, is_half: bool):
        self.tok = tokenizer
        self.bert = bert_model
        self.device = device
        self.is_half = is_half

    @torch.no_grad()
    def _bert_feature(self, text: str, word2ph):
        inputs = self.tok(text, return_tensors="pt")
        for k in inputs: inputs[k] = inputs[k].to(self.device)
        res = self.bert(**inputs, output_hidden_states=True)
        # last-3 to last-2 slice (exactly as original code)
        hid = torch.cat(res["hidden_states"][-3:-2], -1)[0].cpu()[1:-1]
        assert len(word2ph) == len(text)
        feats = []
        for i in range(len(word2ph)):
            feats.append(hid[i].repeat(word2ph[i], 1))
        feats = torch.cat(feats, dim=0)    # [phones, 1024]
        return feats.T                     # [1024, phones]

    def _clean_text_inf(self, text, lang, version):
        lang = lang.replace("all_", "")
        phones, word2ph, norm_text = clean_text(text, lang, version)
        phones = cleaned_text_to_sequence(phones, version)
        return phones, word2ph, norm_text

    def _bert_inf(self, phones, word2ph, norm_text, lang):
        lang = lang.replace("all_", "")
        if lang == "zh":
            return self._bert_feature(norm_text, word2ph).to(self.device)
        else:
            dt = torch.float16 if self.is_half else torch.float32
            return torch.zeros((1024, len(phones)), dtype=dt, device=self.device)

    def get_phones_and_bert(self, text, language, version, final=False):
        text = re.sub(r' {2,}', ' ', text)
        textlist, langlist = [], []

        if language == "all_zh":
            for x in LangSegmenter.getTexts(text, "zh"):
                langlist.append(x["lang"]); textlist.append(x["text"])
        elif language == "all_yue":
            for x in LangSegmenter.getTexts(text, "zh"):
                lang = "yue" if x["lang"] == "zh" else x["lang"]
                langlist.append(lang); textlist.append(x["text"])
        elif language == "all_ja":
            for x in LangSegmenter.getTexts(text, "ja"):
                langlist.append(x["lang"]); textlist.append(x["text"])
        elif language == "all_ko":
            for x in LangSegmenter.getTexts(text, "ko"):
                langlist.append(x["lang"]); textlist.append(x["text"])
        elif language == "en":
            langlist, textlist = ["en"], [text]
        elif language == "auto":
            for x in LangSegmenter.getTexts(text):
                langlist.append(x["lang"]); textlist.append(x["text"])
        elif language == "auto_yue":
            for x in LangSegmenter.getTexts(text):
                lang = "yue" if x["lang"] == "zh" else x["lang"]
                langlist.append(lang); textlist.append(x["text"])
        else:
            for x in LangSegmenter.getTexts(text):
                if langlist:
                    if (x["lang"] == "en" and langlist[-1] == "en") or (x["lang"] != "en" and langlist[-1] != "en"):
                        textlist[-1] += x["text"]; continue
                langlist.append("en" if x["lang"] == "en" else language)
                textlist.append(x["text"])

        phones_list, bert_list, norm_text_list = [], [], []
        for i in range(len(textlist)):
            lang = langlist[i]
            phones, word2ph, norm_text = self._clean_text_inf(textlist[i], lang, version)
            bert = self._bert_inf(phones, word2ph, norm_text, lang)
            phones_list.append(phones); norm_text_list.append(norm_text); bert_list.append(bert)

        bert = torch.cat(bert_list, dim=1)
        phones = sum(phones_list, [])
        norm_text = "".join(norm_text_list)

        if not final and len(phones) < 6:
            return self.get_phones_and_bert("." + text, language, version, final=True)

        dt = torch.float16 if self.is_half else torch.float32
        return phones, bert.to(dt), norm_text
