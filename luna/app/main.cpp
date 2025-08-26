#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include "../ui/MainWindow.h"
#include <QLockFile>

static void loadQss(QApplication& app) {
  // try a few common locations: beside the exe (copied by CMake), or source tree
  const QStringList candidates = {
    QCoreApplication::applicationDirPath() + "/app/style.qss",
    QDir::currentPath() + "/app/style.qss"
  };
  
  for (const auto& path : candidates) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      const QString qss = QString::fromUtf8(f.readAll());
      app.setStyleSheet(qss);
      return;
    }
  }
  // no stylesheet found → fine; app still runs
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("luna-sama");
  QApplication::setOrganizationName("nana14");

  // loadQss(app);
  // Lock file in temp dir, unique name for your app
    QLockFile lockFile(QDir::temp().absoluteFilePath("luna_sama.lock"));
    lockFile.setStaleLockTime(0);  // never auto-release
    if (!lockFile.tryLock()) {
        // Already running
        qWarning("Another instance is already running.");
        return 0;
    }

  MainWindow w;
  w.show();
  return app.exec();
}
