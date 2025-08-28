# Luna Desktop Pet

![Demo](docs/readme/luna_demo.gif)

# Repository
```
LUNA-SAMA/
├── docs/               # MISC: parsed full dialugue used in training
├── gsv/                # Voice model 
├── luna/               # Qt App code
├── luna_llm/           # QLora weights
├── out_repl/           # Output directory of .wav files from vocie model
├── run_api_sovits.sh   # Runs voice model on port 9880 
└── run_api_llm.sh      # Runs llm on port 8000
```

## Core Functionality
- Enter text in the input box and receive Luna’s reply in her style.  
- Replies will be spoken by Luna's voice (audio plays automatically when there is a response from Luna)
- So just as if you are having a conversation with Luna! Live!

## Components/Details
- App is build using Qt framework, Qt 3.9.1.
- LLM Chatbot Backend: Qwen3-8B fine-tuned with 10k Luna dialogue lines (QLoRA).  
- TTS: GPT-SoVits trained on 3.8k+ Luna audio clips.  
- Training was done on RTX 4090; inference runs on RTX 3080 (10GB VRAM).  


# Setup
- ~21GB of disk space needed in total (16GB Qwen3-8B, 4.5GB GPT-Sovits, 0.2GB QLoRA weights)
- Minimum 10GB of GPU VRAM needed
- WSL needed (the shell scripts are written with the intention of running in wsl mode)

## Installation
```
# in windows PowerShell
wsl          # Enable WSL

# in WSL
cd luna-sama
conda create -n luna python=3.10
conda activate luna
pip install -r requirements.txt
```

## Model Weights
1. Download the entire directory [pretrained_weights](https://huggingface.co/lj1995/GPT-SoVITS/tree/main) and put them into `/gsv/pretrained_models` (Don't end up with something like `/gsv/pretrained_models/pretrained_models`. NO. Get rid of the nested folder)
2. Download the trained weights from hf: [luna_weights](https://huggingface.co/nanax14/luna-sama/tree/main) and put the 2 files (`xxx-gpt-e50.ckpt` and `xxx_sovits_e24_s456.pth`) into `gsv/weights`
3. Download the trained QLoRA weights from hf: [qwen3-luna-lora](https://huggingface.co/nanax14/luna-sama/tree/main) and put the file in /luna_llm/qwen3-luna-qlora
4. When you run `./run_api_llm.sh` for the first time, it should automatically download `Qwen3-8B` from huggingface for you: `~/home/<user>/.cache/huggingface/hub/models--Qwen--Qwen3-8B`. This thing will be 16GB. (Note this is in WSL mode)


## Running the models
```
chmod +x run_api_llm.sh run_api_sovits.sh
./run_api_llm.sh      # on one terminal window 
./run_api_sovits.sh   # on another terminal window
```

## Download luna.zip from Github (contains luna_sama.exe)
Go to https://github.com/annali07/luna-sama and under **Release** you can download the `luna_sama.zip`. Unzip it and it contains the .exe file, which is the application of Luna (with the input box, switch pictures etc.) shown in the above demo. 

If you have the above 2 models running, you can directly input text into the input box, and you will get a reply from Luna, with her speaking her reply to you > <

### [NOT NECESSARY] But if you have Qt downloaded and want to build from source...
In PowerShell:
```
# replace with your path <...\Qt\Tools\mingw...>
$env:Path = "E:\Qt\Tools\mingw1310_64\bin;E:\Qt\6.9.1\mingw_64\bin;$env:Path"

cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="E:/Qt/6.9.1/mingw_64" `
  -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
  -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build --config Release -j
```

# About the .exe
- Fades to 50% opacity when mouse is not on the figure for > 10 seconds. 
- Left-click changes the expressions (but in a same set of clothes)
- Right-click opens menu, in which you can:
  - Change the set of clothes to display
  - Close the App. 
- Hold `alt` + left-click, you can drag Luna anywhere. 
- Hold `alt` + mouse-scroll, you can adjust the size of the figure. 


**NOTE** Only one window of the app can be opened at a time. 
