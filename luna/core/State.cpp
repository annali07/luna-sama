/*

State { Idle, Waiting, Playing }

Idle → (user submit) → Waiting

Waiting → (backend returns) → Playing (start audio and show response)

Playing → (audio finished) → Idle

Encapsulate in State with signals: enteredIdle(), enteredWaiting(), enteredPlaying() so UI can react without fragile if/else.

*/