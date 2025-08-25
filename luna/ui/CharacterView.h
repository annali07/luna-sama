// CharacterView.h

/*
  QWidget that paints PNG, handles clicks
*/

#pragma once
#include <QWidget>
#include <QImage>

class ModeManager;

class CharacterView : public QWidget {
  Q_OBJECT
public:
  explicit CharacterView(ModeManager* modes, QWidget* parent=nullptr);
  QSize sizeHint() const override { return {320, 360}; }

signals:
  void leftClicked();
  void rightClicked();

protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent* ev) override;

private:
  ModeManager* modes_;
  void updateFromManager();   // just calls update()
};

