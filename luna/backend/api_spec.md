# Local HTTP

Run a Python FastAPI service that:
- Receives `{ "text": "<user input>" }`
- Calls your LLM to get response_text
- Feeds response_text to GPT-SoVITS → returns a WAV path

Responds JSON:
```
{
  "response_text": "…",
  "audio_url": "http://127.0.0.1:9880/out/xyz.wav",
  "voice_ms": 4231
}
```