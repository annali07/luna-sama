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
#include "AudioPlayer.h"
#include <QMediaPlayer>
#include <QAudioOutput>

AudioPlayer::AudioPlayer(QObject* parent) : QObject(parent) {
  player_ = new QMediaPlayer(this);
  audio_  = new QAudioOutput(this);
  player_->setAudioOutput(audio_);
  audio_->setVolume(0.8);            // ~80%
  hookSignals();
}

void AudioPlayer::hookSignals() {
  // End-of-media
  connect(player_, &QMediaPlayer::mediaStatusChanged, this,
          [this](QMediaPlayer::MediaStatus st){
            if (st == QMediaPlayer::EndOfMedia) emit finished();
          });

  // Qt 6: errorChanged() has NO args; query player_->error()
  connect(player_, &QMediaPlayer::errorChanged, this, [this](){
    if (player_->error() != QMediaPlayer::NoError) {
      const QString msg = player_->errorString().isEmpty()
                        ? QStringLiteral("Audio error")
                        : player_->errorString();
      emit error(msg);
    }
  });
}

void AudioPlayer::play(const QUrl& url) {
  player_->stop();
  player_->setSource(url);       // supports http(s) and file://
  player_->play();
}

void AudioPlayer::stop() {
  player_->stop();
}

bool AudioPlayer::isPlaying() const {
  return player_->playbackState() == QMediaPlayer::PlayingState;
}

void AudioPlayer::setVolume(int percent) {
  const qreal v = qBound(0, percent, 100) / 100.0;
  audio_->setVolume(v);
}
