#ifndef BLENDERDECIMATIONRUNNER_H
#define BLENDERDECIMATIONRUNNER_H

#include "subprocessrunner.h"

class QProcess;
class QTemporaryFile;

class BlenderDecimationRunner : public SubprocessRunner
{
    Q_OBJECT
public:
    explicit BlenderDecimationRunner(QString objFile, QObject *parent = 0);

    virtual void start();
    virtual void cancel();
    virtual bool isValid();
private slots:
    void blenderFinished(int status);

private:
    QProcess *blender;
    QTemporaryFile *tempFile;
    bool valid;
    
};

#endif // BLENDERDECIMATIONRUNNER_H
