// ModeManager.h

/*
  Loads modes (different dresses) & PNG lists; current index; signals
*/

#pragma once
#include <QObject>
#include <QImage>
#include <QStringList>

class ModeManager : public QObject {
  Q_OBJECT
public:
  explicit ModeManager(QObject* parent=nullptr);

  // Where to look for modes/<mode>/ *.png (first hit wins)
  void setSearchRoots(const QStringList& roots);

  // Discover folder names under modes/
  QStringList listModes() const;

  // Select a clothing mode (folder name). Resets to first emotion.
  bool setMode(const QString& name);

  QString currentMode() const { return currentMode_; }

  // Emotions = frames = pngs in current mode folder
  void nextFrame();                      // left click
  int  frameCount() const { return frames_.size(); }
  int  currentIndex() const { return index_; }

  // Image to paint (empty if none)
  QImage currentImage() const;

signals:
  void modeChanged(const QString& name);
  void frameChanged(int index);

private:
  QStringList searchRoots_;  // e.g. <exe>/ui/assets/modes, <cwd>/ui/assets/modes
  QStringList modes_;        // discovered clothing folders
  QString currentMode_;
  QStringList frames_;       // absolute filepaths to pngs in current mode
  int index_ = 0;

  void refreshModes();
  bool loadFramesForMode(const QString& name);
  static QStringList findPngs(const QString& dir);
};

