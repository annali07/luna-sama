// AudioPlayer.h

/*
  Wraps QMediaPlayer/QAudioOutput
*/

class AudioPlayer : public QObject {
  Q_OBJECT
public:
  AudioPlayer(QObject* parent=nullptr);
  void play(const QUrl& urlOrPath);
  void stop();
signals:
  void started();
  void finished();
  void error(const QString& msg);
};

