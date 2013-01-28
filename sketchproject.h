#ifndef SKETCHPROJECT_H
#define SKETCHPROJECT_H

#include <vtkRenderer.h>
#include <QVector>
#include <QString>

#include "modelmanager.h"
#include "transformmanager.h"
#include "worldmanager.h"

class SketchProject
{
public:
    SketchProject(vtkRenderer *r, SketchModelId trackerModel,
                  const bool *buttonStates, const double *analogStates);
    ~SketchProject();
    // physics related functions
    void timestep(double dt);
    // adding things functions
    void addObject(QString filename);
    bool addObjects(QVector<QString> filenames);
private:
    // helper functions
        // input related functions
        void handleInput();
        void updateTrackerPositions();
        void updateTrackerObjectConnections();

    // fields
    ModelManager *models;
    TransformManager *transforms;
    WorldManager *world;
    const bool *buttonDown;
    const double *analog;
    int grabbedWorld;
    SketchObject *leftHand, *rightHand;
    std::vector<SpringId> lhand,rhand;
    double lDist, rDist;
    ObjectId lObj, rObj;
};

#endif // SKETCHPROJECT_H
