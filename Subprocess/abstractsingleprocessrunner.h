#ifndef ABSTRACTSINGLEPROCESSRUNNER_H
#define ABSTRACTSINGLEPROCESSRUNNER_H

#include "subprocessrunner.h"

#include <QString>

class QProcess;

class AbstractSingleProcessRunner : public SubprocessRunner
{
    Q_OBJECT
public:
    // ctor and dtor
    explicit AbstractSingleProcessRunner(QObject *parent = 0);
    virtual ~AbstractSingleProcessRunner();

    // override if additional cleanup needed, but this should be
    // called from the overriding class
    virtual void cancel();
    
private slots:
    void processFinished(int status);

protected:
    // Override to test for success after the process finishes.
    // default returns true indicating that it found success.
    // This is guaranteed to be called after process finished.
    virtual bool didProcessSucceed(QString output);

protected:
    // the process - should be started in overridden start method
    QProcess *process;
    
};

#endif // ABSTRACTSINGLEPROCESSRUNNER_H
