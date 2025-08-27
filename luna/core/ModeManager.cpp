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
#include <QFileInfo>
#include <QFileInfoList>

ModeManager::ModeManager(QObject* parent) : QObject(parent) {
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
  if (currentMode_.isEmpty() && !modes_.isEmpty())
    setMode(modes_.value(5)); // safer than hard-coded [5]
}

void ModeManager::refreshModes() {
  modes_.clear();
  for (const auto& root : searchRoots_) {
    QDir d(root);
    const QFileInfoList dirs = d.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& fi : dirs) {
      const QString name = fi.fileName();
      if (!modes_.contains(name)) modes_ << name;
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
  for (const auto& root : searchRoots_) {
    const QString modeDir = root + "/" + name;
    QDir d(modeDir);
    if (!d.exists()) continue;
    currentModeDir_ = d.absolutePath();            // <-- NEW
    frames_ = findPngs(currentModeDir_);
    return true;
  }
  frames_.clear();
  currentModeDir_.clear();                         // <-- NEW
  return false;
}

QStringList ModeManager::findPngs(const QString& dir) {
  QDir d(dir);
  QStringList pngs = d.entryList({ "*.png", "*.PNG" }, QDir::Files, QDir::Name);
  QStringList out; out.reserve(pngs.size());
  for (const auto& p : pngs) out << d.absoluteFilePath(p);
  return out;
}


bool ModeManager::setFrameByBasename(const QString& basename, Qt::CaseSensitivity cs) {
  if (basename.isEmpty() || frames_.isEmpty()) return false;
  // qDebug() << "[mode] setFrameByBasename:" << basename << " total frames =" << frames_.size();

  for (int i = 0; i < frames_.size(); ++i) {
    const QFileInfo fi(frames_.at(i));
    const QString bn = fi.completeBaseName();
    if (QString::compare(bn, basename, cs) == 0) {
      // qDebug() << "   -> matched index" << i << " path =" << fi.absoluteFilePath();
      if (index_ == i) return true;
      index_ = i;
      emit frameChanged(index_);
      return true;
    }
  }
  // qDebug() << "   -> no match for basename in current frames";
  return false;
}

bool ModeManager::setFrameByPath(const QString& absPath) {
  if (absPath.isEmpty() || frames_.isEmpty()) return false;
  const int i = frames_.indexOf(absPath);
  // qDebug() << "[mode] setFrameByPath:" << absPath << " found index =" << i;
  if (i < 0) return false;
  if (index_ == i) return true;
  index_ = i;
  emit frameChanged(index_);
  return true;
}

bool ModeManager::ensureAndSetFramePath(const QString& absPath) {
  if (absPath.isEmpty()) return false;
  int i = frames_.indexOf(absPath);
  if (i < 0) {
    // qDebug() << "[mode] ensureAndSetFramePath: appending" << absPath;
    frames_ << absPath;
    i = frames_.size() - 1;
  }
  if (index_ == i) return true;
  index_ = i;
  emit frameChanged(index_);
  return true;
}