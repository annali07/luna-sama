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
  io_        = new IOOverlay(this);
  io_->setNames(QString::fromUtf8("あなた"), QString::fromUtf8("桜小路ルナ"));
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

  connect(io_, &IOOverlay::submitted, this, [this](const QString& text){
    io_->showStatus(QStringLiteral("…"));
    QTimer::singleShot(1200, this, [this, text]{
      io_->showOutput(QStringLiteral("LUNA: %1").arg(text));
      QTimer::singleShot(1200, this, [this]{ io_->backToInputMode(); });
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
  qDebug() << "applyScale ->" << s;

  keepBottomRightAnchor(this, [this, s]{
    character_->setScale(s);              // 1) set scale in the view
    character_->adjustSize();             // 2) let it adopt its sizeHint
    setFixedSize(character_->sizeHint()); // 3) window == sprite size
  });

  updateIoGeometry();                     // 4) move/resize the textbox
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  QWidget* w = qobject_cast<QWidget*>(obj);
  const bool onSelf      = (obj == this);
  const bool onCharacter = (obj == character_);
  const bool onOverlay   = (w && (w == io_ || io_->isAncestorOf(w)));

  // Alt + Wheel = zoom 50%..100% (works on wheel and touchpad)
  if (ev->type() == QEvent::Wheel && (onCharacter || onOverlay)) {
    auto* we = static_cast<QWheelEvent*>(ev);
    if (we->modifiers() & Qt::AltModifier) {
      // mouse wheel (angleDelta) and smooth touchpad (pixelDelta)
      qreal steps = 0.0;
      if (!we->angleDelta().isNull())      steps = we->angleDelta().y() / 120.0;
      else if (!we->pixelDelta().isNull()) steps = we->pixelDelta().y() / 60.0;

      if (steps != 0.0) {
        const qreal step = 0.05;                 // 5% per notch
        qreal s = character_->scale() + steps * step;
        s = std::clamp<qreal>(s, 0.5, 1.0);      // clamp 50%..100%
        qDebug() << "ALT-WHEEL steps" << steps << "target scale" << s;
        applyScale(s);                            // <— CALL THE HELPER
      }
      we->accept();
      return true;                                // eat Alt+wheel
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
