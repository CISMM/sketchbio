#include "SettingsHelpers.h"

#include <QSettings>
#include <QFile>
#include <QFileDialog>
#include <QApplication>
#include <QDebug>

namespace SettingsHelpers {

static inline QString getSettingsStringForSubprocessPath(const QString &executableName) {
    return "subprocesses/" + executableName + "/path";
}

QString getSubprocessExecutablePath(const QString &executableName) {
  QSettings settings;  // default parameters specified to the QCoreApplication
                       // at startup
  QString executablePath =
      settings.value(getSettingsStringForSubprocessPath(executableName), QString(""))
          .toString();
  if (executablePath.length() == 0 || !QFile(executablePath).exists()) {
    bool hasGui =
        (qobject_cast<QApplication *>(QCoreApplication::instance()) != 0);
    QDir appDir(QCoreApplication::applicationDirPath());
#if defined(__APPLE__) && defined(__MACH__)
    // test /Applications/appName and /usr/bin then ask
    if (QFile(appDir.filePath(executableName)).exists()) {
      executablePath = appDir.filePath(executableName);
    } else if (QFile("/Applications/" + executableName +
                     ".app/Contents/MacOS/" + executableName).exists()) {
      executablePath = "/Applications/" + executableName +
                       ".app/Contents/MacOS/" + executableName;
    } else if (QFile("/usr/bin/" + executableName).exists()) {
      executablePath = "/usr/bin/" + executableName;
    } else if (QFile("/usr/local/bin/" + executableName).exists()) {
      executablePath = "/usr/local/bin/" + executableName;
    } else {
      if (hasGui) {
        executablePath = QFileDialog::getOpenFileName(
            NULL, "Specify location of '" + executableName + "'",
            "/Applications");
        if (executablePath.endsWith(".app")) {
          executablePath = executablePath + "/Contents/MacOS/" + executableName;
        }
      } else {
        qDebug() << "Could not find \"" << executableName << "\"\n"
                 << "Please run the SketchBio GUI to input the location of "
                    "this program.";
      }
    }
#elif defined(_WIN32)
    // test default locations C:/Program Files and C:/Program Files(x86) then
    // ask for a .exe file
    if (QFile(appDir.filePath(executableName + ".exe")).exists()) {
      executablePath = appDir.filePath(executableName + ".exe");
    } else if (QFile("C:/Program Files/" + executableName + "/" +
                     executableName + ".exe").exists()) {
      executablePath =
          "C:/Program Files/" + executableName + "/" + executableName;
    } else if (QFile("C:/Program Files (x86)/" + executableName + "/" +
                     executableName + ".exe").exists()) {
      executablePath =
          "C:/Program Files (x86)/" + executableName + "/" + executableName;
    } else {
      if (hasGui) {
        executablePath = QFileDialog::getOpenFileName(
            NULL, "Specify location of '" + executableName + "'",
            "C:/Program Files", "Windows Execuatables (*.exe)");
      } else {
        qDebug() << "Could not find \"" << executableName << "\"\n"
                 << "Please run the SketchBio GUI to input the location of "
                    "this program.";
      }
    }
#elif defined(__linux__)
    // test /usr/bin then ask for the file
    if (QFile(appDir.filePath(executableName)).exists()) {
      executablePath = appDir.filePath(executableName);
    } else if (QFile("/usr/bin/" + executableName).exists()) {
      executablePath = "/usr/bin/" + executableName;
    } else if (QFile("/usr/local/bin/" + executableName).exists()) {
      executablePath = "/usr/local/bin/" + executableName;
    } else {
      if (hasGui) {
        executablePath = QFileDialog::getOpenFileName(
            NULL, "Specify location of '" + executableName + "'");
      } else {
        qDebug() << "Could not find \"" << executableName << "\"\n"
                 << "Please run the SketchBio GUI to input the location of "
                    "this program.";
      }
    }
#endif
    setSubprocessExecutablePath(executableName,executablePath);
  }
  qDebug() << "Found " << executableName << " at " << executablePath;
  return executablePath;
}

void setSubprocessExecutablePath(const QString &executableName,
                       const QString &executablePath) {
  QSettings settings;  // default parameters specified to the QCoreApplication
                       // at startup
  settings.setValue(getSettingsStringForSubprocessPath(executableName), executablePath);
}

}
