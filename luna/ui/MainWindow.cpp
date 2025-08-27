/*

Flags: Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint

Attr: setAttribute(Qt::WA_TranslucentBackground, true)

Layout: CharacterView stacked above IOOverlay (with margin so textbox sits just below the PNG).

Snap to corner on first run; Draggable by left-button drag on anywhere of the computer display screen.

*/

/*
  Frameless, translucent, always-on-top pet window.
  CharacterView is in the layout; IOOverlay is an absolute overlay bound to
  the bottom 40% of the sprite's drawn rect. Alt+Left to drag (rebindable).
*/

#include "MainWindow.h"
#include "CharacterView.h"
#include "IOOverlay.h"
#include "../core/ModeManager.h"
#include "../core/BackendClient.h"
#include "../core/AudioPlayer.h"

#include <QAbstractScrollArea>

#include <algorithm>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>

#include <QWheelEvent>
#include <QtMath>
#include <QVBoxLayout>
#include <QMenu>
#include <QActionGroup>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QTimer>
#include <QCursor>
#include <QStyleHints>
#include <QSettings>
#include <functional>

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

#define SMIRK_PROB 60
#define TEXT_WAIT 4000


// tiny helpers to persist the drag modifier
static QString modToKey(Qt::KeyboardModifier m) {
  switch (m) {
    case Qt::AltModifier:    return "Alt";
    case Qt::ControlModifier:return "Ctrl";
    case Qt::ShiftModifier:  return "Shift";
    case Qt::NoModifier:     return "None";
    default:                 return "Alt";
  }
}
static Qt::KeyboardModifier keyToMod(const QString& k) {
  if (k == "Alt")   return Qt::AltModifier;
  if (k == "Ctrl")  return Qt::ControlModifier;
  if (k == "Shift") return Qt::ShiftModifier;
  if (k == "None")  return Qt::NoModifier;
  return Qt::AltModifier;
}

// keep window anchored to bottom-right while resizing
static void keepBottomRightAnchor(QWidget* w, std::function<void()> doResize) {
  const QPoint br = w->frameGeometry().bottomRight();
  doResize();
  w->adjustSize();
  const QRect fr = w->frameGeometry();
  w->move(br - QPoint(fr.width()-1, fr.height()-1));
}
MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
  // 1) Window flags & transparent background
  applyWindowFlags();
  setObjectName("MainRoot"); // QSS: #MainRoot { background: transparent; }

  // 2) Core widgets
  modes_     = new ModeManager(this);
  character_ = new CharacterView(modes_, this);
  audio_ = new AudioPlayer(this);
  emoCtrl_   = new EmotionSpriteController(modes_, this);  // loads summary.json automatically
  io_        = new IOOverlay(this);
  io_->setNames(QString::fromUtf8("NANA"), QString::fromUtf8("桜小路ルナ"));
  io_->raise(); // overlay on top

  // 3) Layout: the window should size exactly to the sprite; no margins
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);
  layout->addWidget(character_, 0, Qt::AlignCenter);
  setLayout(layout);
  character_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // 4) Load settings (drag modifier + initial scale)
  dragMod_ = keyToMod(QSettings().value("dragModifier", "Alt").toString());
  {
    const qreal s = QSettings().value("uiScale", 1.0).toDouble();
    character_->setScale(s);            // 50%..100% clamp happens inside setScale
  }

  // 5) Event filters (just once)
  character_->setCursor(Qt::OpenHandCursor);
  qApp->installEventFilter(this);       // catch Alt+wheel even on overlay/editor
  this->installEventFilter(this);
  character_->installEventFilter(this);
  io_->installEventFilter(this);        // optional but fine


  // fading effect
  // --- Idle fade wiring ---
  character_->setAttribute(Qt::WA_Hover, true);
  io_->setAttribute(Qt::WA_Hover, true);

  // Opacity effect on the character (sprite only)
  charOpacity_ = new QGraphicsOpacityEffect(character_);
  charOpacity_->setOpacity(1.0);
  character_->setGraphicsEffect(charOpacity_);

  // Animation for smooth fades
  fadeAnim_ = new QPropertyAnimation(charOpacity_, "opacity", this);
  fadeAnim_->setDuration(220);
  fadeAnim_->setEasingCurve(QEasingCurve::InOutQuad);

  // 10s idle timer
  idleTimer_ = new QTimer(this);
  idleTimer_->setSingleShot(true);
  idleTimer_->setInterval(10'000);
  connect(idleTimer_, &QTimer::timeout, this, [this]{
    faded_ = true;
    io_->setVisible(false);        // textbox disappears
    fadeTo(0.5);                   // sprite to 50%
  });










  // 6) Signals
  connectSignals();

  // 7) First layout pass after everything exists
  QTimer::singleShot(0, this, [this]{ syncWindowToSprite(); });
}


