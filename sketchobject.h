#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkActor.h>
#include <quat.h>
#include <list>
#include <sketchmodel.h>


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
    // the number of instances controlled by this object (is single or group?)
    virtual int numInstances() =0;
    // the parent "object"/group
    virtual SketchObject *getParent();
    virtual void setParent(SketchObject *p);
    // model information
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    // actor
    virtual vtkActor *getActor();
    // position and orientation
    virtual void getPosition(q_vec_type dest) const;
    virtual void getOrientation(q_type dest) const;
    virtual void getOrientation(PQP_REAL matrix[3][3]) const;
    virtual void setPosition(const q_vec_type newPosition);
    virtual void setOrientation(const q_type newOrientation);
    virtual void setPosAndOrient(const q_vec_type newPosition, const q_type newOrientation);
    // to do with setting undo position
    virtual void getLastPosition(q_vec_type dest) const;
    virtual void getLastOrientation(q_type dest) const;
    virtual void setLastLocation();
    virtual void restoreToLastLocation();
    // to do with collision groups
    virtual int  getPrimaryCollisionGroupNum();
    virtual void setPrimaryCollisionGroupNum(int num);
    virtual bool isInCollisionGroup(int num) const;
    // local transformation & transforming points/vectors
    virtual vtkTransform *getLocalTransform();
    virtual void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const;
    virtual void getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const;
    // force and torque on the object
    virtual void addForce(const q_vec_type point, const q_vec_type force);
    virtual void getForce(q_vec_type out) const;
    virtual void getTorque(q_vec_type out) const;
    virtual void setForceAndTorque(const q_vec_type force, const q_vec_type torque);
    virtual void clearForces();
    // wireframe vs. solid
    virtual void setWireFrame() =0;
    virtual void setSolid() =0;
    // collision with other... ?
    virtual PQP_CollideResult *collide(SketchObject *other, int pqp_flags = PQP_ALL_CONTACTS) =0; // signature may change
    // bounding box info for grab (have to stop using PQP_Distance)
    virtual void getAABoundingBox(double bb[6]) = 0;
protected: // methods
    virtual void recalculateLocalTransform();
    virtual void setLocalTransformPrecomputed(bool isComputed);
    virtual bool isLocalTransformPrecomputed();
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
    virtual PQP_CollideResult *collide(SketchObject *other, int pqp_flags = PQP_ALL_CONTACTS);
    virtual void getAABoundingBox(double bb[]);
private:
    vtkSmartPointer<vtkActor> actor;
    SketchModel *model;
    vtkSmartPointer<vtkTransformPolyDataFilter> modelTransformed;
    vtkSmartPointer<vtkPolyDataMapper> solidMapper;
    vtkSmartPointer<vtkPolyDataMapper> wireframeMapper;
};

