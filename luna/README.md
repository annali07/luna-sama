# Wiring it together (signals/slots)
# Original Envision 

1. User submits text
- `IOOverlay::submitted(text)` → `State.toWaiting()` → `BackendClient::submit(text)`
- `IOOverlay` shows status `(“LUNA …”)`, disables input.
2. Backend returns 
```
BackendClient::success(replyText, audio, voiceMs):
  IOOverlay.showOutput(replyText) (still disabled)
  AudioPlayer.play(audio)
  State.toPlaying()
```
3. Audio finishes
```
AudioPlayer::finished():
  IOOverlay.backToInputMode()
  State.toIdle()
```
4. Errors
- Show a small toast/balloon near the pet (QToolTip or custom bubble).
```
BackendClient::error(msg):

  IOOverlay.backToInputMode()
  State.toIdle()
```


```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/desktop_pet   # (or build\Debug\desktop_pet.exe on Windows)
```


## NOTE
- To change the drag, default = Alt + Left click. Change the Alt key in `MainWindow.h` and `MainWindow.cpp`. Currently suppoprt Alt, Ctrl, Shift. Can freely change between these in app UI.