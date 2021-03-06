#ifndef SKETCHOBJECT_H
#define SKETCHOBJECT_H

#include <quat.h>

#include <vtkSmartPointer.h>
class vtkAlgorithm;
class vtkPolyDataAlgorithm;
class vtkTransformPolyDataFilter;
class vtkColorTransferFunction;
class vtkTransform;
class vtkLinearTransform;
class vtkActor;
class vtkCardinalSpline;

#include <QList>
#include <QScopedPointer>
#include <QMap>
#include <QSet>
class QString;

class SketchModel;
class Keyframe;
class PhysicsStrategy;
class ObjectChangeObserver;
#include "colormaptype.h"
#include "sketchmodel.h"

// used by getPrimaryCollisionGroupNum to indicate that the object has not been
// assigned a group
#define OBJECT_HAS_NO_GROUP (-1)

/*
 * This class is an abstract representation of some instance or group of
 *instances
 * in the world that the user would like to treat as a single object in terms of
 * position/orientation and movement relative to other things.
 *
 * This class is the abstract superclass for the Composite Pattern, although the
 * data that will be the same for both instances and groups is stored and
 *accessed
 * from here, such as position/orientation and force/torque.
 */

class SketchObject
{
   public:
    // Takes an absolute position and an object and its parent
    // and computes and sets the correct parent-relative position
    // of the object so that it has the absolute position specified
    static void setParentRelativePositionForAbsolutePosition(
        SketchObject *obj, SketchObject *parent, const q_vec_type absPos,
        const q_type abs_orient);
    // Given a vtkTransform, compute the position and orientation
    // described by that vtkTransform
    static void getPositionAndOrientationFromLinearTransform(
        vtkLinearTransform *trans, q_vec_type pos, q_type orient);
    static void getPositionAndOrientationFromTransform(vtkTransform *trans,
                                                       q_vec_type pos,
                                                       q_type orient);
    // Given a position and orientation and a vtkTransform object,
    // set up the transform object to have the given position and orientation
    static void setTransformToPositionAndOrientation(vtkTransform *trans,
                                                     const q_vec_type pos,
                                                     const q_type orient);

    // Computes the relative transform from o1 to o2 and returns it as a vtkTransform
    // That is, the transform will contain the position and orientation of o2 in o1's
    // local coordinate system
    static vtkTransform *computeRelativeTransform(SketchObject *o1,SketchObject *o2);

