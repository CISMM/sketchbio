#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <quat.h>

#include <QList>
#include <QVector>
#include <QHash>
#include <QSharedPointer>

#include <vtkSmartPointer.h>
class vtkRenderer;
class vtkTransform;
class vtkPoints;
class vtkPolyData;
class vtkTubeFilter;
class vtkProjectToPlane;
class vtkActor;

class SketchModel;
class ModelManager;
class SketchObject;
class SpringConnection;
class PhysicsStrategy;
#include "groupidgenerator.h"
#include "objectchangeobserver.h"
#include "physicsstrategyfactory.h"


/*
 * This class contains the data that is in the modeled "world", all the objects and
 * springs.  It also contains code to step the "physics" of the simulation and run
 * collision tests.
 */

class WorldManager : public GroupIdGenerator, public ObjectChangeObserver
{
public:
    explicit WorldManager(vtkRenderer *r);
    virtual ~WorldManager();

    /*******************************************************************
     *
     * Gets the next collision group number
     *
     *******************************************************************/
    virtual int getNextGroupId();

    /*******************************************************************
     *
     * Adds a new object with the given modelId to the world with the
     * given position and orientation  Returns the object created
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
    SketchObject *addObject(SketchModel *model,const q_vec_type pos, const q_type orient);
    /*******************************************************************
     * Adds a the given object to the world and returns it.
     *
     * object - the object to add
     *******************************************************************/
    SketchObject *addObject(SketchObject *object);

    /*******************************************************************
     *
     * Removes the SketchObject identified by the given index from the world.
     * This method does not delete the object, instead ownership of the object
     * is transferred to the caller
     *
     * object - the object, returned by addObject
     *
     *******************************************************************/
    void removeObject(SketchObject *object);

    /*******************************************************************
     *
     * Removes the SketchObject identified by the given index from the world.
     * This method also deletes the object.
     *
     * object - the object, returned by addObject
     *
     *******************************************************************/
    void deleteObject(SketchObject *object);

    /*******************************************************************
     *
     * Returns the number of SketchObjects that are stored in the
     * WorldManager
     *
     *******************************************************************/
    int getNumberOfObjects() const;

    /*******************************************************************
     *
     * Returns an iterator over the SketchObjects that are stored in the
     * WorldManager
     *
     *******************************************************************/
    QListIterator<SketchObject *> getObjectIterator() const;

    /*******************************************************************
     *
     * Returns an the list of SketchObjects that are stored in the
     * WorldManager
     *
     *******************************************************************/
    const QList<SketchObject *> *getObjects() const;

    /*******************************************************************
     *
     * Adds the given spring to the list of springs.  Returns a pointer to
     * the spring (same as the one passed in).
     *
     * This passes ownership of the SpringConnection to the world manager
     * and the spring connection will be deleted in removeSpring
     * or the world manager's destructor
     *
     * spring - the spring to add
     *
     *******************************************************************/
    SpringConnection *addSpring(SpringConnection *spring);
    /*******************************************************************
     *
     * Adds the given spring to the list of springs for the left hand
     *
     * This passes ownership of the SpringConnection to the world manager
     * and the spring connection will be deleted in removeSpring
     * or the world manager's destructor
     *
     * spring - the spring to add
     *
     *******************************************************************/
    inline void addLeftHandSpring(SpringConnection *spring) {addSpring(spring,&lHand); }
    /*******************************************************************
     *
     * Adds the given spring to the list of springs for the right hand
     *
     * This passes ownership of the SpringConnection to the world manager
     * and the spring connection will be deleted in removeSpring
     * or the world manager's destructor
     *
     * spring - the spring to add
     *
     *******************************************************************/
    inline void addRightHandSpring(SpringConnection *spring) {addSpring(spring,&rHand); }
    /*******************************************************************
     *
     * Gets the list of springs for the left hand
     *
     *******************************************************************/
    inline QList<SpringConnection *> *getLeftSprings() { return &lHand; }
    /*******************************************************************
     *
     * Gets the list of springs for the right hand
     *
     *******************************************************************/
    inline QList<SpringConnection *> *getRightSprings() { return &rHand; }
    /*******************************************************************
     *
     * Clears the list of springs for the left hand
     *
     *******************************************************************/
    void clearLeftHandSprings();
    /*******************************************************************
     *
     * Clears the list of springs for the right hand
     *
     *******************************************************************/
    void clearRightHandSprings();

