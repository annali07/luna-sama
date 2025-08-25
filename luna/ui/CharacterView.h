// CharacterView.h

/*
  QWidget that paints PNG, handles clicks
*/

#pragma once
#include <QWidget>
#include <QImage>
#include <QRect>
#include <QSize>

class ModeManager;

class CharacterView : public QWidget {
  Q_OBJECT
public:
  explicit CharacterView(ModeManager* modes, QWidget* parent=nullptr);

  void  setScale(qreal s);
  qreal scale() const { return scale_; }

  QSize sizeHint() const override;
  QRect imageRect() const;                 // where the sprite is drawn

signals:
  void leftClicked();
  void rightClicked();

protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent* ev) override;

private:
  ModeManager* modes_;
  qreal        scale_ = 1.0;

  void updateFromManager();
};
