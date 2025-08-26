set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"


python -m gsv.api \
   -s $ROOT/gsv/weights/xxx_sovits_e24_s456.pth \
  -g $ROOT/gsv/weights/xxx-gpt-e50.ckpt\
  -hb $ROOT/gsv/pretrained_models/chinese-hubert-base \
  -b  $ROOT/gsv/pretrained_models/chinese-roberta-wwm-ext-large \
  -d cuda:0 \
  --text_lang ja \
  -dr  $ROOT/gsv/extracted_ogg/v_lun0022.ogg \
  -dt  "ただまあ――正直に言ってしまえば、私の身の回りの世話を最低限できそうな人なら誰でも良かったんだ" \
  -dl ja\
  --ref_wav $ROOT/gsv/extracted_ogg/v_lun0022.ogg \
  --ref_text "ただまあ――正直に言ってしまえば、私の身の回りの世話を最低限できそうな人なら誰でも良かったんだ" \
  --ref_lang ja \
  --out_dir out_repl \
  --basename luna

