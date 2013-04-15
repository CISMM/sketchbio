#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <vtkSmartPointer.h>
#include <vtkAppendPolyData.h>
#include <vtkTransform.h>
#include <vtkActor.h>
#include <quat.h>
#include <sketchmodel.h>
#include <QList>
#include <QScopedPointer>
#include <QMap>
#include <keyframe.h>

// forward declare the collision handler so it can be passed to collide
class PhysicsStrategy;
// forward declare observer of objects moving so it each object can have a list of observers
// (declared at the bottom of this file)
class ObjectForceObserver;


// used by getPrimaryCollisionGroupNum to indicate that the object has not been
// assigned a group
#define OBJECT_HAS_NO_GROUP (-1)

/*
 * This class is an abstract representation of some instance or group of instances
 * in the world that the user would like to treat as a single object in terms of
 * position/orientation and movement relative to other things.
 *
 * This class is the abstract superclass for the Composite Pattern, although the
 * data that will be the same for both instances and groups is stored and accessed
 * from here, such as position/orientation and force/torque.
 */

class SketchObject {
public:
    // constructor
    SketchObject();
    virtual ~SketchObject();
    // the number of instances controlled by this object (is single or group?)
    virtual int numInstances() const =0;
    // the parent "object"/group
    virtual SketchObject *getParent();
    virtual const SketchObject *getParent() const;
    virtual void setParent(SketchObject *p);
    // get the list of child objects
    virtual QList<SketchObject *> *getSubObjects();
    virtual const QList<SketchObject *> *getSubObjects() const;
    // model information - NULL by default since not all objects will have one model
    // however, these must be implemented to return a valid SketchModel if numInstances returns 1
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    // actor - NULL by default since not all objects will have one actor, but must return a valid
    // actor if numInstances is 1
    virtual vtkActor *getActor();
    // position and orientation
    void getPosition(q_vec_type dest) const;
    void getOrientation(q_type dest) const;
    void getOrientation(PQP_REAL matrix[3][3]) const;
    void setPosition(const q_vec_type newPosition);
    void setOrientation(const q_type newOrientation);
    void setPosAndOrient(const q_vec_type newPosition, const q_type newOrientation);
    // to do with setting undo position
    void getLastPosition(q_vec_type dest) const;
    void getLastOrientation(q_type dest) const;
    void setLastLocation();
    void restoreToLastLocation();
    // to do with collision groups (see comments on the collisionGroups field for details of
    // implmentation)
    int  getPrimaryCollisionGroupNum();
    void setPrimaryCollisionGroupNum(int num);
    void addToCollisionGroup(int num);
    bool isInCollisionGroup(int num) const;
    void removeFromCollisionGroup(int num);
    // local transformation & transforming points/vectors
    vtkTransform *getLocalTransform();
    virtual void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const;
    virtual void getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const;
    // add, get, set and clear forces and torques on the object
    virtual void addForce(const q_vec_type point, const q_vec_type force);
    virtual void getForce(q_vec_type out) const;
    virtual void getTorque(q_vec_type out) const;
    virtual void setForceAndTorque(const q_vec_type force, const q_vec_type torque);
    virtual void clearForces();
    // collision with other.  The physics strategy will get the data about the collision
    // and decide how to respond. The bool return value is true iff there was a collision
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags = PQP_ALL_CONTACTS) =0;
    // bounding box info for grab (have to stop using PQP_Distance)
    virtual void getAABoundingBox(double bb[6]) = 0; // TODO - may just get rid of this one
    // this returns the box(es) that contain the lowest-level objects in whatever heirarchy
    // group should do an AppendPolyData to combine these
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes() = 0;
    // to deal with force observers
    void addForceObserver(ObjectForceObserver *obs);
    void removeForceObserver(ObjectForceObserver *obs);
    // to stop the object from updating its transform when its position/orientation are set
    // used when an external source is defining the object's local transformation
    void setLocalTransformPrecomputed(bool isComputed);
    bool isLocalTransformPrecomputed();
    // to set whether or not this object's local transformation has more information about
    // its position than the position vector
    void setLocalTransformDefiningPosition(bool isDefining);
    bool isLocalTransformDefiningPosition();
    // to allow localTransformUpdated to be called from subclasses on other SketchObjects...
    static void localTransformUpdated(SketchObject *obj);
    // methods for accessing and modifying keyframes
    bool hasKeyframes() const;
    int  getNumKeyframes() const;
    const QMap< double, Keyframe > *getKeyframes() const;
    void addKeyframeForCurrentLocation(double t);
    void removeKeyframeForTime(double t);
    void setPositionByAnimationTime(double t);
    // visibility methods
    virtual void setIsVisible(bool isVisible);
    bool isVisible() const;
