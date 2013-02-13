#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkActor.h>
#include <quat.h>
#include <list>
#include <sketchmodel.h>

/*
 * This class contains the data that is unique to each instance of an object
 * such as position, orientation, the vtkActor specific to the object, etc.
 *
 * It also contains the force and torque accumulators that are used in the
 * main physics loop to count how much force and torque are applied to the
 * object.
 */

// used by getPrimaryGroupNum to indicate that the object has not been
// assigned a group
#define OBJECT_HAS_NO_GROUP (-1)

class SketchObject
{
public:
    /*********************************************************************
      *
      * Creates a new SketchObject from the given parameters.  The initial
      * position and orientation of the object are identity.
      *
      * actor - the actor the new object stores physics data for
      * model - the model id of the model this object is based on
      * worldEyeTransform - the transformation manager's world-eye matrix
      *                     used to set up the actor's transform
      *
      ********************************************************************/
    SketchObject(vtkActor *actor, SketchModel *model,vtkTransform *worldEyeTransform);

    /*********************************************************************
      *
      * Returns the model used to create this object
      *
     *********************************************************************/
    inline SketchModel *getModel() { return modelId; }
    /*********************************************************************
      *
      * Returns a const pointer to the model used to create this object
      *
     *********************************************************************/
    inline const SketchModel *getConstModel() const { return modelId; }
    /*********************************************************************
      *
      * Returns the actor this object stores physics data for
      *
     *********************************************************************/
    inline vtkActor *getActor() { return actor; }
    /*********************************************************************
      *
      * Copies the position of the object into the given vector
      *
     *********************************************************************/
    virtual void getPosition(q_vec_type dest) const;
    /*********************************************************************
      *
      * Copies the old position of the object into the given vector
      *
     *********************************************************************/
    inline void getLastPosition(q_vec_type dest) const { q_vec_copy(dest,lastPosition); }
    /*********************************************************************
      *
      * Copies the orientation of the object into the given quaternion
      *
     *********************************************************************/
    virtual void getOrientation(q_type dest) const;
    /*********************************************************************
      *
      * Copies the old orientation of the object into the given quaternion
      *
     *********************************************************************/
    inline void getLastOrientation(q_type dest) const { q_copy(dest,lastOrientation); }
    /*********************************************************************
      *
      * Saves the current position and orientation of the object to the
      * lastPosition and lastOrientation
      *
     *********************************************************************/
    void setLastLocation();
    /*********************************************************************
      *
      * Restores the values saved by setLastLocation to be the current
      * position and orientation of the object
      *
     *********************************************************************/
    void restoreToLastLocation();
    /*********************************************************************
      *
      * Sets the orientation of the object to the given new orientation
      *
     *********************************************************************/
    inline void setOrientation(const q_type newOrientation) { q_copy(orientation, newOrientation); }
    /*********************************************************************
      *
      * Sets the position of the object to the given new position
      *
     *********************************************************************/
    inline void setPosition(const q_vec_type newPosition) { q_vec_copy(position, newPosition); }
    virtual void setPrimaryGroupNum(int num);
    virtual int getPrimaryGroupNum() const;
    virtual bool isInGroup(int num) const;
    /*********************************************************************
      *
      * Gets the transformation within world coordinates that defines this
      * object's position and orientation
      *
     *********************************************************************/
    inline vtkTransform *getLocalTransform() { return localTransform.GetPointer(); }
    /*********************************************************************
      *
      * Recalculates the object's transformation within world coordinates
      * based on its position and orientation if this updating has not been
      * disabled
      *
     *********************************************************************/
    void recalculateLocalTransform();
    /*********************************************************************
      *
      * Enables/disables updating of this object's local transformation
      *
      * Note: These should only be disabled if the local transform has
      *     been redefined to update based on something else
      *
     *********************************************************************/
    inline void allowLocalTransformUpdates(bool allow) { allowTransformUpdate = allow; }
    /*********************************************************************
      *
      * Gets the corresponding world coordinates for the given point in the
      * model's coordinate system
      *
      * modelPoint - the modelspace point given
      * worldCoordsOut - the q_vec_type where the output will be placed
      *
     *********************************************************************/
    void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const;
    /*********************************************************************
      *
      * Clears the cached force and torque on the object
      *
     *********************************************************************/
    inline void clearForces() {q_vec_set(forceAccum,0,0,0); q_vec_set(torqueAccum,0,0,0);}
    /*********************************************************************
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
     *********************************************************************/
    virtual void addForce(q_vec_type point, const q_vec_type force);
    /*********************************************************************
      *
      * Gets the sum of forces on the object (relative to world coordinates)
      *
      * out - the vector where the output will be stored
      *
     *********************************************************************/
    inline void getForce(q_vec_type out) const { q_vec_copy(out,forceAccum);}
    /*********************************************************************
      *
      * Gets the sum of torques on the object (relative to world coordinates)
      *
      * out - the vector where the output will be stored
      *
     *********************************************************************/
    inline void getTorque(q_vec_type out) const { q_vec_copy(out,torqueAccum);}

    /*********************************************************************
      *
      * Returns true if this object should be affected by physics
      *
     *********************************************************************/
    inline bool doPhysics() const { return physicsEnabled; }
    /*********************************************************************
      *
      * Sets whether to do physics on this object
      *
      * doPhysics - true if this object should have physics
      *
     *********************************************************************/
    inline void setDoPhysics(bool doPhysics) { physicsEnabled = doPhysics; }
    /*********************************************************************
      *
      * Returns true if physics is allowed on this object and its
      * transformation is allowed to be updated... i.e. if it is a "normal"
      * object
      *
     *********************************************************************/
    inline bool isNormalObject() const { return physicsEnabled && allowTransformUpdate; }
    /*********************************************************************
      *
      * Sets this object to be displayed as wireframe
      *
     *********************************************************************/
    inline void setWireFrame() { actor->SetMapper(modelId->getWireFrameMapper());}
    /*********************************************************************
      *
      * Sets this object to be displayed as solid
      *
     *********************************************************************/
    inline void setSolid() { actor->SetMapper(modelId->getSolidMapper());}
    /*********************************************************************
      *
      * Converts between the objects' local transformations and the transformations
      * that the collision detection library needs and runs the collisions.  cr is
      * assumed to be a new collision result object that will be filled with data
      * from the test that is run
      *
     *********************************************************************/
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

// helper function-- converts quaternion to a PQP matrix
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