    /*******************************************************************
     *
     * Adds the a spring between the two models and returns a pointer to it.  Returns an iterator
     * to the position of that spring in the list
     *
     * o1 - the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world coordinate space,
     *                  - false if they are relative to the model coordiante space
     * k - the stiffness of the spring
     * minLen - the minimum rest length of the spring
     * maxLen - the maximum rest length of the spring
     *
     *******************************************************************/
    SpringConnection *addSpring(SketchObject *o1, SketchObject *o2, const q_vec_type pos1,
                       const q_vec_type pos2, bool worldRelativePos, double k, double minLen, double maxLen);

    /*******************************************************************
     *
     * Adds the a spring between the two models and returns a pointer to it.  Returns an iterator
     * to the position of that spring in the list
     *
     * o1 - the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world coordinate space,
     *                  - false if they are relative to the model coordiante space
     * k - the stiffness of the spring
     * len - the length of the spring
     *
     *******************************************************************/
    SpringConnection *addSpring(SketchObject *o1, SketchObject *o2, const q_vec_type pos1,
                                       const q_vec_type pos2, bool worldRelativePos, double k, double len);

    /*******************************************************************
     *
     * Removes the given spring from the list of springs
     *
     *******************************************************************/
    void removeSpring(SpringConnection *spring);

    /*******************************************************************
     *
     * Returns the number of springs stored in the spring list
     *
     *******************************************************************/
    int getNumberOfSprings() const;

    /*******************************************************************
     *
     * Returns an iterator over all the springs in the list
     *
     *******************************************************************/
    QListIterator<SpringConnection *> getSpringsIterator() const;
    /*******************************************************************
     *
     * Sets the collision response mode
     *
     *******************************************************************/
    void setCollisionMode(PhysicsMode::Type mode);

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
     * Sets the positions of the objects to their positions at the given
     * time (found from their keyframes).
     * Returns true if all keyframes are at times less than t
     *
     *******************************************************************/
    bool setAnimationTime(double t);
    /*******************************************************************
     *
     * Returns true if object shadows are being shown
     *
     *******************************************************************/
    bool isShowingShadows() const;
    /*******************************************************************
     *
     * Set whether or not to object shadows should be visible
     *
     *******************************************************************/
    void setShowShadows(bool show);
    /*******************************************************************
     *
     * Set object shadows visible
     *
     *******************************************************************/
    void setShadowsOn();
    /*******************************************************************
     *
     * Set object shadows invisible
     *
     *******************************************************************/
    void setShadowsOff();
    /*******************************************************************
     *
     * Should be called to sync states if an object's visibility status
     * is externally modified
     *
     *******************************************************************/
    void changedVisibility(SketchObject *obj);
    /*******************************************************************
     *
     * Returns true if invisible objects are being shown
     *
     *******************************************************************/
    bool isShowingInvisible() const;
    /*******************************************************************
     *
     * Shows all the invisible objects (for edit mode where you should be
     * able to position invisible things like cameras)
     *
     *******************************************************************/
    void showInvisibleObjects();
    /*******************************************************************
     *
     * Hides all the invisible objects (like cameras) for previewing
     * animations
     *
     *******************************************************************/
    void hideInvisibleObjects();
    /*******************************************************************
     *
     * Turns on or off the non-user related spring forces.
     *
     *******************************************************************/
    void setPhysicsSpringsOn(bool on);
    /*******************************************************************
     *
     * Turns on or off the collision tests (when off, objects can pass
     * through each other)
     *
     *******************************************************************/
    void setCollisionCheckOn(bool on);
    /*******************************************************************
     *
     * Returns the closest object to the given object, and the distance
     * between the two objects.
     *
     * subj - the subject object (the one we are comparing to the others)
     * distOut - a pointer to the location where the distance will be stored
     *              when the function exits
     *
     *******************************************************************/
    SketchObject *getClosestObject(SketchObject *subj,double &distOut);
    /*******************************************************************
     *
     * Returns the closest object in the list to the subject object.  This
     * method also returns the distance between the closest and the subject
     * to the last parameter
     *
     * subj - the subject object (the one we are comparing to the others)
     * distOut - a pointer to the location where the distance will be stored
     *              when the function exits
     *
     *******************************************************************/
    static SketchObject *getClosestObject(QList<SketchObject *> &objects,
                                          SketchObject *subj,double &distOut);
    /*******************************************************************
     *
     * Returns the closest spring to the given point, and the 'distance'
     * to the spring.  For this, the spring is defined as a line between
     * its two endpoints.  The distance is the distance from the closest
     * point on this line to the point given.  The boolean return value
     * (parameter) is true if the point is closer to End1 of the spring
     * than to End2.
     *
     *******************************************************************/
    SpringConnection *getClosestSpring(q_vec_type point, double *distOut, bool *closerToEnd1);
    /*******************************************************************
     *
     * This method set the plane used to compute the shadows of objects
     * by specifying a point on the plane and the plane's normal vector.
     *
     *******************************************************************/
    void setShadowPlane(q_vec_type point, q_vec_type nVector);
    /*******************************************************************
     *
     * This method is called whenever a subobject is added to a group
     * in the world and is used to keep the graphics state consistent with
     * the model state.
     *
     *******************************************************************/
    virtual void subobjectAdded(SketchObject *parent, SketchObject *child);
    /*******************************************************************
     *
     * This method is called whenever a subobject is removed from a group
     * in the world and is used to keep the graphics state consistent with
     * the model state.
     *
     *******************************************************************/
    virtual void subobjectRemoved(SketchObject *parent, SketchObject *child);
private:
    /*******************************************************************
     *
     * This method adds the spring to the given list and performs the
     * other steps necessary for the spring to be shown onscreen
     *
     *******************************************************************/
    void addSpring(SpringConnection *spring, QList<SpringConnection *> *list);

#ifdef SHOW_DEBUGGING_FORCE_LINES
    /*******************************************************************
     *
     * This method updates the spring endpoints and removes springs that
     * have been deleted from the shown springs
     *
     *******************************************************************/
    void updateSprings();
#endif