protected: // methods
    // to deal with local transformation - recomputes from position and orientation unless
    // isLocalTransformPrecomputed is true
    void recalculateLocalTransform();
    // to be overridden as a template method if something needs to occur when the transform is updated
    virtual void localTransformUpdated();
protected: // fields
    vtkSmartPointer<vtkTransform> localTransform;
private: // methods
    void notifyObservers();
private: // fields
    SketchObject *parent;
    q_vec_type forceAccum,torqueAccum;
    q_vec_type position, lastPosition;
    q_type orientation, lastOrientation;
    // visibility for animations:
    bool visible;
    // this list is the collision groups. If it is empty, then the object has no collision group and
    // getPrimaryCollisionGroup will return OBJECT_HAS_NO_GROUP.  Else, the primary collision group
    // is ther first element in the list.  setPrimaryCollisionGroup will move the given group to the
    // start of the list (or add it if it is not there already).  addToCollisionGroup appends onto the
    // list
    QList<int> collisionGroups;
    bool localTransformPrecomputed, localTransformDefiningPosition;
    QList<ObjectForceObserver *> observers;
    // this smart pointer contains the keyframes of the object.  If the pointer it contains is null, then
    // there are no keyframes.  Otherwise, the map it points to is a mapping from time to frame where frame
    // contains all the information about what happens at that time (position, orientation, visibility, etc.)
    QScopedPointer< QMap< double, Keyframe > > keyframes;
};

/*
 * This class extends SketchObject to provide an object that is a single instance of a
 * single SketchModel's data.
 */

class ModelInstance : public SketchObject {
public:
    // constructor
    ModelInstance(SketchModel *m);
    virtual ~ModelInstance();
    // specify that this is a leaf by returning 1
    virtual int numInstances() const;
    // getters for data this subclass holds
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    virtual vtkActor *getActor();
    // collision function that depend on data in this subclass
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags = PQP_ALL_CONTACTS);
    virtual void getAABoundingBox(double bb[]);
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes();
protected:
    virtual void localTransformUpdated();
private:
    vtkSmartPointer<vtkActor> actor;
    SketchModel *model;
    vtkSmartPointer<vtkTransformPolyDataFilter> modelTransformed;
    vtkSmartPointer<vtkTransformPolyDataFilter> orientedBB;
    vtkSmartPointer<vtkPolyDataMapper> solidMapper;
};

class ObjectGroup : public SketchObject {
public:
    // constructor
    ObjectGroup();
    // destructor
    virtual ~ObjectGroup();
    // specify this is not a leaf (returns -1 * number of objects)
    virtual int numInstances() const;
    // if numInstances is 1, must provide model and actor, otherwise return null
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    virtual vtkActor *getActor();
    // methods to add/remove objects
    // note: this method gives ObjectGroup ownership until the object is removed
    // so these objects are cleaned up in ObjectGroup's destructor
    void addObject(SketchObject *obj);
    // when the object is removed it is no longer owned by ObjectGroup, the remover is responsible
    // for ensuring it is deallocated
    void removeObject(SketchObject *obj);
    // get the list of child objects
    virtual QList<SketchObject *> *getSubObjects();
    virtual const QList<SketchObject *> *getSubObjects() const;
    // collision function... have to change declaration
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags = PQP_ALL_CONTACTS);
    virtual void getAABoundingBox(double bb[]);
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes();
    virtual void setIsVisible(bool isVisible);
protected:
    virtual void localTransformUpdated();
private:
    QList<SketchObject *> children;
    vtkSmartPointer<vtkAppendPolyData> orientedBBs;
};

/*
 * This class is an observer that needs to be notified when the object has a force/torque applied to it
 */
class ObjectForceObserver {
public:
    virtual ~ObjectForceObserver() {}
    virtual void objectPushed(SketchObject *obj) = 0;
};

// helper function-- converts quaternion to a PQP rotation matrix
inline void quatToPQPMatrix(const q_type quat, PQP_REAL mat[3][3]) {
    q_matrix_type rowMat;
    q_to_col_matrix(rowMat,quat);
    for (int i = 0; i < 3; i++) {
        mat[i][0] = rowMat[i][0];
        mat[i][1] = rowMat[i][1];
        mat[i][2] = rowMat[i][2];
    }
}
#endif // SKETCHOBJECT_H
