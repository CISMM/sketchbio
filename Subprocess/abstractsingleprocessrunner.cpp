#include "abstractsingleprocessrunner.h"

#include <QProcess>
#include <QDebug>

AbstractSingleProcessRunner::AbstractSingleProcessRunner(QObject *parent) :
    SubprocessRunner(parent),
    process(new QProcess(this))
{
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, SIGNAL(finished(int)), this, SLOT(processFinished(int)));
}

AbstractSingleProcessRunner::~AbstractSingleProcessRunner()
{
}

void AbstractSingleProcessRunner::cancel()
{
    qDebug() << "Object surfacing canceled.";
    if (process->state() == QProcess::Running)
    {
        disconnect(process, SIGNAL(finished(int)), this, SLOT(processFinished(int)));
        connect(process, SIGNAL(finished(int)), this, SLOT(deleteLater()));
        process->kill();
    }
}

bool AbstractSingleProcessRunner::didProcessSucceed(QString output)
{
    // default implementation
    return true;
}

void AbstractSingleProcessRunner::processFinished(int status)
{
    bool success = true;
    if (status != QProcess::NormalExit)
    {
        success = false;
    }
    if (process->exitCode() != 0)
    {
        success = false;
    }
    QByteArray result = process->readAll();
    if (!didProcessSucceed(result))
        success = false;
    if (!success)
    {
        qDebug() << "Object surfacing failed.";
        qDebug() << result;
    }
    else
        qDebug() << "Successfully surfaced object.";
    emit finished(success);
    deleteLater();
}
