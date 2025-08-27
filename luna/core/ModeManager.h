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

  void setSearchRoots(const QStringList& roots);
  QStringList listModes() const;

  bool setMode(const QString& name);
  QString currentMode() const { return currentMode_; }

  void nextFrame();
  int  frameCount() const { return frames_.size(); }
  int  currentIndex() const { return index_; }
  QImage currentImage() const;

  // --- NEW: allow external code to pick a concrete PNG as the "current frame"
  // basename = file name without extension (e.g., "lun_s_1_0_03")
  bool setFrameByBasename(const QString& basename,
                          Qt::CaseSensitivity cs = Qt::CaseInsensitive);
  // optional convenience: set by absolute file path
  bool setFrameByPath(const QString& absPath);

  // --- NEW: expose the active mode directory (for summary.json)
  QString modeDir() const { return currentModeDir_; }


    // NEW: ensure the path is in frames_ (append if missing) and select it
  bool ensureAndSetFramePath(const QString& absPath);

  // QString modeDir() const { return currentModeDir_; }

signals:
  void modeChanged(const QString& name);
  void frameChanged(int index);

private:
  QStringList searchRoots_;
  QStringList modes_;
  QString     currentMode_;
  QStringList frames_;          // absolute PNG paths
  int         index_ = 0;

  // --- NEW
  QString     currentModeDir_;

  void refreshModes();
  bool loadFramesForMode(const QString& name);
  static QStringList findPngs(const QString& dir);
};
