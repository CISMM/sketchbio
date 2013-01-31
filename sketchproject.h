#ifndef SKETCHPROJECT_H
#define SKETCHPROJECT_H

#include <vtkRenderer.h>
#include <QVector>
#include <QString>
#include <QDir>

#include "modelmanager.h"
#include "transformmanager.h"
#include "worldmanager.h"
#include "structurereplicator.h"

class SketchProject
{
public:
    SketchProject(vtkRenderer *r, const bool *buttonStates, const double *analogStates);
    ~SketchProject();
    // sets the directory path for this project (should be an absolute path)
    bool setProjectDir(QString dir);
    // sets the left hand position/orientation
    void setLeftHandPos(q_xyz_quat_type *loc);
    // sets the right hand position/orientation
    void setRightHandPos(q_xyz_quat_type *loc);
    // physics related functions
    void timestep(double dt);

    // get model manager
    const ModelManager *getModelManager() const;
    // get transform manager
    const TransformManager *getTransformManager() const;
    // get world manager
    const WorldManager *getWorldManager() const;
    const std::vector<StructureReplicator *> *getReplicas() const;
    // gets the directory path for this project (absolute path)
    QString getProjectDir() const;

    // adding things functions
    // for objects
    ObjectId addObject(QString filename);
    bool addObjects(QVector<QString> filenames);
    // for springs between objects (object-tracker springs managed internally)
    SpringId addSpring(ObjectId o1, ObjectId o2, double minRest, double maxRest,
                       double stiffness, q_vec_type o1Pos, q_vec_type o2Pos);
    // for structure replication chains
    StructureReplicator *addReplication(ObjectId o1, ObjectId o2, int numCopies);
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
    std::vector<StructureReplicator *> replicas;

    // project dir
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

inline const ModelManager *SketchProject::getModelManager() const {
    return models;
}

inline const TransformManager *SketchProject::getTransformManager() const {
    return transforms;
}

inline const WorldManager *SketchProject::getWorldManager() const {
    return world;
}

inline const std::vector<StructureReplicator *> *SketchProject::getReplicas() const {
    return &replicas;
}

#endif // SKETCHPROJECT_H