    /*******************************************************************
     *
     * This method recursively adds the actors for the given object to
     * the renderer, handling both single objects and groups
     *
     *******************************************************************/
    void insertActors(SketchObject *obj);
    /*******************************************************************
     *
     * This method recursively removes the actors for the given object to
     * the renderer, handling both single objects and groups
     *
     *******************************************************************/
    void removeActors(SketchObject *obj);
    /*******************************************************************
     *
     * This method recursively removes the shadows of a given object
     * from the datastructure prior to the object being deleted.
     *
     *******************************************************************/
    void removeShadows(SketchObject *obj);

    QList<SketchObject *> objects;
    QHash<SketchObject *, QPair< vtkSmartPointer< vtkProjectToPlane >,
        vtkSmartPointer< vtkActor > > > shadows;
    QList<SpringConnection *> connections, lHand, rHand;
    QVector<QSharedPointer<PhysicsStrategy> > strategies;
    int nextIdx;
    vtkSmartPointer<vtkRenderer> renderer;
    int lastCapacityUpdate;
#ifdef SHOW_DEBUGGING_FORCE_LINES
    vtkSmartPointer<vtkPoints> springEnds;
    vtkSmartPointer<vtkPolyData> springEndConnections;
    vtkSmartPointer<vtkTubeFilter> tubeFilter;
#endif
    int maxGroupNum;
    bool doPhysicsSprings, doCollisionCheck, showInvisible, showShadows;
    PhysicsMode::Type collisionResponseMode;
};

inline SpringConnection *WorldManager::addSpring(SketchObject *o1, SketchObject *o2, const q_vec_type pos1,
                               const q_vec_type pos2, bool worldRelativePos, double k, double len) {
    return addSpring(o1,o2,pos1,pos2,worldRelativePos,k,len,len);
}

inline const QList<SketchObject *> *WorldManager::getObjects() const {
    return &objects;
}

inline SketchObject *WorldManager::getClosestObject(SketchObject *subj, double &distOut)
{
    return getClosestObject(objects,subj,distOut);
}


#endif // WORLDMANAGER_H
