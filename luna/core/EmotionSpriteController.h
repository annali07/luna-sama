#pragma once
#include <QObject>
#include <QHash>
#include <QStringList>

class ModeManager;

class EmotionSpriteController : public QObject {
  Q_OBJECT
public:
  explicit EmotionSpriteController(ModeManager* modes, QObject* parent=nullptr);

  void reloadForCurrentMode();           // read <modeDir>/summary.json
  bool applyEmotion(const QString& token); // set frame by token
  void maybeSmirk(int probabilityPct = 30);

signals:
  void frameChosen(const QString& absPath);   // OPTIONAL: emit chosen PNG path


private:
  ModeManager* modes_;
  QHash<QString, QStringList> lists_;    // "<E:smile>" -> ["lun_s_..."]
  QString pickOne(const QString& token) const;
};
