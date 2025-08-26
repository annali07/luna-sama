// AudioPlayer.h

/*
  Wraps QMediaPlayer/QAudioOutput
*/

#pragma once
#include <QObject>
#include <QUrl>

class QMediaPlayer;
class QAudioOutput;

class AudioPlayer : public QObject {
  Q_OBJECT
public:
  explicit AudioPlayer(QObject* parent=nullptr);

  void play(const QUrl& url);
  void stop();
  bool isPlaying() const;
  void setVolume(int percent);   // 0..100

signals:
  void finished();               // End of media reached
  void error(const QString& msg);

private:
  QMediaPlayer*  player_ = nullptr;
  QAudioOutput*  audio_  = nullptr;

  void hookSignals();
};