void MainWindow::applyWindowFlags() {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground, true);
}

void MainWindow::connectSignals() {
  connect(character_, &CharacterView::rightClicked, this, [this]{
    showContextMenu(QCursor::pos());
  });

  // once in ctor:
  backend_ = new BackendClient(this);
  backend_->setLlmBaseUrl(QUrl(QStringLiteral("http://127.0.0.1:8000"))); // your FastAPI LLM
  backend_->setTtsBaseUrl(QUrl(QStringLiteral("http://127.0.0.1:9880"))); // your SoVITS
  backend_->setTextLang(QStringLiteral("ja"));

  connect(io_, &IOOverlay::submitted, this, [this](const QString& text){
    // IOOverlay already switched to "…" (output mode, disabled)
    backend_->submit(text);
  });

  connect(backend_, &BackendClient::status, this, [this](const QString& s){
    io_->showStatus(s);          // keep showing "LUNA …"
  });

  connect(backend_, &BackendClient::ready, this, [this](const BackendResult& r){
    io_->showOutput(r.echoText);

    bool audioStarted = false;
    if (r.audioUrl.isValid())      { audio_->play(r.audioUrl);      audioStarted = true; }
    else if (r.localFile.isValid()){ audio_->play(r.localFile);     audioStarted = true; }

    startReenableGate(audioStarted);
  });

  connect(backend_, &BackendClient::ready, this, [this](const BackendResult& r){
    io_->showOutput(r.echoText);

    if (!r.emotion.isEmpty())
      emoCtrl_->applyEmotion(r.emotion.trimmed());

    bool audioStarted = false;
    if (r.audioUrl.isValid())      { audio_->play(r.audioUrl);      audioStarted = true; }
    else if (r.localFile.isValid()){ audio_->play(r.localFile);     audioStarted = true; }

    startReenableGate(audioStarted);
  });

  connect(backend_, &BackendClient::emotionAvailable,
        emoCtrl_,  &EmotionSpriteController::applyEmotion);


  // keep your existing gating connection; add a second connection for the smirk:
  connect(audio_, &AudioPlayer::finished, this, [this]{
    if (emoCtrl_) emoCtrl_->maybeSmirk(SMIRK_PROB);  // 30% chance
  });

  connect(emoCtrl_, &EmotionSpriteController::frameChosen, this, [this](const QString& p){
    // qDebug() << "[ui] face path =" << p;
    // Or surface it briefly in the overlay:
    // io_->showStatus(QStringLiteral("face → %1").arg(p));
  });

  connect(backend_, &BackendClient::emotionAvailable,
        emoCtrl_,   &EmotionSpriteController::applyEmotion);



  // backend end

  // errors
  connect(audio_, &AudioPlayer::error, this, [this](const QString& e){
    io_->showStatus(QStringLiteral("⚠ %1").arg(e));
    // Treat as audio finished; keep 3s minimum if not elapsed yet
    gateAudioDone_ = true;
    // wait 3 seconds
    QTimer::singleShot(TEXT_WAIT, this, [this]{
      finishGateNow();
    });
  });

  connect(backend_, &BackendClient::error, this, [this](const QString& e){
    io_->showStatus(QStringLiteral("⚠ %1").arg(e));
    // No audio will play; just fall back to 3s timer (already running with audioStarted=false)
    gateAudioDone_ = true;
    // wait 3 seconds
    QTimer::singleShot(TEXT_WAIT, this, [this]{
      finishGateNow();
    });
  });


  // keep overlay glued to sprite; also resize window to the sprite
  connect(modes_, &ModeManager::modeChanged,  this, [this](const QString&){ syncWindowToSprite(); });


  connect(modes_, &ModeManager::frameChanged, this, [this](int){            syncWindowToSprite(); });
}

void MainWindow::showEvent(QShowEvent* e) {
  QWidget::showEvent(e);
  // start bottom-right of primary screen
  const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
  move(avail.bottomRight() - QPoint(width()+24, height()+24));
  syncWindowToSprite();
}


void MainWindow::applyScale(qreal s) {
  QSettings().setValue("uiScale", s);
  // qDebug() << "APPLY SCALE ->" << s;

  keepBottomRightAnchor(this, [this, s]{
    character_->setScale(s);              // update view scale
    character_->adjustSize();             // adopt new sizeHint
    setFixedSize(character_->sizeHint()); // window == sprite size
  });

  updateIoGeometry();                     // textbox follows PNG
}


bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  // Identify where the event happened
  QWidget* w = qobject_cast<QWidget*>(obj);
  const bool onSelf      = (obj == this);
  const bool onCharacter = (obj == character_);
  const bool onOverlay   = (w && (w == io_ || io_->isAncestorOf(w)));


  // Any activity inside the pet window cancels idle and restores if faded
  if ((onSelf || onCharacter || onOverlay) &&
      (ev->type() == QEvent::Enter || ev->type() == QEvent::MouseMove)) {
    cancelIdleFadeAndRestore();
  }

  // When the mouse leaves the pet window, start the 10s idle countdown
  if (onSelf && ev->type() == QEvent::Leave) {
    scheduleIdleFade();
  }

  // ---------- Alt + Wheel = zoom (handles mouse & touchpad) ----------
  // ---- Alt + Wheel = zoom (50%..100%), works for vertical or horizontal wheels/touchpads ----
  if (ev->type() == QEvent::Wheel && (onCharacter || onOverlay)) {
    auto* we = static_cast<QWheelEvent*>(ev);
    const Qt::KeyboardModifiers mods = we->modifiers() | QGuiApplication::keyboardModifiers();
    if (mods & Qt::AltModifier) {

      // pick the dominant axis and keep its SIGN
      auto dominant = [](const QPoint& d)->int {
        return (std::abs(d.y()) >= std::abs(d.x())) ? d.y() : d.x();
      };

      // direction: +1 grow, -1 shrink
      int dir = 0;
      if (!we->angleDelta().isNull()) {
        int d = dominant(we->angleDelta());
        dir = (d > 0) ? +1 : (d < 0 ? -1 : 0);
      } else if (!we->pixelDelta().isNull()) {
        int d = dominant(we->pixelDelta());
        dir = (d > 0) ? +1 : (d < 0 ? -1 : 0);
      }

      if (dir != 0) {
        // one notch = 5% change; flip sign for horizontal if you prefer:
        // bool invertHorizontal = false;  // set true if you want the opposite feel
        // if (!we->angleDelta().isNull() && std::abs(we->angleDelta().x()) > std::abs(we->angleDelta().y()))
        //   dir = invertHorizontal ? dir : -dir;

        qreal s = character_->scale() + dir * 0.05;
        s = std::clamp<qreal>(s, 0.5, 1.0);
        applyScale(s);
      }

      we->accept();
      return true;
    }
  }


  // ---------- Keyboard fallback for testing: [ smaller, ] larger ----------
  if (ev->type() == QEvent::KeyPress && (onSelf || onCharacter || onOverlay)) {
    auto* ke = static_cast<QKeyEvent*>(ev);
    if (ke->key() == Qt::Key_BracketLeft) {
      applyScale(std::max<qreal>(0.5, character_->scale() - 0.05));
      return true;
    }
    if (ke->key() == Qt::Key_BracketRight) {
      applyScale(std::min<qreal>(1.0, character_->scale() + 0.05));
      return true;
    }
  }



  // Drag with configured modifier (Alt/Ctrl/Shift/None)
  if (onSelf || onCharacter) {
    switch (ev->type()) {
      case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::LeftButton) {
          const bool wantDrag = (dragMod_ == Qt::NoModifier) || (me->modifiers() & dragMod_);
          if (wantDrag) {
            dragging_ = true;
            draggingStarted_ = false;
            dragOffset_ = me->globalPosition().toPoint() - frameGeometry().topLeft();
            character_->setCursor(Qt::ClosedHandCursor);
            return true; // consume (don’t advance emotion while dragging)
          }
        }
        break;
      }
      case QEvent::MouseMove: {
        if (dragging_) {
          auto* me = static_cast<QMouseEvent*>(ev);
          const QPoint cur = me->globalPosition().toPoint();
          const int thresh = QGuiApplication::styleHints()->startDragDistance();
          if (!draggingStarted_) {
            const QPoint delta = cur - (dragOffset_ + frameGeometry().topLeft());
            if (delta.manhattanLength() < thresh) return true;
            draggingStarted_ = true;
          }
          move(cur - dragOffset_);
          return true;
        }
        break;
      }
      case QEvent::MouseButtonRelease: {
        if (dragging_) {
          dragging_ = false;
          draggingStarted_ = false;
          character_->setCursor(Qt::OpenHandCursor);
          return true;
        }
        break;
      }
      default: break;
    }
  }

  if (onCharacter && ev->type() == QEvent::Resize) {
    updateIoGeometry();
  }

  return QWidget::eventFilter(obj, ev);
}

void MainWindow::showContextMenu(const QPoint& globalPos) {
  QMenu menu;
  auto* modesMenu = menu.addMenu("Change Mode");
  populateModesMenu(modesMenu);

  auto* dragMenu  = menu.addMenu("Drag Binding");
  populateDragBindingMenu(dragMenu);

  menu.addSeparator();
  menu.addAction("Close", []{ qApp->quit(); });
  menu.exec(globalPos);
}

