/*

Discovers ui/assets/modes//meta.json.

API:

QStringList listModes() const;

void setMode(const QString& name);

QImage currentImage() const;

void nextFrame();

Signals: modeChanged(name), frameChanged()

Right-click menu uses ModeManager::listModes() to populate submenu; selecting one calls setMode().

*/

#include "ModeManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>

ModeManager::ModeManager(QObject* parent) : QObject(parent) {
  // default search roots: beside exe, then current working dir
  QString exe = QCoreApplication::applicationDirPath();
  QString cwd = QDir::currentPath();
  setSearchRoots({ exe + "/ui/assets/modes", cwd + "/ui/assets/modes" });
}

void ModeManager::setSearchRoots(const QStringList& roots) {
  searchRoots_.clear();
  for (const auto& r : roots) {
    QDir d(r);
    if (d.exists()) searchRoots_ << d.absolutePath();
  }
  refreshModes();
  // pick first mode if none selected
  if (currentMode_.isEmpty() && !modes_.isEmpty())
    setMode(modes_[5]);
}

void ModeManager::refreshModes() {
  modes_.clear();
  for (const auto& root : searchRoots_) {
    QDir d(root);
    const QFileInfoList dirs = d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& fi : dirs) {
      const QString name = fi.fileName();
      if (!modes_.contains(name))
        modes_ << name;
    }
  }
}

QStringList ModeManager::listModes() const { return modes_; }

bool ModeManager::setMode(const QString& name) {
  if (!modes_.contains(name)) return false;
  if (!loadFramesForMode(name)) return false;
  currentMode_ = name;
  index_ = 0;
  emit modeChanged(currentMode_);
  emit frameChanged(index_);
  return true;
}

void ModeManager::nextFrame() {
  if (frames_.isEmpty()) return;
  index_ = (index_ + 1) % frames_.size();
  emit frameChanged(index_);
}

QImage ModeManager::currentImage() const {
  if (frames_.isEmpty() || index_ < 0 || index_ >= frames_.size()) return QImage();
  return QImage(frames_.at(index_));
}

bool ModeManager::loadFramesForMode(const QString& name) {
  // look across roots; first root that has this mode wins
  for (const auto& root : searchRoots_) {
    const QString modeDir = root + "/" + name;
    QDir d(modeDir);
    if (!d.exists()) continue;
    frames_ = findPngs(modeDir);
    return true; // even if empty; then CharacterView will show placeholder
  }
  frames_.clear();
  return false;
}

QStringList ModeManager::findPngs(const QString& dir) {
  QDir d(dir);
  QStringList pngs = d.entryList(QStringList() << "*.png" << "*.PNG",
                                 QDir::Files, QDir::Name);
  QStringList out;
  out.reserve(pngs.size());
  for (const auto& p : pngs) out << d.absoluteFilePath(p);
  return out;
}
