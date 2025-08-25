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
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>

IOOverlay::IOOverlay(QWidget* parent)
  : QWidget(parent),
    edit_(new QLineEdit(this)),
    overlay_(new QLabel(this))
{
  auto* lay = new QHBoxLayout(this);
  lay->setContentsMargins(12,0,12,0);
  lay->addWidget(edit_);
  setLayout(lay);

  edit_->setObjectName("ioEdit");                 // <-- targets style.qss
  edit_->setPlaceholderText("Type and press Enter…");
  overlay_->setVisible(false);
  overlay_->setWordWrap(true);
  overlay_->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  overlay_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

  connect(edit_, &QLineEdit::returnPressed, this, [this](){
    const QString text = edit_->text().trimmed();
    if (text.isEmpty()) return;
    emit submitted(text);
    showStatus("LUNA …");
  });
}

void IOOverlay::showStatus(const QString& text) { toOutput(text); }
void IOOverlay::showOutput(const QString& text) { toOutput(text); }
void IOOverlay::backToInputMode() { toInput(); }

void IOOverlay::toInput() {
  overlay_->setVisible(false);
  edit_->setEnabled(true);
  edit_->clear();
  edit_->setFocus();
}

void IOOverlay::toOutput(const QString& text) {
  overlay_->setText(text);
  overlay_->setVisible(true);
  edit_->setEnabled(false);
  overlay_->raise();
  overlay_->setGeometry(edit_->geometry().adjusted(6,2,-6,-2));
}
