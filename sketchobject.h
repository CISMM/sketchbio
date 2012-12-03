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
    SketchObject(vtkActor *actor, SketchModelId model,vtkTransform *worldEyeTransform);

    /*********************************************************************
      *
      * Returns the model id used to create this object
      *
     *********************************************************************/
    inline SketchModelId getModelId() const { return modelId; }
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
    inline void getPosition(q_vec_type dest) const { q_vec_copy(dest,position); }
    /*********************************************************************
      *
      * Copies the orientation of the object into the given quaternion
      *
     *********************************************************************/
    inline void getOrientation(q_type dest) const { q_copy(dest,orientation); }
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
    void addForce(q_vec_type point, q_vec_type force);
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
    inline void setWireFrame() { actor->SetMapper((*modelId)->getWireFrameMapper());}
    /*********************************************************************
      *
      * Sets this object to be displayed as solid
      *
     *********************************************************************/
    inline void setSolid() { actor->SetMapper((*modelId)->getSolidMapper());}
private:
    SketchModelId modelId;
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkTransform> localTransform;
    q_vec_type position;
    q_type orientation;
    bool allowTransformUpdate, physicsEnabled;
    q_vec_type forceAccum;
    q_vec_type torqueAccum;
};

/*
 * All objects are stored in a list, and this is how they are referred to when passed around
 * to other parts of the program
 */
typedef std::list<SketchObject *>::iterator ObjectId;

#endif // SKETCHOBJECT_H
