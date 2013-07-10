#include "chimeravtkexportrunner.h"

#include <QScopedPointer>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QDebug>

#include "subprocessutils.h"

ChimeraVTKExportRunner::ChimeraVTKExportRunner(const QString &pdbId, const QString &vtkFile, int threshold,
                                 const QString &chainsToDelete, QObject *parent) :
    AbstractSingleProcessRunner(parent),
    cmdFile(new QTemporaryFile(QDir::tempPath() + "/XXXXXX.py",this)),
    valid(true)
{
    if (!cmdFile->open())
    {
        valid = false;
    }
    else
    {
        cmdFile->write("# Temporary file created by SketchBio to run in UCSF Chimera\n");
        cmdFile->write("from chimera import runCommand\n\n");
        QString line = "runCommand(\"open %1\")\n";
        cmdFile->write(line.arg(pdbId).toStdString().c_str());
        cmdFile->write("runCommand(\"~show; ~ribbon\")\n");
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
        // TODO - change resolution/export multiple resolutions ?
        line = "runCommand(\"sym #0 surfaces all resolution %1\")\n";
        cmdFile->write(line.arg(threshold).toStdString().c_str());
        cmdFile->write("import sys\n");
        line = "sys.path.insert(0,'%1')\n";
        line = line.arg(SubprocessUtils::getChimeraVTKExtensionDir());
        cmdFile->write(line.toStdString().c_str());
        cmdFile->write("import ExportVTK\n");
        line = "ExportVTK.write_scene_as_vtk(\"%1\")\n";
        cmdFile->write(line.arg(vtkFile).toStdString().c_str());
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
    process->start(SubprocessUtils::getSubprocessExecutablePath("chimera"),
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
