#include "blenderanimationrunner.h"
#include <projecttoblenderanimation.h>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QFile>

// remove when relocating getSubprocessExecutableName
#include <QSettings>
#include <QFileDialog>

BlenderAnimationRunner::BlenderAnimationRunner(SketchProject *proj, QString &animationFile, QObject *parent) :
    QObject(parent),
    blender(NULL),
    ffmpeg(NULL),
    py_file(new QTemporaryFile("XXXXXX.py",this)),
    animationFile(new QFile(animationFile,this)),
    valid(true)
{
    // py_file is a child of this object as well
    if (!py_file->open())
    {
        valid = false;
    }
    if (!ProjectToBlenderAnimation::writeProjectBlenderFile(*py_file,proj))
    {
        valid = false;
    } else
    {
        qDebug() << "Wrote temporary file.";
    }
    py_file->close();

    QDir dir = QDir(QDir::currentPath());
    QDir dir2 = dir.absoluteFilePath("anim");

    if (!dir2.exists())
    {
        if (!dir.mkdir("anim"))
        {
            valid = false;
        }
    } else {
        dir2.setFilter(QDir::Files | QDir::NoDotAndDotDot);
        QStringList files = dir2.entryList();
        for (int idx = 0; idx < files.length(); idx++) {
            QFile f(dir2.absoluteFilePath(files[idx]));
            if (!f.remove()) {
                qDebug() << "Could not remove " << files[idx];
                valid = false;
            }
        }
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
}

void BlenderAnimationRunner::start()
{
    // start the subprocess for blender
    blender->start(getSubprocessExecutablePath("blender") ,QStringList() << "-noaudio" << "-b" <<
                   "-F" << "PNG" << "-x" << "1" << "-o" << "anim/#####.png" <<"-P" << py_file->fileName());
    qDebug() << "Starting blender.";
    emit statusChanged("Rendering frames in blender.");
}

bool BlenderAnimationRunner::isValid()
{
    return valid;
}

BlenderAnimationRunner * BlenderAnimationRunner::createAnimationFor(SketchProject *proj, QString &animationFile)
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
    runner->start();
    return runner;
}

void BlenderAnimationRunner::firstStageDone(int exitCode)
{
    // blender has finished rendering frames
    if ((blender->exitStatus() == QProcess::CrashExit) || (exitCode != 0)) {
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
        list << "-r" << QString::number(BLENDER_RENDERER_FRAMERATE) << "-i" << "anim/%05d.png";
        list << "-vcodec" << "huffyuv" << animationFile->fileName();
//        qDebug() << list;
        ffmpeg->start(getSubprocessExecutablePath("ffmpeg"),list);
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

void BlenderAnimationRunner::canceled()
{
    qDebug() << "Animation canceled.";
    if (blender != NULL && blender->state() == QProcess::Running)
    {
        blender->kill();
        disconnect(blender, SIGNAL(finished(int)), this, SLOT(firstStageDone()));
        connect(blender, SIGNAL(finished(int)), this, SLOT(deleteLater()));
    }
    if (ffmpeg != NULL && ffmpeg->state() == QProcess::Running)
    {
        ffmpeg->kill();
        disconnect(ffmpeg, SIGNAL(finished(int)), this, SLOT(secondStageDone()));
        connect(ffmpeg, SIGNAL(finished(int)), this, SLOT(deleteLater()));
    }
}

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
#endif
        else {
            executablePath = QFileDialog::getOpenFileName(this, "Specify location of '" executableName + "'","C:/Program Files","",".exe");
#elif defined(__linux__)
        // test /usr/bin then ask for the file
        if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else {
            executablePath = QFileDialog::getOpenFileName(this,"Specify location of '" + executableName + "'");
        }
#endif
        settings.setValue("subprocesses/" + executableName + "/path",executablePath);
    }
    return executablePath;
}
