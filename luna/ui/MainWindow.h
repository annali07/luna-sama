// MainWindow.h

/*
  Frameless, translucent, always-on-top main window 
*/

#pragma once
#include <QWidget>
#include <QPoint>

class CharacterView;
class IOOverlay;
class ModeManager;

class MainWindow : public QWidget {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);

protected:
  void showEvent(QShowEvent* e) override;
  bool eventFilter(QObject* obj, QEvent* ev) override;

private:
  ModeManager*   modes_ = nullptr;
  CharacterView* character_ = nullptr;
  IOOverlay*     io_ = nullptr;

  // drag state
  bool   dragging_ = false;
  bool   draggingStarted_ = false;
  QPoint dragOffset_;

  // configurable drag modifier (Alt by default)
  Qt::KeyboardModifier dragMod_ = Qt::AltModifier;
  void setDragModifier(Qt::KeyboardModifier mod, bool persist = true);
  Qt::KeyboardModifier dragModifier() const { return dragMod_; }

  void applyWindowFlags();
  void buildUi();
  void connectSignals();
  void showContextMenu(const QPoint& globalPos);
  void populateModesMenu(class QMenu* menu);
  void populateDragBindingMenu(class QMenu* menu);   // <--- NEW
};
