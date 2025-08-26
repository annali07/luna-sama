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
class QTimer;
class QPropertyAnimation;
class QGraphicsOpacityEffect;
class BackendClient;     // <-- add
class AudioPlayer;       // <-- add

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
  
  QTimer*                idleTimer_     = nullptr;
  QGraphicsOpacityEffect* charOpacity_  = nullptr;
  QPropertyAnimation*    fadeAnim_      = nullptr;
  bool                   faded_         = false;

  // NEW: backend + audio
  BackendClient* backend_   = nullptr;   // <-- add this
  AudioPlayer*   audio_     = nullptr;   // <-- add this

  // Drag state
  bool   dragging_        = false;
  bool   draggingStarted_ = false;
  QPoint dragOffset_;
  Qt::KeyboardModifier dragMod_ = Qt::AltModifier;   // configurable


  // MainWindow.h (private:)
  QTimer* gateTimer_ = nullptr;
  QMetaObject::Connection gateConn_;   // to disconnect AudioPlayer::finished
  void startReenableGate(bool audioStarted);
  void finishGate();                   // calls io_->backToInputMode() once



  // Fades on 10s inactivity
  void fadeTo(qreal target);                 // animate character opacity
  void scheduleIdleFade();                   // start 10s timer
  void cancelIdleFadeAndRestore();           // stop timer + restore (if faded)

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
