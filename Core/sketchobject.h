#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <quat.h>

#include <vtkSmartPointer.h>
class vtkPolyDataAlgorithm;
class vtkTransformPolyDataFilter;
class vtkColorTransferFunction;
class vtkTransform;
class vtkLinearTransform;
class vtkActor;

#include <QList>
#include <QScopedPointer>
#include <QMap>
#include <QSet>
class QString;

class SketchModel;
class Keyframe;
class PhysicsStrategy;
class ObjectChangeObserver;

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
    /*
     * This inner class contains an enum that holds the color map types that are available
     *
     */
    class ColorMapType {
    public:
        enum Type {
            SOLID_COLOR_RED,
            SOLID_COLOR_GREEN,
            SOLID_COLOR_BLUE,
            SOLID_COLOR_YELLOW,
            SOLID_COLOR_PURPLE,
            SOLID_COLOR_CYAN,
            DIM_SOLID_COLOR_RED,
            DIM_SOLID_COLOR_GREEN,
            DIM_SOLID_COLOR_BLUE,
            DIM_SOLID_COLOR_YELLOW,
            DIM_SOLID_COLOR_PURPLE,
            DIM_SOLID_COLOR_CYAN,
            BLUE_TO_RED
        };
    };

    // This function takes a color map type and constructs the color map as
    // a vtkColorTransferFunction.  The returned vtkColorTransferFunction should
    // recieved with vtkSmartPointer<...>::Take()
    // low and high are the lowest and highest values in the interval that the
    // color map should map over.
    static vtkColorTransferFunction *getColorMap(ColorMapType::Type cmapType,
                                                 double low, double high);

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
    // gets the color map for this object (only a valid call if the numInstances() == 1)
    virtual ColorMapType::Type getColorMapType() const;
    // sets the color map for this object (or all child objects if numInstances() > 1)
    virtual void setColorMapType(ColorMapType::Type cmap) = 0;
    // gets the array that is currently being used for coloring (default: modelNum)
    // only useful when numInstances == 1
    virtual const QString &getArrayToColorBy() const;
    // sets the array that is being used for coloring (on the object or all subobjects)
    virtual void setArrayToColorBy(QString &arrayName) = 0;
    // gets the transformed polygonal data of the object (do not modify the return value, but
    // you may use it as input to other filters)
    // this may return NULL if the object has no geometry itself (children may have geometry in
    // this case, but if the object has a model (numInstances==1), it should return a valid
    // vtkPolyDataAlgorithm
    virtual vtkTransformPolyDataFilter *getTransformedGeometry() = 0;
    // the conformation of the model used by this object.  If numInstances returns 1, this
    // must return a valid conformation of the model returned by getModel.  Otherwise, let
    // this default implementation return -1
    virtual int getModelConformation() const;
    // actor - NULL by default since not all objects will have one actor, but must return a valid
    // actor if numInstances is 1
    virtual vtkActor *getActor();
    // position and orientation
    void getPosition(q_vec_type dest) const;
    void getOrientation(q_type dest) const;
    void getOrientation(double matrix[3][3]) const;
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
    vtkLinearTransform *getInverseLocalTransform();
    virtual void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const;
    virtual void getWorldSpacePointInModelCoordinates(const q_vec_type worldPoint, q_vec_type modelCoordsOut) const;
    virtual void getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const;
    virtual void getModelVectorInWorldSpace(const q_vec_type modelVec, q_vec_type worldVecOut) const;
    // add, get, set and clear forces and torques on the object
    virtual void addForce(const q_vec_type point, const q_vec_type force);
    virtual void getForce(q_vec_type out) const;
    virtual void getTorque(q_vec_type out) const;
    virtual void setForceAndTorque(const q_vec_type force, const q_vec_type torque);
    virtual void clearForces();
    // collision with other.  The physics strategy will get the data about the collision
    // and decide how to respond. The bool return value is true iff there was a collision
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags) =0;
    // bounding box info for grab (have to stop using PQP_Distance)
    // the bounding box is relative to the object, and should be the axis-aligned bounding
    // box of the untransformed object (so sort-of oriented bounding box)
    // TODO - fix getBoundingBox of ObjectGroup to work this way
    virtual void getBoundingBox(double bb[6]) = 0;
    // this returns the box(es) that contain the lowest-level objects in whatever heirarchy
    // group should do an AppendPolyData to combine these
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes() = 0;
    // to deal with keyframe observers
    void addObserver(ObjectChangeObserver *obs);
    void removeObserver(ObjectChangeObserver *obs);
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
    virtual void addKeyframeForCurrentLocation(double t);
    void removeKeyframeForTime(double t);
    void setPositionByAnimationTime(double t);
    // visibility methods
    virtual void setIsVisible(bool isVisible);
    bool isVisible() const;
    // set/get active status (only has meaning on cameras)
    void setActive(bool isActive);
    bool isActive() const;
    // copy the object and its sub-objects
    virtual SketchObject *deepCopy() = 0;
protected: // methods
    // to deal with local transformation - recomputes from position and orientation unless
    // isLocalTransformPrecomputed is true
    void recalculateLocalTransform();
    // to be overridden as a template method if something needs to occur when the transform is updated
    virtual void localTransformUpdated();
protected: // fields
    vtkSmartPointer<vtkTransform> localTransform;
    vtkSmartPointer<vtkLinearTransform> invLocalTransform;
private: // methods
    void notifyForceObservers();
private: // fields
    SketchObject *parent;
    q_vec_type forceAccum,torqueAccum;
    q_vec_type position, lastPosition;
    q_type orientation, lastOrientation;
    // visibility for animations:
    bool visible, active;
    // this list is the collision groups. If it is empty, then the object has no collision group and
    // getPrimaryCollisionGroup will return OBJECT_HAS_NO_GROUP.  Else, the primary collision group
    // is ther first element in the list.  setPrimaryCollisionGroup will move the given group to the
    // start of the list (or add it if it is not there already).  addToCollisionGroup appends onto the
    // list
    QList<int> collisionGroups;
    bool localTransformPrecomputed, localTransformDefiningPosition;
    QSet<ObjectChangeObserver *> observers;
    // this smart pointer contains the keyframes of the object.  If the pointer it contains is null, then
    // there are no keyframes.  Otherwise, the map it points to is a mapping from time to frame where frame
    // contains all the information about what happens at that time (position, orientation, visibility, etc.)
    QScopedPointer< QMap< double, Keyframe > > keyframes;
};



// helper function-- converts quaternion to a PQP rotation matrix
inline void quatToPQPMatrix(const q_type quat, double mat[3][3]) {
    q_matrix_type colMat;
    q_to_col_matrix(colMat,quat);
    for (int i = 0; i < 3; i++) {
        mat[i][0] = colMat[i][0];
        mat[i][1] = colMat[i][1];
        mat[i][2] = colMat[i][2];
    }
}
#endif // SKETCHOBJECT_H
