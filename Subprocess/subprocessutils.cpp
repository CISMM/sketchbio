#include "subprocessutils.h"

#include <QString>
#include <QSettings>
#include <QFile>
#include <QApplication>
#include <QDebug>
#include <QFileDialog>

#include "chimeravtkexportrunner.h"
#include "pymolobjmaker.h"
#include "blenderanimationrunner.h"
#include "blenderdecimationrunner.h"
#include "modelfrompdbrunner.h"

namespace SubprocessUtils {

QString getSubprocessExecutablePath(const QString &executableName) {
    QSettings settings; // default parameters specified to the QCoreApplication at startup
    QString executablePath = settings.value("subprocesses/" + executableName + "/path",QString("")).toString();
    if (executablePath.length() == 0 || ! QFile(executablePath).exists()) {
        bool hasGui = (qobject_cast<QApplication *>(QCoreApplication::instance()) != 0);
#if defined(__APPLE__) && defined(__MACH__)
        // test /Applications/appName and /usr/bin then ask
        if (QFile("/Applications/" + executableName + ".app/Contents/MacOS/" + executableName).exists()) {
            executablePath = "/Applications/" + executableName + ".app/Contents/MacOS/" + executableName;
        } else if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else if (QFile("/usr/local/bin/" + executableName).exists()) {
            executablePath = "/usr/local/bin/" + executableName;
        } else {
            if (hasGui)
            {
                executablePath = QFileDialog::getOpenFileName(NULL,"Specify location of '" + executableName + "'","/Applications");
                if (executablePath.endsWith(".app")) {
                    executablePath = executablePath + "/Contents/MacOS/" + executableName;
                }
            }
            else
            {
                qDebug() << "Could not find \"" << executableName << "\"\n" <<
                            "Please run the SketchBio GUI to input the location of this program.";
            }
        }
#elif defined(_WIN32)
        // test default locations C:/Program Files and C:/Program Files(x86) then ask for a .exe file
        if (QFile("C:/Program Files/" + executableName + "/" + executableName + ".exe").exists()) {
            executablePath = "C:/Program Files/" + executableName + "/" + executableName;
        }
        else if (QFile("C:/Program Files (x86)/" + executableName + "/" + executableName + ".exe").exists()) {
            executablePath = "C:/Program Files (x86)/" + executableName + "/" + executableName;
        }
        else {
            if (hasGui)
            {
                executablePath = QFileDialog::getOpenFileName(NULL, "Specify location of '" + executableName + "'","C:/Program Files","Windows Execuatables (*.exe)");
            }
            else
            {
                qDebug() << "Could not find \"" << executableName << "\"\n" <<
                            "Please run the SketchBio GUI to input the location of this program.";
            }
        }
#elif defined(__linux__)
        // test /usr/bin then ask for the file
        if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else {
            if (hasGui)
            {
                executablePath = QFileDialog::getOpenFileName(NULL,"Specify location of '" + executableName + "'");
            }
            else
            {
                qDebug() << "Could not find \"" << executableName << "\"\n" <<
                            "Please run the SketchBio GUI to input the location of this program.";
            }
        }
#endif
        settings.setValue("subprocesses/" + executableName + "/path",executablePath);
    }
	qDebug() << "Found " << executableName << " at " << executablePath;
    return executablePath;
}

SubprocessRunner *makeChimeraSurfaceFor(const QString &pdbID, const QString &vtkFile,int threshold,
                                    const QString &chainsToDelete)
{
    ChimeraVTKExportRunner *maker = new ChimeraVTKExportRunner(pdbID,vtkFile,threshold,chainsToDelete);
    if (!maker->isValid())
    {
        delete maker;
        maker = NULL;
    }
    return maker;
}

SubprocessRunner *makePyMolOBJFor(const QString &pdbID, const QString &saveDir)
{
    PymolOBJMaker *maker = new PymolOBJMaker(pdbID,saveDir);
    if (!maker->isValid())
    {
        delete maker;
        maker = NULL;
    }
    return maker;
}

SubprocessRunner *createAnimationFor(SketchProject *proj, const QString &animationFile)
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

SubprocessRunner *simplifyObjFileByPercent(const QString &objFile, double percentOfOriginal)
{
    BlenderDecimationRunner *runner = new BlenderDecimationRunner(objFile,DecimationType::PERCENT,
                                                                  percentOfOriginal);

    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

SubprocessRunner *simplifyObjFile(const QString &objFile, int triangles)
{
    BlenderDecimationRunner *runner = new BlenderDecimationRunner(
                objFile,DecimationType::POLYGON_COUNT,triangles);

    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

SubprocessRunner *loadFromPDB(SketchProject *proj, const QString &pdb,
                              const QString &chainsToDelete)
{
    ModelFromPDBRunner *runner = new ModelFromPDBRunner(proj,pdb,chainsToDelete);
    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

}
