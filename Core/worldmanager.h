#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <quat.h>

#include <QList>
#include <QVector>
#include <QHash>
#include <QSharedPointer>

#include <vtkSmartPointer.h>
class vtkRenderer;
class vtkLineSource;
class vtkTransform;
class vtkPoints;
class vtkPolyData;
class vtkTubeFilter;
class vtkActor;
class vtkAppendPolyData;

class vtkProjectToPlane;

class SketchModel;
class ModelManager;
class SketchObject;
class Connector;
class SpringConnection;
class PhysicsStrategy;
#include "groupidgenerator.h"
#include "objectchangeobserver.h"
#include "physicsstrategyfactory.h"

class WorldObserver
{
   public:
    virtual ~WorldObserver() {}
    // will be called on the observer when an object is added.  The parameter
    // is the new object
    virtual void objectAdded(SketchObject *) {}
    // will be called on the observer when an object is removed, but before
    // the object is deleted (if delete object called).  The parameter is
    // the object that was removed.  Note that if called as part of
    // deleteObject, the pointer may not be valid after objectRemoved returns
    // and deleteObject finishes
    virtual void objectRemoved(SketchObject *) {}
    // will be called on the observer when the physics springs active state
    // changes.  The parameter will be true if the springs are active
    // after the change
    virtual void springActivationChanged() {}
    // will be called on the observer when the collision detection active
    // state changes.  The parameter will be true if the collision detection
    // is on after the change
    virtual void collisionDetectionActivationChanged() {}
};

