/*

AudioPlayer

Wrap QMediaPlayer + QAudioOutput.

API:

void play(const QUrl& urlOrPath);

void stop();

bool isPlaying() const;

Signals:

started(); finished(); error(QString)

Hook mediaStatusChanged / playbackStateChanged to detect end, then emit finished().

*/