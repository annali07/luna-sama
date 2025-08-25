/*

Paints current PNG with QPainter (no square—PNG alpha shows through).

Left-click: ModeManager::nextFrame() (or shuffle within the set).

Right-click: shows ModeMenu positioned near cursor.

Optional hover animation or slight bob (QPropertyAnimation) later. (what animatios are available?)

*/

#include "CharacterView.h"
#include "../core/ModeManager.h"
#include <QPainter>
#include <QMouseEvent>

CharacterView::CharacterView(ModeManager* modes, QWidget* parent)
  : QWidget(parent), modes_(modes)
{
  setAttribute(Qt::WA_TranslucentBackground, true);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  // repaint when mode or frame changes
  connect(modes_, &ModeManager::frameChanged,   this, [this](int){ updateFromManager(); });
  connect(modes_, &ModeManager::modeChanged,    this, [this](const QString&){ updateFromManager(); });
}

void CharacterView::updateFromManager() { update(); }

void CharacterView::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

  const QImage img = modes_->currentImage();
  if (!img.isNull()) {
    const QSize tgt = img.size().scaled(size(), Qt::KeepAspectRatio);
    const QRect r((width()-tgt.width())/2, (height()-tgt.height())/2, tgt.width(), tgt.height());
    p.drawImage(r, img);
  } else {
    // placeholder
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,60));
    p.drawEllipse(QRect(width()/2 - 80, height()-40, 160, 24));
    p.setBrush(QColor(255,255,255,180));
    p.drawRoundedRect(QRect(width()/2 - 60, height()/2 - 100, 120, 160), 20, 20);
  }
}

void CharacterView::mousePressEvent(QMouseEvent* ev) {
  if (ev->button() == Qt::LeftButton) {
    emit leftClicked();
    modes_->nextFrame();         // cycle emotion
  } else if (ev->button() == Qt::RightButton) {
    emit rightClicked();
  }
}
