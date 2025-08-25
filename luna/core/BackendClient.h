// BackendClient.h

/*
  Interface; emits finished (text, audioPath)
*/

#pragma once
#include <QObject>
#include <QUrl>

class BackendClient : public QObject {
  Q_OBJECT
public:
  explicit BackendClient(QObject* parent=nullptr) : QObject(parent) {}
  virtual ~BackendClient() = default;
  virtual void submit(const QString& userText) = 0;

signals:
  void success(const QString& responseText, const QUrl& audioUrlOrPath, int voiceMs);
  void error(const QString& message);
};
