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
class QMenu;

class MainWindow : public QWidget {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);

protected:
  void showEvent(QShowEvent* e) override;
  bool eventFilter(QObject* obj, QEvent* ev) override;

private:
  // Core widgets
  ModeManager*   modes_      = nullptr;
  CharacterView* character_  = nullptr;
  IOOverlay*     io_         = nullptr;

  // Drag state
  bool   dragging_        = false;
  bool   draggingStarted_ = false;
  QPoint dragOffset_;
  Qt::KeyboardModifier dragMod_ = Qt::AltModifier;   // configurable

  // Behavior
  void applyScale(qreal s);
  void applyWindowFlags();
  void connectSignals();
  void showContextMenu(const QPoint& globalPos);
  void populateModesMenu(QMenu* menu);
  void populateDragBindingMenu(QMenu* menu);
  void updateIoGeometry();       // place IOOverlay over bottom 40% of sprite
  void syncWindowToSprite();     // window size == sprite size; keep bottom-right
  void setDragModifier(Qt::KeyboardModifier mod, bool persist = true);
};
