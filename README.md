# Luna-sama's voice trained with GPT-SoVits
Training data: 3806 `.ogg` files extracted from Tsuki ni Yorisou Otome no Sahou 
Trained with RTX4090
Emotion classification: None (implementing it rn..(will I implement it?)

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

# Note
This repo contains many directories from https://github.com/RVC-Boss/GPT-SoVITS.
I factored out some key files into this repo, together with trained weights for Luna's voice (located in pretrained\_weights)

After running `./run.sh`, you can input **Japanese** text and a `.wav` audio file will be generated in `repl_out` (feel free to change the name of this output directory in `run.sh`). 

Currently only Japanese is supported (I mean you can type Chinese and output a chinese audio but the quality is uh.... um... not ideal)



