#ifndef MODELFROMPDBRUNNER_H
#define MODELFROMPDBRUNNER_H

#include <subprocessrunner.h>

class SketchProject;
class SketchModel;

// This is a subprocess runner to load a pdb file into a model object and
// create the various simplification levels of it.  Currently it uses the
// chimera to create surfaces from the pdb ids and blender to simplify them
// to the various levels.
class ModelFromPDBRunner : public SubprocessRunner
{
    Q_OBJECT
public:
    ModelFromPDBRunner(SketchProject *proj, QString &pdb, QObject *parent = 0);
    ~ModelFromPDBRunner();

    virtual void start();
    virtual void cancel();
    virtual bool isValid();
private slots:
    // Called when a subprocess finishes.  Since only one subprocess is called
    // at a time, there is a definite order to which just finished, kept track of
    // in the field stepNum
    void stepFinished(bool succeeded);
private:
    // PDB id
    QString pdbId;
    // project to add model to
    SketchProject *project;
    // The model (once it is created, keep a reference to it)
    SketchModel *model;
    // The conformation within the model (-1 before the model is created)
    int conformation;
    // The current subprocess (uses other SubprocessRunners instead of reimplementing)
    SubprocessRunner *currentRunner;
    // The step number we are on (which subprocess is running)
    int stepNum;
};

#endif // MODELFROMPDBRUNNER_H
