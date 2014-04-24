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

QString getChimeraVTKExtensionDir()
{
    QString appPath = QApplication::instance()->applicationDirPath();
#if defined(__APPLE__) && defined(__MACH__)
    if (appPath.endsWith("MacOS")) // If we are in an application bundle
    {
        return appPath.mid(0,appPath.lastIndexOf("/")).append("Resources/scripts");
    }
    else // development build
    {
        return appPath.append("/scripts");
    }
#elif defined(_WIN32)
	QString result;
    if (appPath.contains("Program Files")) // Installed
    {
        result = appPath.append("/scripts");
    }
    else if (appPath.contains("Subprocess")) // development build test programs
    {
        result = appPath.mid(0,appPath.lastIndexOf("/Subprocess") + 1).append("scripts");
    }
	else // development build
	{
		result = appPath.mid(0,appPath.lastIndexOf("/")).append("/scripts");
	}
	result = result.replace("/","\\\\");
	qDebug() << "Found chimera scripts at: " << result;
	return result;
#elif defined(__linux__)
    if (appPath.contains("/usr"))
    {
        return "/usr/local/share/sketchbio";
    }
    else
    {
        return appPath.append("/scripts");
    }
#endif
}

SubprocessRunner *makeChimeraSurfaceFor(
        const QString &pdbID, const QString &vtkFile,int threshold,
        const QString &chainsToDelete,bool shouldExportBiologicalUnit)
{
    ChimeraVTKExportRunner *maker = new ChimeraVTKExportRunner(
                pdbID,vtkFile,threshold,chainsToDelete,shouldExportBiologicalUnit);
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

SubprocessRunner *createAnimationFor(SketchBio::Project *proj, const QString &animationFile)
{
    if (!animationFile.endsWith(".avi", Qt::CaseInsensitive))
    {
        // we don't know how to render other formats than avi right now
        qDebug() << "Illegal file format" << animationFile.toStdString().c_str();
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

SubprocessRunner *loadFromPDBId(
        SketchBio::Project *proj, const QString &pdb,
        const QString &chainsToDelete, bool exportWholeBiologicalUnit)
{
    ModelFromPDBRunner *runner = new ModelFromPDBRunner(
                proj,pdb,chainsToDelete,exportWholeBiologicalUnit);
    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

SubprocessRunner *loadFromPDBFile(
        SketchBio::Project *proj, const QString &pdbfilename,
        const QString &chainsToDelete, bool exportWholeBiologicalUnit)
{
    QString prefix = pdbfilename.mid(pdbfilename.lastIndexOf("/")+1);
    ModelFromPDBRunner *runner = new ModelFromPDBRunner(
                proj,pdbfilename,
                prefix,chainsToDelete,exportWholeBiologicalUnit);
    if (!runner->isValid())
    {
        delete runner;
        return NULL;
    }
    return runner;
}

}
