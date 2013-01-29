#ifndef SKETCHPROJECT_H
#define SKETCHPROJECT_H

#include <vtkRenderer.h>
#include <QVector>
#include <QString>
#include <QDir>

#include "modelmanager.h"
#include "transformmanager.h"
#include "worldmanager.h"

class SketchProject
{
public:
    SketchProject(vtkRenderer *r, const bool *buttonStates, const double *analogStates);
    ~SketchProject();
    // sets the directory path for this project (should be an absolute path)
    bool setProjectDir(QString dir);
    // physics related functions
    void timestep(double dt);

    // get model manager
    ModelManager *getModelManager();
    // get transform manager
    TransformManager *getTransformManager();
    // get world manager
    WorldManager *getWorldManager();
    // gets the directory path for this project (absolute path)
    QString getProjectDir() const;

    // adding things functions
    bool addObject(QString filename);
    bool addObjects(QVector<QString> filenames);
private:
    // helper functions
    // input related functions
    void handleInput();
    void updateTrackerPositions();
    void updateTrackerObjectConnections();

    // fields
    // managers
    ModelManager *models;
    TransformManager *transforms;
    WorldManager *world;
    QDir *projectDir;

    // user interaction stuff
    const bool *buttonDown;
    const double *analog;
    int grabbedWorld;
    SketchObject *leftHand, *rightHand;
    std::vector<SpringId> lhand,rhand;
    double lDist, rDist;
    ObjectId lObj, rObj;
};

inline ModelManager *SketchProject::getModelManager() {
    return models;
}

inline TransformManager *SketchProject::getTransformManager() {
    return transforms;
}

inline WorldManager *SketchProject::getWorldManager() {
    return world;
}

#endif // SKETCHPROJECT_H
