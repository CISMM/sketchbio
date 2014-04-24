#include "blenderanimationrunner.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryFile>

#include <sketchproject.h>
#include <SettingsHelpers.h>

#include <projecttoblenderanimation.h>

#include "subprocessutils.h"

BlenderAnimationRunner::BlenderAnimationRunner(SketchBio::Project *proj, const QString &aFile,
                                               QObject *parent) :
    SubprocessRunner(parent),
    blender(NULL),
    ffmpeg(NULL),
    py_file(new QTemporaryFile(QDir::tempPath() + "/XXXXXX.py",this)),
    animationFile(new QFile(aFile,this)),
    frameDir(NULL),
    valid(true)
{
    // for now we only render avi files
    if (!aFile.endsWith(".avi",Qt::CaseInsensitive))
    {
        valid = false;
    }
    // py_file is a child of this object as well
    if (!py_file->open())
    {
        valid = false;
    }
    if (!ProjectToBlenderAnimation::writeProjectBlenderFile(
                *py_file,proj,SubprocessUtils::getChimeraVTKExtensionDir()))
    {
        valid = false;
    } else
    {
        qDebug() << "Wrote temporary file.";
    }
    py_file->close();

    QDir dir = QDir(proj->getProjectDir());
    QDir dir2 = dir.absoluteFilePath("anim");

    if (!dir2.exists())
    {
        if (!dir.mkdir("anim"))
        {
            valid = false;
        }
    }
    if (dir2.exists())
    {
        frameDir = new QDir(dir2);
        dir2.setFilter(QDir::Files | QDir::NoDotAndDotDot);
        QStringList files = dir2.entryList();
        for (int idx = 0; idx < files.length(); idx++)
        {
            QFile f(dir2.absoluteFilePath(files[idx]));
            if (!f.remove())
            {
                qDebug() << "Could not remove " << files[idx];
                valid = false;
            }
        }
    }
    else
    {
        valid = false;
    }
    // create the process as a child of this object
    blender = new QProcess(this);
    blender->setProcessChannelMode(QProcess::MergedChannels);

    // set up signals
    connect(blender, SIGNAL(finished(int)), this, SLOT(firstStageDone(int)));

}

BlenderAnimationRunner::~BlenderAnimationRunner()
{
    qDebug() << "Cleaning up from animation.";
    delete frameDir;
}

void BlenderAnimationRunner::start()
{
    // start the subprocess for blender
    blender->start(SettingsHelpers::getSubprocessExecutablePath("blender"),
                   QStringList() << "-noaudio" << "-b" <<
                   "-F" << "PNG" << "-x" << "1" << "-o" <<
                   frameDir->absolutePath() +"/#####.png" <<"-P"
                   << py_file->fileName());
    qDebug() << "Starting blender.";
    if (!blender->waitForStarted())
    {
        emit finished(false);
        qDebug() << "Blender failed to start";
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
        deleteLater();
    }
    else
        emit statusChanged("Rendering frames in blender.");
}

bool BlenderAnimationRunner::isValid()
{
    return valid;
}


void BlenderAnimationRunner::firstStageDone(int exitCode)
{
    // blender has finished rendering frames
	QFile f(frameDir->absoluteFilePath("00001.png"));
	if ((blender->exitStatus() == QProcess::CrashExit) || (exitCode != 0) || ! f.exists()) {
        qDebug() << "Blender animation failed";
        qDebug() << blender->readAll();
        emit finished(false);
        deleteLater();
    } else {
        qDebug() << "Done rendering frames.";
        ffmpeg = new QProcess(this);
        ffmpeg->setReadChannelMode(QProcess::MergedChannels);

        connect(ffmpeg, SIGNAL(finished(int)), this, SLOT(secondStageDone(int)));

        QStringList list;
        list << "-r" << QString::number(BLENDER_RENDERER_FRAMERATE) << "-i"
             << frameDir->absolutePath() + "/%05d.png"
             << "-vcodec" << "huffyuv" << animationFile->fileName();
//        qDebug() << list;
        ffmpeg->start(SettingsHelpers::getSubprocessExecutablePath("ffmpeg"),list);
        qDebug() << "Starting ffmpeg.";
        emit statusChanged("Converting frames into video.");
    }
}

void BlenderAnimationRunner::secondStageDone(int exitCode)
{
    if ((ffmpeg->exitStatus() == QProcess::CrashExit) || (exitCode != 0))
    {
        qDebug() << "Failed to create video.";
        qDebug() << ffmpeg->readAll();
        emit finished(false);
        deleteLater();
    } else
    {
        qDebug() << "Finished creating video.";
        emit finished(true);
        this->deleteLater();
    }
}

void BlenderAnimationRunner::cancel()
{
    qDebug() << "Animation canceled.";
    if (blender != NULL && blender->state() == QProcess::Running)

        disconnect(blender, SIGNAL(finished(int)), this, SLOT(firstStageDone(int)));
        connect(blender, SIGNAL(finished(int)), this, SLOT(deleteLater()));{
        blender->kill();
    }
    if (ffmpeg != NULL && ffmpeg->state() == QProcess::Running)
    {
        disconnect(ffmpeg, SIGNAL(finished(int)), this, SLOT(secondStageDone(int)));
        connect(ffmpeg, SIGNAL(finished(int)), this, SLOT(deleteLater()));
        ffmpeg->kill();
    }
}

