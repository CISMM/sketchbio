#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <map>
#include <list>
#include "sketchobject.h"
#include "modelmanager.h"
#include "springconnection.h"
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkTubeFilter.h>

/*
 * This class contains the data that is in the modeled "world", all the objects and
 * springs.  It also contains code to step the "physics" of the simulation and run
 * collision tests.
 */

class WorldManager
{
public:
    WorldManager(vtkRenderer *r, vtkTransform *worldEyeTransform);
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
    ObjectId addObject(SketchModel *model,q_vec_type pos, q_type orient);
    /*******************************************************************
     * Adds a the given object to the world and returns its index.
     *
     * object - the object to add
     *******************************************************************/
    ObjectId addObject(SketchObject *object);

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
    int getNumberOfObjects() const;

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
     * Adds the a spring between the two models and returns its id.  Returns an iterator
     * to the position of that spring in the list
     *
     * id1 - the id of the first object to connect
     * id2 - the id of the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world coordinate space,
     *                  - false if they are relative to the model coordiante space
     * k - the stiffness of the spring
     * l - the length of the spring
     *
     *******************************************************************/
    SpringId addSpring(ObjectId id1, ObjectId id2, const q_vec_type pos1,
                       const q_vec_type pos2, bool worldRelativePos, double k, double minL, double maxL);

    /*******************************************************************
     *
     * Adds the a spring between the two models and returns its id.  Returns an iterator
     * to the position of that spring in the list
     *
     * id1 - the id of the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world coordinate space,
     *                  - false if they are relative to the model coordiante space
     * k - the stiffness of the spring
     * l - the length of the spring
     *
     *******************************************************************/
    SpringId addSpring(ObjectId id1, SketchObject *o2, const q_vec_type pos1,
                       const q_vec_type pos2, bool worldRelativePos, double k, double l);

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
    int getNumberOfSprings() const;

    /*******************************************************************
     *
     * Steps the phyiscs system by timestep dt, applying collision
     * detection, forces, and constraints to the models.
     *
     * dt  - the timestep for the physics simulation
     *
     *******************************************************************/
    void stepPhysics(double dt);
    /*******************************************************************
     *
     * Turns on or off the physics, with physics off, this becomes a
     * visualization tool.
     *
     *******************************************************************/
    void togglePhysics();
    /*******************************************************************
     *
     * Returns the closest object to the given object, and the distance
     * between the two objects.  Note: only "real" objects are tested, i.e.
     * only objects with doPhysics() = true are checked.
     *
     * subj - the subject object (the one we are comparing to the others)
     * distOut - a pointer to the location where the distance will be stored
     *              when the function exits
     *
     *******************************************************************/
    ObjectId getClosestObject(ObjectId subj,double *distOut);
    /*******************************************************************
     *
     * Returns the closest object to the given object, and the distance
     * between the two objects.  Note: only "real" objects are tested, i.e.
     * only objects with doPhysics() = true are checked.
     *
     * subj - the subject object (the one we are comparing to the others)
     * distOut - a pointer to the location where the distance will be stored
     *              when the function exits
     *
     *******************************************************************/
    ObjectId getClosestObject(SketchObject *subj,double *distOut);
private:
    /*******************************************************************
     *
     * Computes collisions between the given objects and applies the
     * resultant forces to them -- subroutine of stepPhysics
     *
     *******************************************************************/
    void collide(SketchObject *o1, SketchObject *o2);

    void updateSprings();

    std::list<SketchObject *> objects;
    std::list<SpringConnection *> connections;
    int nextIdx;
    vtkSmartPointer<vtkRenderer> renderer;
    int lastCapacityUpdate;
    vtkSmartPointer<vtkPoints> springEnds;
    vtkSmartPointer<vtkPolyData> springEndConnections;
    vtkSmartPointer<vtkTubeFilter> tubeFilter;
    vtkSmartPointer<vtkTransform> worldEyeTransform;
    bool pausePhysics;
};

#endif // WORLDMANAGER_H
