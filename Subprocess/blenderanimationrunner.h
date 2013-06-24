#ifndef BLENDERANIMATIONRUNNER_H
#define BLENDERANIMATIONRUNNER_H

#include "subprocessrunner.h"

class SketchProject;
class QProcess;
class QFile;
class QTemporaryFile;
class QDir;

/*
 * This class manages exporting animations to blender and ffmpeg so that the export will run from a single
 * call in the GUI code.  This uses the Qt event loop and signals/slots to do the work asynchronously, (however,
 * it is done on the main thread unless the object is given to another thread).
 */
class BlenderAnimationRunner : public SubprocessRunner
{
    Q_OBJECT
public:
    // constructor
    explicit BlenderAnimationRunner(SketchProject *proj, const QString &aFile,
                                    QObject *parent = 0);
    // destructor
    virtual ~BlenderAnimationRunner();
    // starts the animation process
    virtual void start();
    // call to cancel the animation and kill subprocesses
    virtual void cancel();
    // test if the setup succeeded
    virtual bool isValid();
    
private slots:
    // internal slots used as callbacks from the processes
    void firstStageDone(int exitCode);
    void secondStageDone(int exitCode);
private:
    QProcess *blender;
    QProcess *ffmpeg;
    QTemporaryFile *py_file;
    QFile *animationFile;
    QDir *frameDir;
    bool valid;
    
};

#endif // BLENDERANIMATIONRUNNER_H
