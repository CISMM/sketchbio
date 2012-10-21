#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <map>
#include "sketchobject.h"
#include "modelmanager.h"
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

class WorldManager
{
public:
    WorldManager(ModelManager *models, vtkRenderer *r);
    ~WorldManager();

    /*******************************************************************
     *
     * Adds a new object with the given modelId to the world with the
     * given position and orientation  Returns the index of this object.
     *
     * modelId - the id of the model to use when generating the object,
     *           should be a valid modelId with the model manager
     * pos     - the position at which the object should be added
     * orient  - the orientation of the object when it is added
     * worldEyeTransform - this is the world-Eye transformation matrix
     *                  from the TransformManager used to set up the
     *                  new object's actor
     *
     *******************************************************************/
    int addObject(int modelId,q_vec_type pos, q_type orient, vtkTransform *worldEyeTransform);

    /*******************************************************************
     *
     * Removes the SketchObject identified by the given index from the world
     *
     * objectId - the index of the object, returned by addObject
     *
     *******************************************************************/
    void removeObject(int objectId);
    /*******************************************************************
     *
     * Returns the SketchObject identified by the given index
     *
     * objectId - the index of the object, returned by addObject
     *
     *******************************************************************/
    SketchObject * getObject(int objectId);

    /*******************************************************************
     *
     * Returns the number of SketchObjects that are stored in the
     * WorldManager
     *
     *******************************************************************/
    int getNumberOfObjects();

    /*******************************************************************
     *
     * Steps the phyiscs system by timestep dt, applying collision
     * detection, forces, and constraints to the models.
     *
     * dt  - the timestep for the physics simulation
     *
     *******************************************************************/
    void stepPhysics(double dt);
private:
    std::map<int,SketchObject *> objects;
    int nextIdx;
    ModelManager *modelManager;
    vtkSmartPointer<vtkRenderer> renderer;
};

#endif // WORLDMANAGER_H
