/*

HttpBackendClient (example)

Config in AppConfig: base URL, timeouts.

submit(text) → POST /v1/reply → parse JSON → emit success(...).

FastAPI sketch (in backend/examples/fastapi_server.py):

POST /v1/reply with {text} body.

Returns {response_text, audio_url, voice_ms}.


*/