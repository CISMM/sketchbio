#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <map>
#include <QList>
#include <QVector>
#include <QSharedPointer>
#include "sketchobject.h"
#include "modelmanager.h"
#include "springconnection.h"
#include "groupidgenerator.h"
#include <physicsstrategy.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkTubeFilter.h>

/*
 * For testing only: dynamic changing of collision response types. Each enum is a
 * response strategy
 */
namespace CollisionMode {
    enum Type {
        ORIGINAL_COLLISION_RESPONSE=0,
        POSE_MODE_TRY_ONE=1,
        BINARY_COLLISION_SEARCH=2,
        POSE_WITH_PCA_COLLISION_RESPONSE=3
    };
    /*******************************************************************
     *
     * This method populates the list of collision strategies so that their
     * index in the list is the same as the value of the Type enum that
     * should trigger them.
     *
     *******************************************************************/
    inline void populateStrategies(QVector<QSharedPointer<PhysicsStrategy> > &strategies) {
        strategies.clear();
        QSharedPointer<PhysicsStrategy> s1(new SimplePhysicsStrategy());
        strategies.append(s1);
        QSharedPointer<PhysicsStrategy> s2(new PoseModePhysicsStrategy());
        strategies.append(s2);
        QSharedPointer<PhysicsStrategy> s3(new BinaryCollisionSearchStrategy());
        strategies.append(s3);
        QSharedPointer<PhysicsStrategy> s4(new PoseModePCAPhysicsStrategy());
        strategies.append(s4);
    }
}

/*
 * This class contains the data that is in the modeled "world", all the objects and
 * springs.  It also contains code to step the "physics" of the simulation and run
 * collision tests.
 */

class WorldManager : public GroupIdGenerator
{
public:
    WorldManager(vtkRenderer *r, vtkTransform *worldEyeTransform);
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
     * Removes the SketchObject identified by the given index from the world
     *
     * object - the object, returned by addObject
     *
     *******************************************************************/
    void removeObject(SketchObject *object);

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
    void setCollisionMode(CollisionMode::Type mode);

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
     * between the two objects.  Note: only "real" objects are tested, i.e.
     * only objects with doPhysics() = true are checked.
     *
     * subj - the subject object (the one we are comparing to the others)
     * distOut - a pointer to the location where the distance will be stored
     *              when the function exits
     *
     *******************************************************************/
    SketchObject *getClosestObject(SketchObject *subj,double *distOut);
    /*******************************************************************
     *
     * Computes collision response force between the two objects based on
     * the given collision data.  Only applies collision response to the
     * objects whose primary group numbers are in affectedGroups.  This
     * function applies collision force along the normal of a plane fitted
     * to the corners of all triangles that are in collision.
     *
     * However, if affectedGroups is empty it assumes we are doing non-pose
     * mode physics and applies the collision response to both objects.
     *
     * o1 - the first object (first passed to SketchObject::collide)
     * o2 - the second object (second passed to SketchObject::collide)
     * cr - the result of colliding the objects (result of SketchObject::collide)
     * affectedGroups - the group numbers that collision response should be applied to
     *
     *******************************************************************/
    static void applyPCACollisionResponseForce(SketchObject *o1, SketchObject *o2,
                                            PQP_CollideResult *cr, QSet<int> &affectedGroups);
    /*******************************************************************
     *
     * Computes collision response force between the two objects based on
     * the given collision data.  Only applies collision response to the
     * objects whose primary group numbers are in affectedGroups.  This
     * function applies collision force along the normals of the colliding
     * pairs of triangles
     *
     * However, if affectedGroups is empty it assumes we are doing non-pose
     * mode physics and applies the collision response to both objects.
     *
     * o1 - the first object (first passed to SketchObject::collide)
     * o2 - the second object (second passed to SketchObject::collide)
     * cr - the result of colliding the objects (result of SketchObject::collide)
     * affectedGroups - the group numbers that collision response should be applied to
     *
     *******************************************************************/
    static void applyCollisionResponseForce(SketchObject *o1, SketchObject *o2,
                                            PQP_CollideResult *cr, QSet<int> &affectedGroups);
private:
    /*******************************************************************
     *
     * This method adds the spring to the given list and performs the
     * other steps necessary for the spring to be shown onscreen
     *
     *******************************************************************/
    void addSpring(SpringConnection *spring, QList<SpringConnection *> *list);

    /*******************************************************************
     *
     * This method updates the spring endpoints and removes springs that
     * have been deleted from the shown springs
     *
     *******************************************************************/
    void updateSprings();

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

    QList<SketchObject *> objects;
    QList<SpringConnection *> connections, lHand, rHand;
    QVector<QSharedPointer<PhysicsStrategy> > strategies;
    int nextIdx;
    vtkSmartPointer<vtkRenderer> renderer;
    int lastCapacityUpdate;
    vtkSmartPointer<vtkPoints> springEnds;
    vtkSmartPointer<vtkPolyData> springEndConnections;
    vtkSmartPointer<vtkTubeFilter> tubeFilter;
    vtkSmartPointer<vtkTransform> worldEyeTransform;
    int maxGroupNum;
    bool doPhysicsSprings, doCollisionCheck, showInvisible;
    CollisionMode::Type collisionResponseMode;
};

inline SpringConnection *WorldManager::addSpring(SketchObject *o1, SketchObject *o2, const q_vec_type pos1,
                               const q_vec_type pos2, bool worldRelativePos, double k, double len) {
    return addSpring(o1,o2,pos1,pos2,worldRelativePos,k,len,len);
}

inline const QList<SketchObject *> *WorldManager::getObjects() const {
    return &objects;
}


#endif // WORLDMANAGER_H
