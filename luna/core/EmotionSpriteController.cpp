#include "EmotionSpriteController.h"
#include "ModeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QJsonArray>   // ← add this
#include <QJsonValue>   // ← and this
#include<QFileInfo>

EmotionSpriteController::EmotionSpriteController(ModeManager* m, QObject* p)
  : QObject(p), modes_(m)
{
  // keep map in sync with mode changes
  connect(modes_, &ModeManager::modeChanged, this, [this](const QString&){
    reloadForCurrentMode();
  });
  reloadForCurrentMode();
}
void EmotionSpriteController::reloadForCurrentMode() {
  lists_.clear();
  const QString dir = modes_->modeDir();

  // Prefer sum.json; (optional) fall back to summary/combined if present
  const QStringList candidates = { "sum.json", "summary.json", "combined.json" };
  QString jsonPath;
  for (const auto& name : candidates) {
    const QString p = dir + "/" + name;
    if (QFileInfo::exists(p)) { jsonPath = p; break; }
  }
  if (jsonPath.isEmpty()) {
    return;
  }

  QFile f(jsonPath);
  if (!f.open(QIODevice::ReadOnly)) return;   // ← add this

  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());

  const QJsonObject obj = doc.object();
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    if (!it.value().isArray()) continue;
    QStringList v;
    for (const QJsonValue& e : it.value().toArray())
      if (e.isString()) v << e.toString();
    if (!v.isEmpty()) lists_.insert(it.key(), v);
  }
}

QString EmotionSpriteController::pickOne(const QString& token) const {
  const auto it = lists_.find(token);
  if (it == lists_.end() || it->isEmpty()) return {};
  const int idx = QRandomGenerator::global()->bounded(it->size());
  return it->at(idx);
}

bool EmotionSpriteController::applyEmotion(const QString& token) {
  // --- bias to smile with some probability ---
  constexpr int kSmileProbPct = 25;                              // ← 25% chance
  static const QString kSmile = QStringLiteral("<E:smile>");
  QString chosen = token.trimmed();

  if (chosen != kSmile && kSmileProbPct > 0) {
    // only consider bias if we actually have smile frames in sum.json
    const bool haveSmile = lists_.contains(kSmile) && !lists_.value(kSmile).isEmpty();
    if (haveSmile && QRandomGenerator::global()->bounded(100) < kSmileProbPct) {
      chosen = kSmile;
    }
  }

  // --- existing selection logic (unchanged), but use 'chosen' instead of 'token' ---
  const QString base = pickOne(chosen);

  if (base.isEmpty()) return false;

  // try by basename (indexed frames)
  if (modes_->setFrameByBasename(base)) {
    const QString abs = modes_->modeDir() + "/" + base + ".png";
    return true;
  }

  // fallback: absolute path
  const QString abs = modes_->modeDir() + "/" + base + ".png";
  const bool exists = QFileInfo::exists(abs);
  if (exists) {
    const bool ok = modes_->ensureAndSetFramePath(abs);
    return ok;
  }

  return false;
}

void EmotionSpriteController::maybeSmirk(int probabilityPct) {
  probabilityPct = qBound(0, probabilityPct, 100);
  if (QRandomGenerator::global()->bounded(100) < probabilityPct)
    applyEmotion(QStringLiteral("<E:smirk>"));
}
