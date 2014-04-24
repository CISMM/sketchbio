#include "chimeravtkexportrunner.h"

#include <QScopedPointer>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QDebug>

#include <SettingsHelpers.h>

#include "subprocessutils.h"

ChimeraVTKExportRunner::ChimeraVTKExportRunner(
        const QString &pdbId, const QString &vtkFile, int threshold,
        const QString &chainsToDelete, bool shouldExportWholeBioUnit,
        QObject *parent) :
    AbstractSingleProcessRunner(parent),
    cmdFile(new QTemporaryFile(QDir::tempPath() + "/XXXXXX.py",this)),
    resultFile(vtkFile),
    valid(true)
{
    if (!cmdFile->open())
    {
        valid = false;
    }
    else
    {
        cmdFile->write("# Temporary file created by SketchBio to run in UCSF Chimera\n");
        cmdFile->write("from chimera import runCommand\n");
        cmdFile->write("from chimera import openModels\n\n");
        QString line = "runCommand(\"open %1\")\n";
        cmdFile->write(line.arg(pdbId).toStdString().c_str());
        cmdFile->write("runCommand(\"~show; ~ribbon\")\n");
        cmdFile->write("runCommand(\"delete solvent\")\n");
        QString cTD = chainsToDelete.trimmed();
        for (int i = 0; i < cTD.length(); i++)
        {
            QChar c = cTD.at(i).toUpper();
            if (c.isUpper())
            {
                line = "runCommand(\"delete #0:.%1\")\n";
                line = line.arg(c);
                cmdFile->write(line.toStdString().c_str());
            }
        }
        // set up to export charge
        cmdFile->write("runCommand(\"surf\")\n");
        cmdFile->write("runCommand(\"coulombic gpadding 20.0 gname CoulombicESP -10 red 0 white 10 blue\")\n");
        cmdFile->write("volume = openModels.list()[2]\n");
        cmdFile->write("modelList = [openModels.list()[0]]\n");
        // TODO - change resolution/export multiple resolutions ?
        cmdFile->write("from Midas import MidasError\n");
        cmdFile->write("try:\n");
        if (shouldExportWholeBioUnit)
        {
            cmdFile->write("\tvolume = None\n"); // TODO - can't do whole biounit
            // charge data yet, need to store something about xform used to create
            // point first
            // this by default exports the whole biological unit
            line = "\trunCommand(\"sym #0 surfaces all resolution %1\")\n";
        }
        else
        {
            // this command explicitly defines what to export as just the existing piece
            // and will work even when no biological unit information is present
            line = "\trunCommand(\"sym #0 group shift,1,0 surfaces all resolution %1\")\n";
        }
        cmdFile->write(line.arg(threshold).toStdString().c_str());
        cmdFile->write("\tmodelList.append(openModels.list()[3])\n");
        cmdFile->write("except MidasError:\n");
        cmdFile->write("\tprint \"Failed to create Multiscale surface...\"\n");
        cmdFile->write("\tmodelList.append(openModels.list()[1])\n");
        cmdFile->write("import sys\n");
        line = "sys.path.insert(0,'%1')\n";
        line = line.arg(SubprocessUtils::getChimeraVTKExtensionDir());
        cmdFile->write(line.toStdString().c_str());
        cmdFile->write("import ExportVTK\n");
        line = "ExportVTK.write_models_as_vtk(\"%1\",modelList,volume)\n";
#ifdef _WIN32
		cmdFile->write(line.arg(QString(vtkFile).replace("/","\\\\")).toStdString().c_str());
#else
        cmdFile->write(line.arg(vtkFile).toStdString().c_str());
#endif
        cmdFile->write("runCommand(\"close all\")\n");
        cmdFile->write("runCommand(\"stop now\")\n");
        cmdFile->close();
        if (cmdFile->error() != QFile::NoError)
            valid = false;
    }
}

ChimeraVTKExportRunner::~ChimeraVTKExportRunner()
{
    QFile f(cmdFile->fileName() + "c");
    if (f.exists())
        f.remove();
}

bool ChimeraVTKExportRunner::isValid()
{
    return valid;
}

void ChimeraVTKExportRunner::start()
{
    qDebug() << "Starting Chimera";
    process->start(SettingsHelpers::getSubprocessExecutablePath("chimera"),
                   QStringList() << "--nogui" << cmdFile->fileName()
                   );
    if (!process->waitForStarted())
    {
        emit finished(false);
        deleteLater();
    }
    else
    {
        emit statusChanged("Creating surface with UCSF Chimera...");
        qDebug() << "Chimera Started";
    }
}

bool ChimeraVTKExportRunner::didProcessSucceed(QString output)
{
    if (output.contains("Failed to create Multiscale surface..."))
    {
        qDebug() << "Warning: using full Connoly surface since reduced resolution"
                    " surfacing failed.";
    }
    return QFile(resultFile).exists();
}
