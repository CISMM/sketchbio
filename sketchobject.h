#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkActor.h>
#include <quat.h>

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
    SketchObject(vtkActor *actor, int model,vtkTransform *worldEyeTransform);

    /*********************************************************************
      *
      * Returns the model id used to create this object
      *
     *********************************************************************/
    inline int getModelId() const { return modelId; }
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
    inline void allowLocalTransformUpdates(bool allow) { updateLocalTransform = allow; }
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
private:
    int modelId;
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkTransform> localTransform;
    q_vec_type position;
    q_type orientation;
    bool updateLocalTransform;
};

#endif // SKETCHOBJECT_H