/*
 * This class contains the data that is unique to each instance of an object
 * such as position, orientation, the vtkActor specific to the object, etc.
 *
 * It also contains the force and torque accumulators that are used in the
 * main physics loop to count how much force and torque are applied to the
 * object.
 * /


class SketchObject
{
public:
    / *********************************************************************
      *
      * Creates a new SketchObject from the given parameters.  The initial
      * position and orientation of the object are identity.
      *
      * actor - the actor the new object stores physics data for
      * model - the model id of the model this object is based on
      * worldEyeTransform - the transformation manager's world-eye matrix
      *                     used to set up the actor's transform
      *
      ******************************************************************** /
    SketchObject(vtkActor *actor, SketchModel *model,vtkTransform *worldEyeTransform);

    / *********************************************************************
      *
      * Returns the model used to create this object
      *
     ********************************************************************* /
    inline SketchModel *getModel() { return modelId; }
    / *********************************************************************
      *
      * Returns a const pointer to the model used to create this object
      *
     ********************************************************************* /
    inline const SketchModel *getConstModel() const { return modelId; }
    / *********************************************************************
      *
      * Returns the actor this object stores physics data for
      *
     ********************************************************************* /
    inline vtkActor *getActor() { return actor; }
    / *********************************************************************
      *
      * Copies the position of the object into the given vector
      *
     ********************************************************************* /
    virtual void getPosition(q_vec_type dest) const;
    / *********************************************************************
      *
      * Copies the old position of the object into the given vector
      *
     ********************************************************************* /
    inline void getLastPosition(q_vec_type dest) const { q_vec_copy(dest,lastPosition); }
    / *********************************************************************
      *
      * Copies the orientation of the object into the given quaternion
      *
     ********************************************************************* /
    virtual void getOrientation(q_type dest) const;
    / *********************************************************************
      *
      * Copies the old orientation of the object into the given quaternion
      *
     ********************************************************************* /
    inline void getLastOrientation(q_type dest) const { q_copy(dest,lastOrientation); }
    / *********************************************************************
      *
      * Saves the current position and orientation of the object to the
      * lastPosition and lastOrientation
      *
     ********************************************************************* /
    void setLastLocation();
    / *********************************************************************
      *
      * Restores the values saved by setLastLocation to be the current
      * position and orientation of the object
      *
     ********************************************************************* /
    void restoreToLastLocation();
    / *********************************************************************
      *
      * Sets the orientation of the object to the given new orientation
      *
     ********************************************************************* /
    inline void setOrientation(const q_type newOrientation) { q_copy(orientation, newOrientation); }
    / *********************************************************************
      *
      * Sets the position of the object to the given new position
      *
     ********************************************************************* /
    inline void setPosition(const q_vec_type newPosition) { q_vec_copy(position, newPosition); }
    / *********************************************************************
      *
      * Sets the primary group number of the object... may be overridden to do nothing if
      * primary group is determined some other way
      *
     ********************************************************************* /
    virtual void setPrimaryGroupNum(int num);
    / *********************************************************************
      *
      * Gets the primary group number of the object, may be overridden to
      * define an algorithmic group number assignment
      *
     ********************************************************************* /
    virtual int getPrimaryGroupNum() const;
    / *********************************************************************
      *
      * Returns true if the object is in the given group.  Objects may be
      * in multiple groups (via subclassing and overriding this), but
      * by default this simply returns true if the group passed in is the
      * object's primary group.  An object should be in a group IFF moving
      * one object with that primary group number causes this object to move.
      *
     ********************************************************************* /
    virtual bool isInGroup(int num) const;
    / *********************************************************************
      *
      * Gets the transformation within world coordinates that defines this
      * object's position and orientation
      *
     ********************************************************************* /
    inline vtkTransform *getLocalTransform() { return localTransform.GetPointer(); }
    / *********************************************************************
      *
      * Recalculates the object's transformation within world coordinates
      * based on its position and orientation if this updating has not been
      * disabled
      *
     ********************************************************************* /
    void recalculateLocalTransform();
    / *********************************************************************
      *
      * Enables/disables updating of this object's local transformation
      *
      * Note: These should only be disabled if the local transform has
      *     been redefined to update based on something else
      *
     ********************************************************************* /
    inline void allowLocalTransformUpdates(bool allow) { allowTransformUpdate = allow; }
    / *********************************************************************
      *
      * Gets the corresponding world coordinates for the given point in the
      * model's coordinate system
      *
      * modelPoint - the modelspace point given
      * worldCoordsOut - the q_vec_type where the output will be placed
      *
     ********************************************************************* /
    void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const;
    / *********************************************************************
      *
      * Clears the cached force and torque on the object
      *
     ********************************************************************* /
    inline void clearForces() {q_vec_set(forceAccum,0,0,0); q_vec_set(torqueAccum,0,0,0);}
    / *********************************************************************
      *
      * Adds the given force at the point to the cached force on the object
      * also adds the associated torque produced by that force
      *
      * Note: the force is given relative to the world coordinate space
      *         while the point is given relative to the model coordinate space
      *
      * point - the point relative to the model on which the force is being applied
      * force - the force relative to the world coordinate axes
      *
     ********************************************************************* /
    virtual void addForce(q_vec_type point, const q_vec_type force);
    / *********************************************************************
      *
      * Sets the sum of forces and torques on the object
      *
      * force - the new force
      * torque - the new torque
      *
     ********************************************************************* /
    void setForceAndTorque(const q_vec_type force,const q_vec_type torque);
    / *********************************************************************
      *
      * Gets the sum of forces on the object (relative to world coordinates)
      *
      * out - the vector where the output will be stored
      *
     ********************************************************************* /
    inline void getForce(q_vec_type out) const { q_vec_copy(out,forceAccum);}
    / *********************************************************************
      *
      * Gets the sum of torques on the object (relative to world coordinates)
      *
      * out - the vector where the output will be stored
      *
     ********************************************************************* /
    inline void getTorque(q_vec_type out) const { q_vec_copy(out,torqueAccum);}

    / *********************************************************************
      *
      * Returns true if this object should be affected by physics
      *
     ********************************************************************* /
    inline bool doPhysics() const { return physicsEnabled; }
    / *********************************************************************
      *
      * Sets whether to do physics on this object
      *
      * doPhysics - true if this object should have physics
      *
     ********************************************************************* /
    inline void setDoPhysics(bool doPhysics) { physicsEnabled = doPhysics; }
    / *********************************************************************
      *
      * Returns true if physics is allowed on this object and its
      * transformation is allowed to be updated... i.e. if it is a "normal"
      * object
      *
     ********************************************************************* /
    inline bool isNormalObject() const { return physicsEnabled && allowTransformUpdate; }
    / *********************************************************************
      *
      * Sets this object to be displayed as wireframe
      *
     ********************************************************************* /
    inline void setWireFrame() { actor->SetMapper(modelId->getWireFrameMapper());}
    / *********************************************************************
      *
      * Sets this object to be displayed as solid
      *
     ********************************************************************* /
    inline void setSolid() { actor->SetMapper(modelId->getSolidMapper());}
    / *********************************************************************
      *
      * Converts between the objects' local transformations and the transformations
      * that the collision detection library needs and runs the collisions.  cr is
      * assumed to be a new collision result object that will be filled with data
      * from the test that is run
      *
     ********************************************************************* /
    static void collide(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr,
                        int pqpFlags = PQP_ALL_CONTACTS);
protected:
    vtkSmartPointer<vtkTransform> localTransform;
    bool allowTransformUpdate, physicsEnabled;
private:
    SketchModel *modelId;
    vtkSmartPointer<vtkActor> actor;
    q_vec_type position;
    q_type orientation;
    q_vec_type lastPosition;
    q_type lastOrientation;
    q_vec_type forceAccum;
    q_vec_type torqueAccum;
    int groupNum;
};

inline void SketchObject::setLastLocation() {
    q_vec_copy(lastPosition,position);
    q_copy(lastOrientation,orientation);
}

inline void SketchObject::restoreToLastLocation() {
    q_vec_copy(position,lastPosition);
    q_copy(orientation,lastOrientation);
}

inline void SketchObject::setForceAndTorque(const q_vec_type force, const q_vec_type torque) {
    q_vec_copy(forceAccum,force);
    q_vec_copy(torqueAccum,torque);
}
*/

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
