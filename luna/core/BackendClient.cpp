#include "BackendClient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

BackendClient::BackendClient(QObject* parent)
  : QObject(parent),
    nam_(new QNetworkAccessManager(this)) {}

void BackendClient::setBaseUrl(const QUrl& base)  { baseUrl_ = base; }
void BackendClient::setTextLang(const QString& l) { textLang_ = l;   }

void BackendClient::submit(const QString& userText) {
  pendingText_ = userText;                         // <--- was missing
  emit status(QStringLiteral("LUNA …"));

  QUrl url = baseUrl_.resolved(QUrl(QStringLiteral("/speak")));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("text"), userText);
  q.addQueryItem(QStringLiteral("text_lang"), textLang_);
  url.setQuery(q);

  auto* rep = nam_->get(QNetworkRequest(url));
  connect(rep, &QNetworkReply::finished, this, [this, rep]{ handleReply(rep); });
}

void BackendClient::handleReply(QNetworkReply* rep) {
  rep->deleteLater();

  if (rep->error() != QNetworkReply::NoError) {
    emit error(QStringLiteral("Network error: %1").arg(rep->errorString()));
    return;
  }

  const QByteArray bytes = rep->readAll();
  QJsonParseError pe{};
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
  if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
    emit error(QStringLiteral("Bad JSON from backend"));
    return;
  }

  const auto obj = doc.object();
  const bool ok     = obj.value(QStringLiteral("ok")).toBool();
  const QString urlStr  = obj.value(QStringLiteral("url")).toString();   // "/audio/xyz.wav"
  const QString pathStr = obj.value(QStringLiteral("path")).toString();  // absolute fs path
  const int sr          = obj.value(QStringLiteral("sample_rate")).toInt(0);

  if (!ok || (urlStr.isEmpty() && pathStr.isEmpty())) {
    emit error(QStringLiteral("Backend returned no audio"));
    return;
  }

  BackendResult r;
  r.echoText   = pendingText_;                           // <--- was missing
  r.sampleRate = sr;

  if (!urlStr.isEmpty())
    r.audioUrl = resolveMaybeRelative(baseUrl_, urlStr);
  if (!pathStr.isEmpty())
    r.localFile = QUrl::fromLocalFile(pathStr);          // <--- was missing

  emit ready(r);
}

QUrl BackendClient::resolveMaybeRelative(const QUrl& base, const QString& maybe) {
  if (maybe.isEmpty()) return {};
  const QUrl u(maybe);
  return u.isRelative() ? base.resolved(u) : u;
}
