/*

Flags: Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint

Attr: setAttribute(Qt::WA_TranslucentBackground, true)

Layout: CharacterView stacked above IOOverlay (with margin so textbox sits just below the PNG).

Snap to corner on first run; Draggable by left-button drag on anywhere of the computer display screen.

*/

#include "MainWindow.h"
#include "CharacterView.h"
#include "IOOverlay.h"
#include "../core/ModeManager.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QTimer>
#include <QCursor>
#include <QStyleHints>
#include <QActionGroup>

MainWindow::MainWindow(QWidget* parent)
  : QWidget(parent)
{
  modes_     = new ModeManager(this);
  character_ = new CharacterView(modes_, this);
  io_        = new IOOverlay(this);

  applyWindowFlags();
  buildUi();
  connectSignals();

  // enable drag by grabbing the sprite
  character_->setCursor(Qt::OpenHandCursor);
  character_->installEventFilter(this);
  this->installEventFilter(this);

  // optional: choose a default clothing
  // modes_->setMode("casual");
}

void MainWindow::applyWindowFlags() {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground, true);
}

void MainWindow::buildUi() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,8);
  layout->setSpacing(4);

  layout->addWidget(character_, 0, Qt::AlignHCenter | Qt::AlignBottom);
  layout->addWidget(io_,        0, Qt::AlignHCenter | Qt::AlignTop);
  setLayout(layout);

  resize(320, 420);
}

void MainWindow::connectSignals() {
  connect(character_, &CharacterView::rightClicked, this, [this](){
    showContextMenu(QCursor::pos());
  });

  // demo IO flow (replace with BackendClient later)
  connect(io_, &IOOverlay::submitted, this, [this](const QString& text){
    io_->showOutput(QStringLiteral("LUNA: %1").arg(text));
    QTimer::singleShot(1200, this, [this](){ io_->backToInputMode(); });
  });
}

void MainWindow::showEvent(QShowEvent* e) {
  QWidget::showEvent(e);
  const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
  const QSize sz = size();
  move(avail.right() - sz.width() - 24, avail.bottom() - sz.height() - 24);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  if (obj == this || obj == character_) {
    switch (ev->type()) {
      case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::LeftButton) {
          dragging_ = true;
          draggingStarted_ = false;
          dragOffset_ = me->globalPosition().toPoint() - frameGeometry().topLeft();
          character_->setCursor(Qt::ClosedHandCursor);
          return false; // allow CharacterView click if we don't drag
        }
        break;
      }
      case QEvent::MouseMove: {
        if (dragging_) {
          auto *me = static_cast<QMouseEvent*>(ev);
          const QPoint cur = me->globalPosition().toPoint();
          const int thresh = QGuiApplication::styleHints()->startDragDistance();
          if (!draggingStarted_) {
            const QPoint delta = cur - (dragOffset_ + frameGeometry().topLeft());
            if (delta.manhattanLength() < thresh) return false;
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
          return true; // eat release after a drag
        }
        break;
      }
      default: break;
    }
  }
  return QWidget::eventFilter(obj, ev);
}

void MainWindow::showContextMenu(const QPoint& globalPos) {
  QMenu menu;
  QMenu* modesMenu = menu.addMenu("Change Mode");
  populateModesMenu(modesMenu);

  menu.addSeparator();
  menu.addAction("Close", [](){ qApp->quit(); });

  menu.exec(globalPos);
}

void MainWindow::populateModesMenu(QMenu* menu) {
  menu->clear();
  QActionGroup* group = new QActionGroup(menu);
  group->setExclusive(true);

  const QStringList names = modes_->listModes();
  const QString current   = modes_->currentMode();

  for (const QString& name : names) {
    QAction* act = menu->addAction(name);
    act->setCheckable(true);
    act->setChecked(name == current);
    group->addAction(act);
    connect(act, &QAction::triggered, this, [this, name](){
      modes_->setMode(name);
    });
  }
}