/*
 * This class contains the data that is in the modeled "world", all the objects
 * and
 * springs.  It also contains code to step the "physics" of the simulation and
 * run
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
     * The new object is owned by the WorldManager
     *
     * model   - the model to use when generating the object,
     *           should be a valid modelId with the model manager
     * pos     - the position at which the object should be added
     * orient  - the orientation of the object when it is added
     *
     *******************************************************************/
    SketchObject *addObject(SketchModel *model, const q_vec_type pos,
                            const q_type orient);
    /*******************************************************************
     * Adds a the given object to the world and returns it.
     *
     * object - the object to add
     *******************************************************************/
    SketchObject *addObject(SketchObject *object);

    /*******************************************************************
     *
     * Removes the SketchObject identified by the given index from the world.
     *
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
     *
     * This method also deletes the object, the pointer passed in will point
     * to freed memory after this call
     *
     * object - the object, returned by addObject
     *
     *******************************************************************/
    void deleteObject(SketchObject *object);

    /*******************************************************************
 *
 * Takes an object and either removes it from its group or adds it to
 * one if necessary based on the animation time. Also does the same
     * for its subobjects recursively.
 *
 *******************************************************************/
    void updateGroupStatus(SketchObject *object, double t,
                           double lastGroupUpdate);

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
    QListIterator< SketchObject * > getObjectIterator() const;

    /*******************************************************************
     *
     * Returns an the list of SketchObjects that are stored in the
     * WorldManager
     *
     *******************************************************************/
    const QList< SketchObject * > *getObjects() const;

    /*******************************************************************
     *
     * Clears all the objects stored in the WorldManager (removes and
     * deletes them)
     *
     *******************************************************************/
    void clearObjects();

    /*******************************************************************
     *
     * Adds the given spring to the list of springs.  Returns a pointer to
     * the spring (same as the one passed in).
     *
     * This passes ownership of the Connector to the world manager
     * and the spring connection will be deleted in removeSpring
     * or the world manager's destructor
     *
     * spring - the spring to add
     *
     *******************************************************************/
    Connector *addConnector(Connector *spring);

    /*******************************************************************
     *
     * Adds the given spring to the list of springs for the user interface
     *
     * This does NOT pass ownership of the Connector to the world manager
     *
     * spring - the spring to add
     *
     *******************************************************************/
    inline void addUISpring(Connector *spring)
    {
        addConnector(spring, uiSprings);
    }
    /*******************************************************************
     *
     * Gets the list of connectors (springs) for the left hand
     *
     *******************************************************************/
    inline const QList< Connector * > *getUISprings() const
    {
        return &uiSprings;
    }
    /*******************************************************************
     *
     * Removes the connector from the user interface connectors
     *
     *******************************************************************/
    void removeUISpring(Connector *spring);

    /*******************************************************************
     *
     * Clears all the connectors in the WorldManager (removes and deletes them)
     *
     *******************************************************************/
    void clearConnectors();
    /*******************************************************************
     *
     * Adds the a spring between the two models and returns a pointer to it.
     *
     * The created spring is owned by the WorldManager
     *
     * o1 - the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world
     *coordinate space,
     *                  - false if they are relative to the model coordiante
     *space
     * k - the stiffness of the spring
     * minLen - the minimum rest length of the spring
     * maxLen - the maximum rest length of the spring
     *
     *******************************************************************/
    SpringConnection *addSpring(SketchObject *o1, SketchObject *o2,
                                const q_vec_type pos1, const q_vec_type pos2,
                                bool worldRelativePos, double k, double minLen,
                                double maxLen);

    /*******************************************************************
     *
     * Adds the a spring between the two models and returns a pointer to it.
     *
     * The created spring is owned by the WorldManager
     *
     * o1 - the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world
     *coordinate space,
     *                  - false if they are relative to the model coordiante
     *space
     * k - the stiffness of the spring
     * len - the length of the spring
     *
     *******************************************************************/
    SpringConnection *addSpring(SketchObject *o1, SketchObject *o2,
                                const q_vec_type pos1, const q_vec_type pos2,
                                bool worldRelativePos, double k, double len);

    /*******************************************************************
     *
     * Removes the given spring from the list of springs and deletes it
     *
     * The pointer passed in will be invalid after this call, having been
     * freed
     *
     *******************************************************************/
    void removeSpring(Connector *spring);

    /*******************************************************************
     *
     * Returns the number of springs stored in the spring list
     *
     *******************************************************************/
    int getNumberOfConnectors() const;

    /*******************************************************************
     *
     * Returns a const reference to the list of springs
     *
     *******************************************************************/
    const QList< Connector * > &getSprings() const;
    /*******************************************************************
     *
     * Returns an iterator over all the springs in the list
     *
     *******************************************************************/
    QListIterator< Connector * > getSpringsIterator() const;
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
     * Turns on or off the keyframe outlines based on the given time.  If
     * an object has keyframes at the given time, then the keyframe outlines
     * will be made visible around that object.  This function, unlike
     * setAnimationTime() does not modify object positions.
     *
     *******************************************************************/
    void setKeyframeOutlinesForTime(double t);
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
     * Returns true if the non-user related spring forces are enabled
     *
     *******************************************************************/
    bool areSpringsEnabled();
    /*******************************************************************
     *
     * Turns on or off the collision tests (when off, objects can pass
     * through each other)
     *
     *******************************************************************/
    void setCollisionCheckOn(bool on);
    /*******************************************************************
     *
     * Returns true if the collision testing is on
     *
     *******************************************************************/
    bool isCollisionTestingOn();
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
    SketchObject *getClosestObject(SketchObject *subj, double &distOut);
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
    static SketchObject *getClosestObject(QList< SketchObject * > &objects,
                                          SketchObject *subj, double &distOut);
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
    Connector *getClosestConnector(q_vec_type point, double *distOut,
                                   bool *closerToEnd1);
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
    /*******************************************************************
     *
     * Called whenever the visibility of an object changes
     *
     *******************************************************************/
    virtual void objectVisibilityChanged(SketchObject *obj);
    /*******************************************************************
     *
     * Adds an observer to the world manager
     *
     *******************************************************************/
    void addObserver(WorldObserver *w);
    /*******************************************************************
     *
     * Removes an observer to the world manager
     *
     *******************************************************************/
    void removeObserver(WorldObserver *w);

   private:
    /*******************************************************************
     *
     * This method adds the spring to the given list and performs the
     * other steps necessary for the connector to be shown onscreen
     *
     *******************************************************************/
    void addConnector(Connector *spring, QList< Connector * > &list);

    /*******************************************************************
     *
     * This method updates the connector endpoints for the current frame
     *
     *******************************************************************/
    void updateConnectors();

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

    typedef QPair< vtkSmartPointer< vtkProjectToPlane >,
                   vtkSmartPointer< vtkActor > > ShadowPair;
    typedef QPair< vtkSmartPointer< vtkLineSource >,
                   vtkSmartPointer< vtkActor > > ConnectorPair;

    QList< SketchObject * > objects;
    QHash< SketchObject *, ShadowPair > shadows;
    QHash< Connector *, ConnectorPair > lines;
    QList< Connector * > connections, uiSprings;
    QVector< QSharedPointer< PhysicsStrategy > > strategies;

    vtkSmartPointer< vtkRenderer > renderer;
    vtkSmartPointer< vtkAppendPolyData > orientedHalfPlaneOutlines;
    vtkSmartPointer< vtkActor > halfPlanesActor;

    int maxGroupNum;
    bool doPhysicsSprings, doCollisionCheck, showInvisible, showShadows;
    PhysicsMode::Type collisionResponseMode;

    double lastGroupUpdate;
    QList< WorldObserver * > observers;
};

inline SpringConnection *WorldManager::addSpring(
    SketchObject *o1, SketchObject *o2, const q_vec_type pos1,
    const q_vec_type pos2, bool worldRelativePos, double k, double len)
{
    return addSpring(o1, o2, pos1, pos2, worldRelativePos, k, len, len);
}

inline const QList< SketchObject * > *WorldManager::getObjects() const
{
    return &objects;
}

inline const QList< Connector * > &WorldManager::getSprings() const
{
    return connections;
}

inline SketchObject *WorldManager::getClosestObject(SketchObject *subj,
                                                    double &distOut)
{
    return getClosestObject(objects, subj, distOut);
}

#endif  // WORLDMANAGER_H
