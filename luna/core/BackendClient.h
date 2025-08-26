// BackendClient.h

/*
  Interface; emits finished (text, audioPath)
*/

#pragma once
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

struct BackendResult {
  QString echoText;     // original user text to show again
  QUrl    audioUrl;     // http(s) URL (from "url", resolved against base)
  QUrl    localFile;    // file:// path (from "path"), optional
  int     sampleRate = 0;
};

class BackendClient : public QObject {
  Q_OBJECT
public:
  explicit BackendClient(QObject* parent=nullptr);

  void setBaseUrl(const QUrl& base);     // e.g. http://127.0.0.1:9880
  void setTextLang(const QString& lang); // "ja"/"zh"/"en"

public slots:
  void submit(const QString& userText);  // non-blocking

signals:
  void status(const QString& s);         // e.g., "LUNA …"
  void ready(const BackendResult& r);    // parsed payload
  void error(const QString& msg);

private:
  QNetworkAccessManager* nam_;
  QUrl    baseUrl_     { QStringLiteral("http://127.0.0.1:9880") };
  QString textLang_    { QStringLiteral("ja") };
  QString pendingText_;                    // <--- was missing

  void handleReply(QNetworkReply* rep);
  static QUrl resolveMaybeRelative(const QUrl& base, const QString& maybe);
};
