# Luna Desktop Pet

![Demo](docs/readme/luna_demo.gif)

## Core Functionality
- After inputting text into the textbox, you will get a response with Luna's talking style. 
- An audio with Luna's voice will automatically read the response text. 
- So just as if you are having a conversation with Luna! Live!

## Components
- App is build using Qt framework, Qt 3.9.1.
- Backend leverages a Qwen3-8B model fined-tuned with QLoRA with 10000 Luna's lines to talk like Luna.
- Backend leverages GPT-SoVits TTS model trained with 3.8k+ audio files of luna's voice. 

## Additioal Features 
- Fades to 50% opacity when mouse is not on the figure for > 10 seconds. 
- Left-click changes the expressions (but in a same set of clothes)
- Right-click opens menu, in which you can:
  - Change the set of clothes to display
  - Close the App. 

**NOTE** Only one window of the app can be opened at a time. 


# Repository

# Setup

# Build
In PowerShell:
```
$env:Path = "E:\Qt\Tools\mingw1310_64\bin;E:\Qt\6.9.1\mingw_64\bin;$env:Path"

cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="E:/Qt/6.9.1/mingw_64" `
  -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
  -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build --config Release -j
```