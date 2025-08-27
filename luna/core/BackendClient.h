#pragma once
#include <QObject>
#include <QUrl>
#include <QString> 
class QNetworkAccessManager;
class QNetworkReply;

struct BackendResult {
  QString echoText;     // LLM: "<E:...>\n「…」"  (what you display)
  QString emotion;      // optional: "<E:...>"
  QString sentence;     // optional: 「…」
  QUrl    audioUrl;     // http(s) URL to audio (if provided by TTS)
  QUrl    localFile;    // file:// path, optional
  int     sampleRate = 0;
};

class BackendClient : public QObject {
  Q_OBJECT
public:
  explicit BackendClient(QObject* parent=nullptr);

  // URLs
  void setLlmBaseUrl(const QUrl& base);            // e.g. http://127.0.0.1:8000
  void setTtsBaseUrl(const QUrl& base);            // e.g. http://127.0.0.1:9880
  void setBaseUrl(const QUrl& base) { setTtsBaseUrl(base); } // backward compat


  void setTextLang(const QString& lang);           // "ja"/"zh"/"en"

public slots:
  void submit(const QString& userText);            // user → LLM → TTS (async chain)

signals:
  void status(const QString& s);                   // e.g., "LUNA …", "TTS …"
  void ready(const BackendResult& r);              // final payload (text + audio info)
  void error(const QString& msg);
  void emotionAvailable(const QString& token);  // ← ADD THIS

private:
  QNetworkAccessManager* nam_;
  QUrl llmBaseUrl_ { QStringLiteral("http://127.0.0.1:8000") };
  QUrl ttsBaseUrl_ { QStringLiteral("http://127.0.0.1:9880") };
  QString textLang_ { QStringLiteral("ja") };

  // pendings for current request
  QString pendingUser_;
  QString pendingEmotion_;
  QString pendingSentence_;
  QString pendingEchoText_;

  void handleLlmReply(QNetworkReply* rep);
  void handleTtsReply(QNetworkReply* rep);

  static QUrl resolveMaybeRelative(const QUrl& base, const QString& maybe);
};
