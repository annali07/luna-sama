// IOOverlay.h

/*
  Transparent input/output box (QLineEdit + Display)
*/

#pragma once
#include <QWidget>

class QLineEdit;
class QLabel;

class IOOverlay : public QWidget {
  Q_OBJECT
public:
  explicit IOOverlay(QWidget* parent=nullptr);
  void showStatus(const QString& text);
  void showOutput(const QString& text);
  void backToInputMode();

signals:
  void submitted(const QString& userText);

private:
  QLineEdit* edit_;
  QLabel*    overlay_;
  void toInput();
  void toOutput(const QString& text);
};
