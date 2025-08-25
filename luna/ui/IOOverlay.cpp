/*

IOOverlay (transparent textbox)

Two states:
  Input mode: QLineEdit visible, placeholderText="Type and press Enter…".
  Output mode: QLabel overlay (semi-transparent) shows LLM text; input disabled.

Style:
  QSS: transparent background, white text with drop shadow, rounded subtle border when focused.

Flow:
  On Enter: emit submitted(text), clear line edit, switch to “status” display ("LUNA …" or a subtle spinner), disable input.
  When backend returns: set display to LLM text, keep disabled.
  AudioPlayer emits finished: hide display, re-enable input.

*/
#include "IOOverlay.h"
#include <QTextEdit>
#include <QLabel>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QKeyEvent>
#include <algorithm>

IOOverlay::IOOverlay(QWidget* parent)
  : QWidget(parent),
    edit_(new QTextEdit(this)),
    header_(new QLabel(this)),
    body_(new QLabel(this))
{
  setAttribute(Qt::WA_TranslucentBackground, true);

  // Header (the fixed top line)
  header_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  header_->setStyleSheet("color: white;");

  // Input (multi-line, wrapping, borderless, no scrollbars)
  edit_->setAcceptRichText(false);
  edit_->setWordWrapMode(QTextOption::WordWrap);                    // wrap
  edit_->setLineWrapMode(QTextEdit::WidgetWidth);                   // wrap to width
  edit_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  edit_->setFrameStyle(QFrame::NoFrame);
  edit_->viewport()->setAutoFillBackground(false);
  edit_->setStyleSheet("background: transparent; color: white;");
  edit_->setPlaceholderText("Type and press Enter…");
  edit_->installEventFilter(this);                                  // handle Enter

  // Output body (read-only text)
  body_->setVisible(false);
  body_->setWordWrap(true);
  body_->setStyleSheet("color: white;");

  toInput();
}

bool IOOverlay::eventFilter(QObject* obj, QEvent* ev) {
  if (obj == edit_ && ev->type() == QEvent::KeyPress) {
    auto *ke = static_cast<QKeyEvent*>(ev);
    const bool justEnter = (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
                           && !(ke->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier));
    if (justEnter) {
      const QString t = edit_->toPlainText().trimmed();
      if (!t.isEmpty()) {
        emit submitted(t);
        toOutput(QString::fromUtf8("…"));
      }
      return true;  // consume
    }
    // Shift+Enter inserts newline (default)
  }
  return QWidget::eventFilter(obj, ev);
}

void IOOverlay::setNames(const QString& userName, const QString& charName) {
  userName_ = userName;
  charName_ = charName;
  header_->setText(QStringLiteral("【%1】").arg(edit_->isEnabled()? userName_ : charName_));
}

void IOOverlay::setBounds(const QRect& r) {
  setGeometry(r);
  layoutChildren();
  updateFonts();
  update();
}

void IOOverlay::showStatus(const QString& text) { toOutput(text); }
void IOOverlay::showOutput(const QString& text) { toOutput(text); }
void IOOverlay::backToInputMode()               { toInput();      }

void IOOverlay::toInput() {
  body_->setVisible(false);
  edit_->setEnabled(true);
  edit_->setVisible(true);
  header_->setText(QStringLiteral("【%1】").arg(userName_));
  edit_->clear();
  edit_->setFocus();
  layoutChildren();
  update();
}

void IOOverlay::toOutput(const QString& text) {
  edit_->setEnabled(false);
  edit_->setVisible(false);
  body_->setVisible(true);
  body_->setText(text);
  header_->setText(QStringLiteral("【%1】").arg(charName_));
  layoutChildren();
  update();
}

void IOOverlay::layoutChildren() {
  const int w = width();
  const int h = height();

  const int pad = std::max(6, h/20);
  const int headerH = std::max(18, h/5);                  // taller header to avoid clipping
  header_->setGeometry(pad, pad, w - 2*pad, headerH);

  const int contentTop = pad + headerH;
  const int contentH   = std::max(12, h - contentTop - pad);

  if (edit_->isVisible())
    edit_->setGeometry(pad, contentTop, w - 2*pad, contentH);
  if (body_->isVisible())
    body_->setGeometry(pad, contentTop, w - 2*pad, contentH);
}

void IOOverlay::updateFonts() {
  const int h = height();
  const int headerH = std::max(18, h/5);
  QFont fh = header_->font(); fh.setPixelSize(std::max(12, (headerH * 60)/100)); header_->setFont(fh);

  QFont fc = edit_->font();   fc.setPixelSize(std::max(12, (h * 12)/100));       // ~12% of panel height
  edit_->setFont(fc);
  body_->setFont(fc);
}

void IOOverlay::paintEvent(QPaintEvent*) {
  // Transparent panel — comment this block if you want no panel at all
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  const int rad = std::max(8, height()/10);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0,0,0,90));          // translucent dark
  p.drawRoundedRect(rect(), rad, rad);
}

void IOOverlay::resizeEvent(QResizeEvent*) { layoutChildren(); updateFonts(); }
