#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <map>
#include <list>
#include "sketchobject.h"
#include "modelmanager.h"
#include "springconnection.h"
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

// These will be returned from the add calls and used to get the
// object/spring that was added and later to remove it
typedef std::list<SpringConnection *>::iterator SpringId;
typedef std::list<SketchObject *>::iterator ObjectId;

class WorldManager
{
public:
    WorldManager(ModelManager *models, vtkRenderer *r, vtkTransform *worldEyeTransform);
    ~WorldManager();

    /*******************************************************************
     *
     * Adds a new object with the given modelId to the world with the
     * given position and orientation  Returns the index of this object.
     *
     * All objects created here are deleted either in removeObject or
     * the destructor of the world manager.
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
    ObjectId addObject(int modelId,q_vec_type pos, q_type orient, vtkTransform *worldEyeTransform);

    /*******************************************************************
     *
     * Removes the SketchObject identified by the given index from the world
     *
     * objectId - the index of the object, returned by addObject
     *
     *******************************************************************/
    void removeObject(ObjectId objectId);

    /*******************************************************************
     *
     * Returns the number of SketchObjects that are stored in the
     * WorldManager
     *
     *******************************************************************/
    int getNumberOfObjects();

    /*******************************************************************
     *
     * Adds the given spring to the list of springs.  Returns an iterator
     * to the position of that spring in the list
     *
     * This passes ownership of the SpringConnection to the world manager
     * and the spring connection will be deleted in removeSpring
     * or the world manager's destructor
     *
     * spring - the spring to add
     *
     *******************************************************************/
    SpringId addSpring(SpringConnection *spring);

    /*******************************************************************
     *
     * Removes the spring identified by the given iterator from the list
     *
     *******************************************************************/
    void removeSpring(SpringId id);

    /*******************************************************************
     *
     * Returns the number of springs stored in the spring list
     *
     *******************************************************************/
    int getNumberOfSprings();

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
    std::list<SketchObject *> objects;
    std::list<SpringConnection *> connections;
    int nextIdx;
    ModelManager *modelManager;
    vtkSmartPointer<vtkRenderer> renderer;
    int lastCapacityUpdate;
    vtkSmartPointer<vtkPoints> springEnds;
    vtkSmartPointer<vtkPolyData> springEndConnections;
};

#endif // WORLDMANAGER_H
