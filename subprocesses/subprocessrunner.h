#ifndef SUBPROCESSRUNNER_H
#define SUBPROCESSRUNNER_H

#include <QObject>

/*
 *
 * This class is a base for all the subprocess-controlling objects that use Qt signals/slots
 * to do work asynchronously.  When the object is created, first check whether it is valid.
 * If it is not valid delete it.  Then connect all signals/slots
 * then call start() to start the subprocess.  After start() is called, if you wish to cancel
 * the operation, call the cancel() slot.  If cancel() is not called, then the signal finished()
 * will be emitted after the subprocess completes.  All implementations should call deleteLater()
 * after emitting finished and also within the body of cancel so that no explicit deletion is
 * needed.
 *
 */
class SubprocessRunner : public QObject
{
    Q_OBJECT
public:
    explicit SubprocessRunner(QObject *parent = 0) : QObject(parent) {}
    virtual ~SubprocessRunner() {}
    // should return true if the initial setup of the subprocess data succeeded.
    virtual bool isValid() = 0;
public slots:
    // starts the subprocess
    virtual void start() = 0;
    // cancels the subprocess
    virtual void cancel() = 0;
signals:
    // emitted when the subprocess is finished.  The boolean will be true if it succeeded and
    // false otherwise.
    void finished(bool success);
    // emitted to give status messages to the user (use if the process has stages that
    // can be displayed to the user)
    void statusChanged(QString status);
};

#endif // SUBPROCESSRUNNER_H
