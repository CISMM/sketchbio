#ifndef MODELFROMPDBRUNNER_H
#define MODELFROMPDBRUNNER_H

#include <subprocessrunner.h>

namespace SketchBio {
class Project;
}
class SketchModel;

// This is a subprocess runner to load a pdb file into a model object and
// create the various simplification levels of it.  Currently it uses the
// chimera to create surfaces from the pdb ids and blender to simplify them
// to the various levels.
class ModelFromPDBRunner : public SubprocessRunner
{
    Q_OBJECT
public:
    // proj - the project
    // pdb - the 4 character pdb id
    // toDelete - chain identifiers to delete before surfacing
    ModelFromPDBRunner(SketchBio::Project *proj, const QString &pdb,
                       const QString &toDelete, bool shouldExportBiologicalUnit,
                       QObject *parent = 0);
    // proj - the project
    // filename - the pdb file
    // modelFilePre - the prefix to use for the model files in the project dir
    // toDeltee - chain identifiers to delete before surfacing
    ModelFromPDBRunner(SketchBio::Project *proj, const QString &filename,
                       const QString &modelFilePre, const QString &toDelete,
                       bool shouldExportBiologicalUnit,
                       QObject *parent = 0);
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
    // PDB id, and the chain identifiers of chains to delete before surfacing
    QString pdbId, chainsToDelete, modelFilePrefix;
    // project to add model to
    SketchBio::Project *project;
    // The model (once it is created, keep a reference to it)
    SketchModel *model;
    // The conformation within the model (-1 before the model is created)
    int conformation;
    // The current subprocess (uses other SubprocessRunners instead of reimplementing)
    SubprocessRunner *currentRunner;
    // The step number we are on (which subprocess is running)
    int stepNum;
    bool importFromLocalFile, exportWholeBiologicalUnit;
};

#endif // MODELFROMPDBRUNNER_H