   public:
    // constructor
    SketchObject();
    virtual ~SketchObject();
    // the number of instances controlled by this object (is single or group?)
    // this should return 1 for a single object that fulfills the requirements
    // defined on the virtual functions below.  Otherwise it should return a
    // value other than 1, which is some indication of how many single objects
    // this one contains
    virtual int numInstances() const = 0;
    // the parent "object"/group
    virtual SketchObject *getParent();
    virtual const SketchObject *getParent() const;
    virtual void setParent(SketchObject *p);
    // get the list of child objects
    virtual QList< SketchObject * > *getSubObjects();
    virtual const QList< SketchObject * > *getSubObjects() const;
    // model information - NULL by default since not all objects will have one
    // model
    // however, these must be implemented to return a valid SketchModel if
    // numInstances returns 1
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
	// sets the object to display in full resolution or a simplified resolution
	virtual void showFullResolution();
	virtual void hideFullResolution();
    // gets the color map for this object (only a valid call if the
    // numInstances() == 1)
    ColorMapType::Type getColorMapType() const;
    // sets the color map for this object
    void setColorMapType(ColorMapType::Type cmap);
    // gets the array that is currently being used for coloring (default:
    // modelNum)
    // only useful when numInstances == 1
    const QString &getArrayToColorBy() const;
    // sets the array that is being used for coloring
    void setArrayToColorBy(const QString &arrayName);
    // gets the color map - the previous 4 may eventually be replaced by this
    // and
    // a corresponding set method
    const ColorMapType::ColorMap &getColorMap() const;
	// gets the luminance for solid colored objects
	virtual double getLuminance() const;
	virtual double getDisplayLuminance() const;
	virtual void setLuminance(double lum);
	virtual void setMinLuminance(double minLum);
	virtual void setMaxLuminance(double maxLum);
    // the conformation of the model used by this object.  If numInstances
    // returns 1, this
    // must return a valid conformation of the model returned by getModel.
    // Otherwise, let
    // this default implementation return -1
    virtual int getModelConformation() const;
    // actor - NULL by default since not all objects will have one actor, but
    // must return a valid
    // actor if numInstances is 1
    virtual vtkActor *getActor();
    // position and orientation
    virtual void getPosition(q_vec_type dest) const;
    virtual void getOrientation(q_type dest) const;
    void getOrientation(double matrix[3][3]) const;
    void setPosition(const q_vec_type newPosition);
    void setOrientation(const q_type newOrientation);
    void setPosAndOrient(const q_vec_type newPosition,
                         const q_type newOrientation);
    // to do with setting "undo" position -- not real undo, but a temporary
    // state
    // for physics calculations
    void getLastPosition(q_vec_type dest) const;
    void getLastOrientation(q_type dest) const;
    void setLastLocation();
    void restoreToLastLocation();
    // to do with collision groups (see comments on the collisionGroups field
    // for details of
    // implmentation)
    int getPrimaryCollisionGroupNum();
    void setPrimaryCollisionGroupNum(int num);
    void addToCollisionGroup(int num);
    bool isInCollisionGroup(int num) const;
    void removeFromCollisionGroup(int num);
    // local transformation & transforming points/vectors
    vtkTransform *getLocalTransform();
    vtkLinearTransform *getInverseLocalTransform();
    virtual void getModelSpacePointInWorldCoordinates(
        const q_vec_type modelPoint, q_vec_type worldCoordsOut) const;
    virtual void getWorldSpacePointInModelCoordinates(
        const q_vec_type worldPoint, q_vec_type modelCoordsOut) const;
    virtual void getWorldVectorInModelSpace(const q_vec_type worldVec,
                                            q_vec_type modelVecOut) const;
    virtual void getModelVectorInWorldSpace(const q_vec_type modelVec,
                                            q_vec_type worldVecOut) const;
    // add, get, set and clear forces and torques on the object
    virtual void addForce(const q_vec_type point, const q_vec_type force);
    virtual void getForce(q_vec_type out) const;
    virtual void getTorque(q_vec_type out) const;
    virtual void setForceAndTorque(const q_vec_type force,
                                   const q_vec_type torque);
    virtual void clearForces();
    // collision with other.  The physics strategy will get the data about the
    // collision
    // and decide how to respond. The bool return value is true iff there was a
    // collision
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics,
                         int pqp_flags) = 0;
    // bounding box info for grab (have to stop using PQP_Distance)
    // the bounding box is relative to the object, and should be the
    // axis-aligned bounding
    // box of the untransformed object (so sort-of oriented bounding box)
    // TODO - fix getBoundingBox of ObjectGroup to work this way, it doesn't
    virtual void getBoundingBox(double bb[6]) = 0;
    // this returns the box(es) that contain the lowest-level objects in
    // whatever heirarchy
    // group should do an AppendPolyData to combine these
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes() = 0;
    // this returns the indicator lines that show status information about the
    // object
    // (for now they will show whether or not it is keyframed)
    virtual vtkAlgorithm *getOrientedHalfPlaneOutlines() = 0;
    // to deal with keyframe observers
    void addObserver(ObjectChangeObserver *obs);
    void removeObserver(ObjectChangeObserver *obs);
    // to stop the object from updating its transform when its
    // position/orientation are set
    // used when an external source is defining the object's local
    // transformation
    void setLocalTransformPrecomputed(bool isComputed);
    bool isLocalTransformPrecomputed() const;
    // to set whether or not this object's local transformation has more
    // information about
    // its position than the position vector
    void setLocalTransformDefiningPosition(bool isDefining);
    bool isLocalTransformDefiningPosition() const;
    // to allow localTransformUpdated to be called from subclasses on other
    // SketchObjects...
    static void localTransformUpdated(SketchObject *obj);
    // methods for accessing and modifying keyframes
    bool hasKeyframes() const;
    int getNumKeyframes() const;
    const QMap< double, Keyframe > *getKeyframes() const;
    // return the level of the object within groups to be used in a keyframe
    int getGroupingLevel();
    // adds a keyframe at the given time, recursively descends to child objects
    // and
    // notifies ObjectChangeObservers that a keyframe has been added
    void addKeyframeForCurrentLocation(double t);
    // inserts the given keyframe to the map of keyframes.  Does not recursively
    // descend
    // to subobjects or cause observers to be notified.
    void insertKeyframe(double time, const Keyframe &keyframe);
    // returns true if the object's state is different from the state at the
    // keyframe
    // at time t.  If there is no keyframe at time t, returns true.  Essentially
    // asks
    // 'if a keyframe is set at time t, will that change the state of this
    // object or
    // it's children's keyframes?'
    bool hasChangedSinceKeyframe(double t);
    // removes the keyframe at the given time, if any
    void removeKeyframeForTime(double t);
    // clears all keyframes
    void clearKeyframes();
    // sets the position and other data based on this object's keyframes to the
    // correct
    // state for the given time in the animation
    void setPositionByAnimationTime(double t);
    // Computes interpolating splines for animation
    void computeSplines();
    void getPosAndOrFromSpline(q_vec_type pos_dest, q_type or_dest, double t);
    // visibility methods
    void setIsVisible(bool isVisible);
    static void setIsVisibleRecursive(SketchObject *obj, bool isVisible);
    bool isVisible() const;
    // set/get active status (only has meaning on cameras)
    void setActive(bool isActive);
    bool isActive() const;
	// set/get grabbed status
	void setGrabbed(bool isGrabbed);
	bool isGrabbed() const;
    // set/get the 'propagate-force-to-parent' status
    void setPropagateForceToParent(bool propagate);
    bool isPropagatingForceToParent();
    // copy the object and its sub-objects (only model & position information
    // copied
    virtual SketchObject *getCopy() = 0;
    // copies the object including everything copied by makeDeepCopyOf as well
    // as the
    // data copied by getCopy().  For now this function is just used in testing
    virtual SketchObject *deepCopy() = 0;

   protected:  // methods
    // copies the position, orientation, visiblity active and propagateForce
    // status from
    // the other object
    // also copies keyframes
    void makeDeepCopyOf(SketchObject *other);
    // to deal with local transformation - recomputes from position and
    // orientation unless
    // isLocalTransformPrecomputed is true
    void recalculateLocalTransform();
    // to be overridden as a template method if something needs to occur when
    // the transform is updated
    virtual void localTransformUpdated();
    // notifies change observers that the given object has been added as a child
    // or grandchild, etc
    void notifyObjectAdded(SketchObject *child);
    // notifies change observers that the given object has been removed as a
    // child or grandchild, etc
    void notifyObjectRemoved(SketchObject *child);
    virtual void setSolidColor(double color[3]) {}
    virtual void updateColorMap() {}

   protected:  // fields
    vtkSmartPointer< vtkTransform > localTransform;
    vtkSmartPointer< vtkLinearTransform > invLocalTransform;

   private:  // methods
    void notifyForceObservers();

   private:  // fields
    // Disable copy constructor and assignment operator these are not implemented
    // and not supported
    SketchObject(const SketchObject &other);
    SketchObject &operator=(const SketchObject &other);

    SketchObject *parent;
    q_vec_type forceAccum, torqueAccum;
    q_vec_type position, lastPosition;
    q_type orientation, lastOrientation;
    // visibility for animations:
    bool visible, active, grabbed, propagateForce;
    // this list is the collision groups. If it is empty, then the object has no
    // collision group and
    // getPrimaryCollisionGroup will return OBJECT_HAS_NO_GROUP.  Else, the
    // primary collision group
    // is ther first element in the list.  setPrimaryCollisionGroup will move
    // the given group to the
    // start of the list (or add it if it is not there already).
    // addToCollisionGroup appends onto the
    // list
    QList< int > collisionGroups;
    bool localTransformPrecomputed, localTransformDefiningPosition;
    QSet< ObjectChangeObserver * > observers;
    ColorMapType::ColorMap map;
    // this smart pointer contains the keyframes of the object.  If the pointer
    // it contains is null, then
    // there are no keyframes.  Otherwise, the map it points to is a mapping
    // from time to frame where frame
    // contains all the information about what happens at that time (position,
    // orientation, visibility, etc.)
    QScopedPointer< QMap< double, Keyframe > > keyframes;
    QScopedPointer< QMap< double, vtkSmartPointer< vtkCardinalSpline > > >
        xsplines, ysplines, zsplines, yaw_splines, pitch_splines, roll_splines;
};

// helper function-- converts quaternion to a PQP rotation matrix
inline void quatToPQPMatrix(const q_type quat, double mat[3][3])
{
    q_matrix_type colMat;
    q_to_col_matrix(colMat, quat);
    for (int i = 0; i < 3; i++) {
        mat[i][0] = colMat[i][0];
        mat[i][1] = colMat[i][1];
        mat[i][2] = colMat[i][2];
    }
}

inline const ColorMapType::ColorMap &SketchObject::getColorMap() const
{
    return map;
}
inline ColorMapType::Type SketchObject::getColorMapType() const
{
    return map.first;
}
inline void SketchObject::setColorMapType(ColorMapType::Type cmap)
{
    map.first = cmap;
    updateColorMap();
}

inline const QString &SketchObject::getArrayToColorBy() const
{
    return map.second;
}

inline void SketchObject::setArrayToColorBy(const QString &arrayName)
{
    map.second = arrayName;
    updateColorMap();
}
#endif  // SKETCHOBJECT_H
