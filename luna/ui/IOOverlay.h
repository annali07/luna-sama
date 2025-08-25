// IOOverlay.h

/*
  Transparent input/output box (QLineEdit + Display)
*/

#pragma once
#include <QWidget>

class QTextEdit;            // <— change from QLineEdit
class QLabel;

class IOOverlay : public QWidget {
  Q_OBJECT
public:
  explicit IOOverlay(QWidget* parent=nullptr);

  void setBounds(const QRect& r);
  void setNames(const QString& userName, const QString& charName);
  void showStatus(const QString& text);
  void showOutput(const QString& text);
  void backToInputMode();

signals:
  void submitted(const QString& userText);

protected:
  void paintEvent(QPaintEvent*) override;
  void resizeEvent(QResizeEvent*) override;
  bool eventFilter(QObject* obj, QEvent* ev) override;   // <— to catch Enter

private:
  QTextEdit* edit_  = nullptr;   // <— change type
  QLabel*    header_= nullptr;
  QLabel*    body_  = nullptr;

  QString userName_ = QString::fromUtf8("あなた");
  QString charName_ = QString::fromUtf8("桜小路ルナ");

  void toInput();
  void toOutput(const QString& text);
  void layoutChildren();
  void updateFonts();
};
