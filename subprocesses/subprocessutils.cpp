#include "subprocessutils.h"
#include <QString>
#include <QSettings>
#include <QFile>
#include <QFileDialog>
#include "chimeraobjmaker.h"
#include "pymolobjmaker.h"
#include "blenderanimationrunner.h"
#include "blenderdecimationrunner.h"

namespace SubprocessUtils {

QString getSubprocessExecutablePath(QString executableName) {
    QSettings settings; // default parameters specified to the QCoreApplication at startup
    QString executablePath = settings.value("subprocesses/" + executableName + "/path",QString("")).toString();
    if (executablePath.length() == 0 || ! QFile(executablePath).exists()) {
#if defined(__APPLE__) && defined(__MACH__)
        // test /Applications/appName and /usr/bin then ask
        if (QFile("/Applications/" + executableName + ".app/Contents/MacOS/" + executableName).exists()) {
            executablePath = "/Applications/" + executableName + ".app/Contents/MacOS/" + executableName;
        } else if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else if (QFile("/usr/local/bin/" + executableName).exists()) {
            executablePath = "/usr/local/bin/" + executableName;
        } else {
            executablePath = QFileDialog::getOpenFileName(NULL,"Specify location of '" + executableName + "'","/Applications");
            if (executablePath.endsWith(".app")) {
                executablePath = executablePath + "/Contents/MacOS/" + executableName;
            }
        }
#elif defined(__WINDOWS__)
        // test default locations C:/Program Files and C:/Program Files(x86) then ask for a .exe file
        if (QFile("C:/Program Files/" + executableName "/" + executableName + ".exe").exists()) {
            executablePath = "C:/Program Files/" + executableName + "/" + executableName;
        }
#ifdef _WIN64
        else if (QFile("C:/Program Files(x86)/" + executableName + "/" + executableName + ".exe").exists()) {
            executablePath = "C:/Program Files(x86)/" + executableName + "/" + executableName;
        }
        else {
            executablePath = QFileDialog::getOpenFileName(NULL, "Specify location of '" executableName + "'","C:/Program Files","",".exe");
        }
#endif
#elif defined(__linux__)
        // test /usr/bin then ask for the file
        if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else {
            executablePath = QFileDialog::getOpenFileName(NULL,"Specify location of '" + executableName + "'");
        }
#endif
        settings.setValue("subprocesses/" + executableName + "/path",executablePath);
    }
    return executablePath;
}

SubprocessRunner *makeChimeraOBJFor(QString pdbID, QString objFile,int threshold)
{
    ChimeraOBJMaker *maker = new ChimeraOBJMaker(pdbID,objFile,threshold);
    if (!maker->isValid())
    {
        delete maker;
        maker = NULL;
    }
    return maker;
}

SubprocessRunner *makePyMolOBJFor(QString pdbID, QString saveDir)
{
    PymolOBJMaker *maker = new PymolOBJMaker(pdbID,saveDir);
    if (!maker->isValid())
    {
        delete maker;
        maker = NULL;
    }
    return maker;
}

SubprocessRunner *createAnimationFor(SketchProject *proj, QString animationFile)
{
    if (!animationFile.endsWith(".avi", Qt::CaseInsensitive))
    {
        // we don't know how to render other formats than avi right now
        return NULL;
    }
    // this object will automatically delete itself when the subprocess finishes with
    // Qt slots/signals... it is really strange that you can do this.
    BlenderAnimationRunner *runner = new BlenderAnimationRunner(proj,animationFile,NULL);
    // check if temporary files were created and such
    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

SubprocessRunner *simplifyObjFile(QString objFile)
{
    BlenderDecimationRunner *runner = new BlenderDecimationRunner(objFile);

    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

}
