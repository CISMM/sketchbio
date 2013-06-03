#ifndef MODELFROMPDBRUNNER_H
#define MODELFROMPDBRUNNER_H

#include <subprocessrunner.h>

class SketchProject;
class SketchModel;

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
    void stepFinished(bool succeeded);
private:
    QString pdbId;
    SketchProject *project;
    SketchModel *model;
    SubprocessRunner *currentRunner;
    int stepNum;
};

#endif // MODELFROMPDBRUNNER_H
