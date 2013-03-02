#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkActor.h>
#include <quat.h>
#include <list>
#include <sketchmodel.h>

// forward declare the collision handler so it can be passed to collide
class PhysicsStrategy;


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
    virtual int numInstances() =0;
    // the parent "object"/group
    virtual SketchObject *getParent();
    virtual void setParent(SketchObject *p);
    // model information - NULL by default since not all objects will have one model
    // however, these must be implemented to return a valid SketchModel if numInstances returns 1
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    // actor - NULL by default since not all objects will have one actor
    virtual vtkActor *getActor();
    // position and orientation
    virtual void getPosition(q_vec_type dest) const;
    virtual void getOrientation(q_type dest) const;
    virtual void getOrientation(PQP_REAL matrix[3][3]) const;
    virtual void setPosition(const q_vec_type newPosition);
    virtual void setOrientation(const q_type newOrientation);
    virtual void setPosAndOrient(const q_vec_type newPosition, const q_type newOrientation);
    // to do with setting undo position
    void getLastPosition(q_vec_type dest) const;
    void getLastOrientation(q_type dest) const;
    void setLastLocation();
    void restoreToLastLocation();
    // to do with collision groups
    virtual int  getPrimaryCollisionGroupNum();
    virtual void setPrimaryCollisionGroupNum(int num);
    virtual bool isInCollisionGroup(int num) const;
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
    // wireframe vs. solid
    virtual void setWireFrame() =0;
    virtual void setSolid() =0;
    // collision with other.  The physics strategy will get the data about the collision
    // and decide how to respond. The bool return value is true iff there was a collision
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags = PQP_ALL_CONTACTS) =0;
    // bounding box info for grab (have to stop using PQP_Distance)
    virtual void getAABoundingBox(double bb[6]) = 0;
protected: // methods
    // to deal with local transformation
    void recalculateLocalTransform();
    void setLocalTransformPrecomputed(bool isComputed);
    bool isLocalTransformPrecomputed();
    // to be overridden as a template method if something needs to occur when the transform is updated
    virtual void localTransformUpdated();
    // to allow localTransformUpdated to be called from subclasses on other SketchObjects...
    static void localTransformUpdated(SketchObject *obj);
protected: // fields
    vtkSmartPointer<vtkTransform> localTransform;
private: // fields
    SketchObject *parent;
    q_vec_type forceAccum,torqueAccum;
    q_vec_type position, lastPosition;
    q_type orientation, lastOrientation;
    int primaryCollisionGroup;
    bool localTransformPrecomputed;
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
    virtual int numInstances();
    // getters for data this subclass holds
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    virtual vtkActor *getActor();
    // functions that the parent can't implement
    virtual void setWireFrame();
    virtual void setSolid();
    // collision function that depend on data in this subclass
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags = PQP_ALL_CONTACTS);
    virtual void getAABoundingBox(double bb[]);
protected:
    virtual void localTransformUpdated();
private:
    vtkSmartPointer<vtkActor> actor;
    SketchModel *model;
    vtkSmartPointer<vtkTransformPolyDataFilter> modelTransformed;
    vtkSmartPointer<vtkPolyDataMapper> solidMapper;
    vtkSmartPointer<vtkPolyDataMapper> wireframeMapper;
};

class ObjectGroup : public SketchObject {
public:
    // constructor
    ObjectGroup();
    // destructor
    virtual ~ObjectGroup();
    // specify this is not a leaf (returns -1 * number of objects)
    virtual int numInstances();
    // if numInstances is 1, must provide model in both these cases...
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    // methods to add/remove objects
    // note: this method gives ObjectGroup ownership until the object is removed
    // so these objects are cleaned up in ObjectGroup's destructor
    void addObject(SketchObject *obj);
    // when the object is removed it is no longer owned by ObjectGroup, the remover is responsible
    // for ensuring it is deallocated
    void removeObject(SketchObject *obj);
    // get the list of objects
    const QList<SketchObject *> *getSubObjects() const;
    // recursive on group functions
    virtual void setWireFrame();
    virtual void setSolid();
    // collision function... have to change declaration
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags = PQP_ALL_CONTACTS);
    virtual void getAABoundingBox(double bb[]);
protected:
    virtual void localTransformUpdated();
private:
    QList<SketchObject *> children;
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
