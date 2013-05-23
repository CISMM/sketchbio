#include "chimeraobjmaker.h"
#include "subprocessutils.h"

#include <QScopedPointer>
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>

ChimeraOBJMaker::ChimeraOBJMaker(QString pdbId, QString objFile, QObject *parent) :
    SubprocessRunner(parent),
    chimera(new QProcess(this)),
    cmdFile(new QTemporaryFile("XXXXXX.py",this)),
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
        // TODO - change resolution/export multiple resolutions ?
        cmdFile->write("runCommand(\"sym #0 surfaces all resolution 0\")\n");
        cmdFile->write("import ExportOBJ\n");
        line = "ExportOBJ.write_surfaces_as_wavefront_obj(\"%1\")\n";
        cmdFile->write(line.arg(objFile).toStdString().c_str());
        cmdFile->write("runCommand(\"close all\")\n");
        cmdFile->write("runCommand(\"stop now\")\n");
        if (cmdFile->error() != QFile::NoError)
            valid = false;
        cmdFile->close();
        connect(chimera,SIGNAL(finished(int)),this, SLOT(processResult(int)));
    }
}

ChimeraOBJMaker::~ChimeraOBJMaker()
{
}

bool ChimeraOBJMaker::isValid()
{
    return valid;
}

void ChimeraOBJMaker::start()
{
    qDebug() << "Starting Chimera";
    chimera->start(SubprocessUtils::getSubprocessExecutablePath("chimera"),
                   QStringList() << "--nogui" << cmdFile->fileName()
                   );
    if (!chimera->waitForStarted())
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

void ChimeraOBJMaker::processResult(int status)
{
    bool success = true;
    if (status != QProcess::NormalExit)
    {
        success = false;
    }
    if (chimera->exitCode() != 0)
    {
        success = false;
    }
    if (!success)
    {
        qDebug() << "Object surfacing failed.";
        qDebug() << chimera->readAll();
    }
    else
        qDebug() << "Successfully surfaced object.";
    emit finished(success);
    deleteLater();
}

void ChimeraOBJMaker::cancel()
{
    qDebug() << "Object surfacing canceled.";
    if (chimera->state() == QProcess::Running)
    {
        disconnect(chimera, SIGNAL(finished(int)), this, SLOT(processResult(int)));
        connect(chimera, SIGNAL(finished(int)), this, SLOT(deleteLater()));
        chimera->kill();
    }
}