void MainWindow::populateModesMenu(QMenu* menu) {
  menu->clear();
  QActionGroup* group = new QActionGroup(menu);
  group->setExclusive(true);

  const QStringList names = modes_->listModes();
  const QString current   = modes_->currentMode();
  if (names.isEmpty()) {
    auto* a = menu->addAction("No modes found (ui/assets/modes/<folder>/pngs)");
    a->setEnabled(false);
    return;
  }
  for (const QString& name : names) {
    QAction* act = menu->addAction(name);
    act->setCheckable(true);
    act->setChecked(name == current);
    group->addAction(act);
    connect(act, &QAction::triggered, this, [this, name]{ modes_->setMode(name); });
  }
}

void MainWindow::populateDragBindingMenu(QMenu* m) {
  m->clear();
  QActionGroup* g = new QActionGroup(m);
  g->setExclusive(true);
  struct Opt { const char* label; Qt::KeyboardModifier mod; };
  const Opt opts[] = { {"Alt",Qt::AltModifier}, {"Ctrl",Qt::ControlModifier},
                       {"Shift",Qt::ShiftModifier}, {"None",Qt::NoModifier} };
  for (const auto& o : opts) {
    QAction* a = m->addAction(o.label);
    a->setCheckable(true);
    a->setChecked(dragMod_ == o.mod);
    g->addAction(a);
    connect(a, &QAction::triggered, this, [this, o]{ setDragModifier(o.mod, true); });
  }
}

void MainWindow::setDragModifier(Qt::KeyboardModifier mod, bool persist) {
  dragMod_ = mod;
  if (persist) QSettings().setValue("dragModifier", modToKey(mod));
}

void MainWindow::syncWindowToSprite() {
  keepBottomRightAnchor(this, [this]{
    character_->adjustSize();
    setFixedSize(character_->sizeHint());    // window = sprite size
  });
  updateIoGeometry();
}

void MainWindow::updateIoGeometry() {
  const QRect imgLocal = character_->imageRect();
  if (imgLocal.isEmpty()) { io_->setBounds(QRect()); return; }

  const QPoint tl = character_->mapTo(this, imgLocal.topLeft());
  const QRect  img(tl, imgLocal.size());      // scaled PNG rect in MainWindow coords

  constexpr double kWidthRatio  = 0.40;       // 50% of PNG width
  constexpr double kHeightRatio = 0.25;       // about 25% of PNG height for the panel
  constexpr int    kMinH        = 60;

  const int w = std::max(1, int(img.width()  * kWidthRatio));
  const int h = std::max(kMinH, int(img.height() * kHeightRatio));

  const int x = img.left() + (img.width() - w) / 2;   // center horizontally
  const int y = img.bottom() - h+5;                 // anchor to bottom

  QRect box(x, y, w, h);
  box.adjust(0, 4, 0, -4);                            // tiny vertical breathing room

  io_->setBounds(box);
}

void MainWindow::fadeTo(qreal target) {
  if (!charOpacity_) return;
  fadeAnim_->stop();
  fadeAnim_->setStartValue(charOpacity_->opacity());
  fadeAnim_->setEndValue(target);
  fadeAnim_->start();
}

void MainWindow::scheduleIdleFade() {
  if (!idleTimer_) return;
  if (!idleTimer_->isActive())
    idleTimer_->start();
}

void MainWindow::cancelIdleFadeAndRestore() {
  if (!idleTimer_) return;
  idleTimer_->stop();
  if (faded_) {
    faded_ = false;
    io_->setVisible(true);   // textbox reappears
    fadeTo(1.0);             // sprite back to full opacity
  }
}


void MainWindow::finishGateNow() {
  if (gateTimer_) { gateTimer_->stop(); gateTimer_->deleteLater(); gateTimer_ = nullptr; }
  if (gateConn_)  { disconnect(gateConn_); gateConn_ = {}; }
  io_->backToInputMode();
}

void MainWindow::tryFinishGate() {
  if (gateAudioDone_ && gateTimerDone_) finishGateNow();
}

void MainWindow::startReenableGate(bool audioStarted) {
  // reset previous gate
  if (gateTimer_) { gateTimer_->stop(); gateTimer_->deleteLater(); gateTimer_ = nullptr; }
  if (gateConn_)  { disconnect(gateConn_); gateConn_ = {}; }
  gateAudioDone_ = !audioStarted;   // if no audio, treat as already done
  gateTimerDone_ = false;

  // 3s minimum hold
  gateTimer_ = new QTimer(this);
  gateTimer_->setSingleShot(true);
  gateTimer_->start(TEXT_WAIT);
  connect(gateTimer_, &QTimer::timeout, this, [this]{
    gateTimerDone_ = true;
    tryFinishGate();
  });

  // audio finished (only if started)
  if (audioStarted) {
    gateConn_ = connect(audio_, &AudioPlayer::finished, this, [this]{
      gateAudioDone_ = true;
      tryFinishGate();
    });
  }
}
