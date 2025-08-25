/*

Paints current PNG with QPainter (no square—PNG alpha shows through).

Left-click: ModeManager::nextFrame() (or shuffle within the set).

Right-click: shows ModeMenu positioned near cursor.

Optional hover animation or slight bob (QPropertyAnimation) later. (what animatios are available?)

*/

#include "CharacterView.h"
#include "../core/ModeManager.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>

CharacterView::CharacterView(ModeManager* modes, QWidget* parent)
  : QWidget(parent), modes_(modes)
{
  setAttribute(Qt::WA_TranslucentBackground, true);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  connect(modes_, &ModeManager::frameChanged, this, [this](int){ updateFromManager(); });
  connect(modes_, &ModeManager::modeChanged,  this, [this](const QString&){ updateFromManager(); });
}

// SINGLE definition — clamp to 50%..100%
void CharacterView::setScale(qreal s) {
  scale_ = std::clamp<qreal>(s, 0.5, 1.0);
  updateGeometry();
  update();
}

QSize CharacterView::sizeHint() const {
  const QImage img = modes_->currentImage();
  if (!img.isNull()) {
    const int w = qMax(1, qRound(img.width()  * scale_));
    const int h = qMax(1, qRound(img.height() * scale_));
    return QSize(w, h);
  }
  return QSize(qRound(320 * scale_), qRound(360 * scale_));
}

QRect CharacterView::imageRect() const {
  const QImage img = modes_->currentImage();
  if (!img.isNull()) {
    const int w = qMax(1, qRound(img.width()  * scale_));
    const int h = qMax(1, qRound(img.height() * scale_));
    const int x = (width()  - w) / 2;
    const int y = (height() - h) / 2;
    return QRect(x, y, w, h);
  }
  const QSize tgt = sizeHint();
  return QRect((width()-tgt.width())/2, (height()-tgt.height())/2, tgt.width(), tgt.height());
}

void CharacterView::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

  const QImage img = modes_->currentImage();
  const QRect r = imageRect();

  if (!img.isNull()) {
    p.drawImage(r, img);
  } else {
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,60));
    p.drawEllipse(QRect(r.center().x()-80, r.bottom()-40, 160, 24));
    p.setBrush(QColor(255,255,255,180));
    p.drawRoundedRect(QRect(r.center().x()-60, r.center().y()-100, 120, 160), 20, 20);
  }
}

void CharacterView::mousePressEvent(QMouseEvent* ev) {
  if (ev->button() == Qt::LeftButton) {
    emit leftClicked();
    modes_->nextFrame(); 
  }
    else if (ev->button() == Qt::RightButton) emit rightClicked();
}

void CharacterView::updateFromManager() {
  updateGeometry();
  update();
}
