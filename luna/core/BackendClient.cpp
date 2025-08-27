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

void BackendClient::setLlmBaseUrl(const QUrl& base) { llmBaseUrl_ = base; }
void BackendClient::setTtsBaseUrl(const QUrl& base) { ttsBaseUrl_ = base; }
void BackendClient::setTextLang(const QString& l)   { textLang_   = l;   }

void BackendClient::submit(const QString& userText) {
  pendingUser_.clear();
  pendingEmotion_.clear();
  pendingSentence_.clear();
  pendingEchoText_.clear();

  pendingUser_ = userText;
  emit status(QStringLiteral("LUNA …"));

  QUrl url = llmBaseUrl_.resolved(QUrl(QStringLiteral("/chat")));
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
  const QJsonObject payload{{QStringLiteral("user"), userText}};
  auto* rep = nam_->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
  connect(rep, &QNetworkReply::finished, this, [this, rep]{ handleLlmReply(rep); });
}

void BackendClient::handleLlmReply(QNetworkReply* rep) {
  rep->deleteLater();

  if (rep->error() != QNetworkReply::NoError) {
    emit error(QStringLiteral("LLM error: %1").arg(rep->errorString()));
    return;
  }

  const QByteArray bytes = rep->readAll();
  QJsonParseError pe{};
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
  if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
    emit error(QStringLiteral("LLM: bad JSON"));
    return;
  }
  const auto obj = doc.object();

  // { "emotion":"<E:smile>", "sentence":"「…」" }
  pendingEmotion_  = obj.value(QStringLiteral("emotion")).toString();
  pendingSentence_ = obj.value(QStringLiteral("sentence")).toString();

  if (pendingSentence_.trimmed().isEmpty()) {
    emit error(QStringLiteral("LLM: missing 'sentence'"));
    return;
  }

  // Build the display text for GUI (keep your preferred formatting)
  pendingEchoText_ = pendingEmotion_.isEmpty()
                   ? pendingSentence_
                   : pendingSentence_;

               
  // 🔔 Notify emotion to the sprite controller immediately
  if (!pendingEmotion_.trimmed().isEmpty()) {
    emit emotionAvailable(pendingEmotion_.trimmed());
  }
 
  emit status(QStringLiteral("… …"));

  // Kick off TTS on the spoken line
  QUrl tts = ttsBaseUrl_.resolved(QUrl(QStringLiteral("/speak")));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("text"), pendingSentence_);
  q.addQueryItem(QStringLiteral("text_lang"), textLang_);
  tts.setQuery(q);

  auto* ttsRep = nam_->get(QNetworkRequest(tts));
  connect(ttsRep, &QNetworkReply::finished, this, [this, ttsRep]{ handleTtsReply(ttsRep); });
}

void BackendClient::handleTtsReply(QNetworkReply* rep) {
  rep->deleteLater();

  BackendResult r;
  r.echoText = pendingEchoText_;   // GUI text only

  if (rep->error() != QNetworkReply::NoError) {
    emit error(QStringLiteral("TTS error: %1").arg(rep->errorString()));
    emit ready(r);                  // deliver text even if no audio
    return;
  }

  const QByteArray bytes = rep->readAll();
  QJsonParseError pe{};
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
  if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
    emit error(QStringLiteral("TTS: bad JSON"));
    emit ready(r);
    return;
  }

  const auto obj   = doc.object();
  const bool ok    = obj.value(QStringLiteral("ok")).toBool(true);
  const QString u  = obj.value(QStringLiteral("url")).toString();
  const QString p  = obj.value(QStringLiteral("path")).toString();
  r.sampleRate     = obj.value(QStringLiteral("sample_rate")).toInt(0);

  if (!u.isEmpty()) r.audioUrl  = resolveMaybeRelative(ttsBaseUrl_, u);
  if (!p.isEmpty()) r.localFile = QUrl::fromLocalFile(p);

  if (!ok && !r.audioUrl.isValid() && !r.localFile.isValid()) {
    emit error(QStringLiteral("TTS: no audio"));
  }

  emit ready(r);
}

QUrl BackendClient::resolveMaybeRelative(const QUrl& base, const QString& maybe) {
  if (maybe.isEmpty()) return {};
  const QUrl u(maybe);
  return u.isRelative() ? base.resolved(u) : u;
}
