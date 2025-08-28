# Luna's voice trained with GPT-SoVits
Training data: 3806 `.ogg` files extracted from Tsuki ni Yorisou Otome no Sahou

Trained with RTX4090

Emotion Classification: None (implementing it rn..(will I implement it?))

# Installation
```
cd luna-sama
conda create -n luna python=3.10
conda activate luna
pip install -r requirements.txt
```

# Start Inference
```
cd luna-sama
chmod +x run.sh
./run.sh
```


## Weights
1. Download the entire directory [pretrained_weights](https://huggingface.co/lj1995/GPT-SoVITS/tree/main) and put them into `/gsv/pretrained_models`
2. Download the trained weights from hf: [luna_weights](https://huggingface.co/nanax14/luna-sama/tree/main) and put the 2 files (`xxx-gpt-e50.ckpt` and `xxx_sovits_e24_s456.pth`) into `gsv/weights`


# Note
This repo contains many directories from https://github.com/RVC-Boss/GPT-SoVITS.
I factored out some key files into this repo, together with trained weights for Luna's voice (located in gsv/weights)

After running `./run.sh`, you can input **Japanese** text and a `.wav` audio file will be generated in `repl_out` (feel free to change the name of this output directory in `run.sh`). 

Currently only Japanese is supported (I mean you can type Chinese and output a chinese audio but the quality is uh.... um... not ideal)


