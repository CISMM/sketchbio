#ifndef BLENDERANIMATIONRUNNER_H
#define BLENDERANIMATIONRUNNER_H

#include <QObject>
#include <QProcess>
#include <QTemporaryFile>

class SketchProject;

/*
 * This class manages exporting animations to blender and ffmpeg so that the export will run from a single
 * call in the GUI code.  This uses the Qt event loop and signals/slots to do the work asynchronously, (however,
 * it is done on the main thread unless the object is given to another thread).
 */
class BlenderAnimationRunner : public QObject
{
    Q_OBJECT
public:
    // this returns an object that can will be deleted when the application is done
    // rendering the animation.  Listen for the finished() signal on this object to
    // find out when the animation is done.  Note, this creates the object and starts it
    // the parameters are the file to save the animation to (should be .avi) and the
    // project to generate the animation from.
    // If this function returns NULL then the temporary files/directories could not be created and
    // creating the subprocesses failed.
    static BlenderAnimationRunner *createAnimationFor(const SketchProject *project, QString &animationFile);
private:
    explicit BlenderAnimationRunner(const SketchProject *proj, QString &animationFile, QObject *parent = 0);
    virtual ~BlenderAnimationRunner();

    void start();
    bool isValid();
    
signals:
    // gives intermediate status messages about what it is doing
    void statusChanged(QString str);
    // emitted when the animation is done. The boolean is true iff the subprocesses succeeded.
    // note: when finished is emitted the object is scheduled for deletion, so do not assume the
    // reference is still valid after this signal
    void finished(bool success);
private slots:
    // internal slots used as callbacks from the processes
    void firstStageDone();
    void secondStageDone();
public slots:
    // call to cancel the animation and kill subprocesses
    void canceled();
private:
    QProcess *blender;
    QProcess *ffmpeg;
    QTemporaryFile *py_file;
    QFile *animationFile;
    bool valid;
    
};

QString getSubprocessExecutablePath(QString executableName);

#endif // BLENDERANIMATIONRUNNER_H
