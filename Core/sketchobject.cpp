#include "sketchobject.h"

#include <vtkTransform.h>
#include <vtkNew.h>
#include <vtkColorTransferFunction.h>

#include "keyframe.h"
#include "objectchangeobserver.h"


void SketchObject::setParentRelativePositionForAbsolutePosition(
        SketchObject *obj, SketchObject *parent,
        const q_vec_type absPos, const q_type abs_orient)
{
    double angle, x, y, z;
    q_to_axis_angle(&x, &y, &z, &angle, abs_orient);
    angle = angle * 180.0 / Q_PI;
    vtkNew< vtkTransform > trans;
    trans->Identity();
    trans->PostMultiply();
    trans->RotateWXYZ(angle,x,y,z);
    trans->Translate(absPos);
    trans->Concatenate(parent->getInverseLocalTransform());
    q_vec_type relPos;
    q_type relOrient;
    double wxyz[4];
    trans->GetPosition(relPos);
    trans->GetOrientationWXYZ(wxyz);
    q_from_axis_angle(relOrient,wxyz[1],wxyz[2],wxyz[3],Q_PI/180.0 * wxyz[0]);
    obj->setPosAndOrient(relPos,relOrient);
}

//#########################################################################
SketchObject::SketchObject() :
    localTransform(vtkSmartPointer<vtkTransform>::New()),
    invLocalTransform(localTransform->GetLinearInverse()),
    parent(NULL),
    visible(true),
    active(false),
    propagateForce(false),
    collisionGroups(),
    localTransformPrecomputed(false),
    localTransformDefiningPosition(false),
    observers(),
    keyframes(NULL)
{
    q_vec_set(forceAccum,0,0,0);
    q_vec_set(torqueAccum,0,0,0);
    q_vec_set(position,0,0,0);
    q_vec_set(lastPosition,0,0,0);
    q_from_axis_angle(orientation,1,0,0,0);
    q_from_axis_angle(lastOrientation,1,0,0,0);
    recalculateLocalTransform();
}

//#########################################################################
SketchObject::~SketchObject()
{
}

//#########################################################################
SketchObject *SketchObject::getParent()
{
    return parent;
}

//#########################################################################
const SketchObject *SketchObject::getParent() const
{
    return parent;
}

//#########################################################################
void SketchObject::setParent(SketchObject *p)
{
    parent = p;
    recalculateLocalTransform();
}

//#########################################################################
QList<SketchObject *> *SketchObject::getSubObjects()
{
    return NULL;
}

//#########################################################################
const QList<SketchObject *> *SketchObject::getSubObjects() const
{
    return NULL;
}

//#########################################################################
SketchModel *SketchObject::getModel()
{
    return NULL;
}
//#########################################################################
const SketchModel *SketchObject::getModel() const
{
    return NULL;
}
//#########################################################################
ColorMapType::Type SketchObject::getColorMapType() const
{
    return ColorMapType::SOLID_COLOR_RED;
}
//#########################################################################
QString SketchObject::getArrayToColorBy() const
{
    return "";
}
//#########################################################################
int SketchObject::getModelConformation() const {
    return -1;
}
//#########################################################################
vtkActor *SketchObject::getActor() {
    return NULL;
}
//#########################################################################
void SketchObject::getPosition(q_vec_type dest) const
{
    if (parent != NULL || localTransformDefiningPosition)
    {
        localTransform->GetPosition(dest);
    }
    else
    {
        q_vec_copy(dest,position);
    }
}
//#########################################################################
void SketchObject::getOrientation(q_type dest) const
{
    if (parent != NULL || localTransformDefiningPosition)
    {
        double wxyz[4];
        localTransform->GetOrientationWXYZ(wxyz);
        q_from_axis_angle(dest,wxyz[1],wxyz[2],wxyz[3],wxyz[0]*Q_PI/180.0);
    }
    else
    {
        q_copy(dest,orientation);
    }
}
//#########################################################################
void SketchObject::getOrientation(double matrix[3][3]) const
{
    q_type tmp;
    getOrientation(tmp); // simpler than duplicating case code
    quatToPQPMatrix(tmp,matrix);
}

//#########################################################################
void SketchObject::setPosition(const q_vec_type newPosition)
{
    q_vec_copy(position,newPosition);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();)
    {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::setOrientation(const q_type newOrientation)
{
    q_copy(orientation,newOrientation);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();)
    {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::setPosAndOrient(const q_vec_type newPosition, const q_type newOrientation)
{
    q_vec_copy(position,newPosition);
    q_copy(orientation,newOrientation);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();)
    {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::getLastPosition(q_vec_type dest) const
{
    q_vec_copy(dest,lastPosition);
}
//#########################################################################
void SketchObject::getLastOrientation(q_type dest) const
{
    q_copy(dest,lastOrientation);
}
//#########################################################################
void SketchObject::setLastLocation()
{
    SketchObject::getPosition(lastPosition);
    SketchObject::getOrientation(lastOrientation);
}

//#########################################################################
void SketchObject::restoreToLastLocation()
{
    if (parent == NULL)
    {
        setPosAndOrient(lastPosition,lastOrientation);
    }
    else
    {
        setParentRelativePositionForAbsolutePosition(
                    this,parent,lastPosition,lastOrientation);
    }
}

//#########################################################################
int SketchObject::getPrimaryCollisionGroupNum()
{
    if (collisionGroups.empty())
    {
        return OBJECT_HAS_NO_GROUP;
    }
    else
    {
        return collisionGroups[0];
    }
}

//#########################################################################
void SketchObject::setPrimaryCollisionGroupNum(int num)
{
    if (collisionGroups.contains(num))
    {
        collisionGroups.removeOne(num);
    }
    collisionGroups.prepend(num);
}
//#########################################################################
void SketchObject::addToCollisionGroup(int num)
{
    if (collisionGroups.contains(num) || num == OBJECT_HAS_NO_GROUP)
        return;
    collisionGroups.append(num);
}

//#########################################################################
bool SketchObject::isInCollisionGroup(int num) const
{
    return collisionGroups.contains(num);
}
//#########################################################################
void SketchObject::removeFromCollisionGroup(int num)
{
    collisionGroups.removeAll(num);
}
//#########################################################################
vtkTransform *SketchObject::getLocalTransform()
{
    return localTransform;
}
//#########################################################################
vtkLinearTransform *SketchObject::getInverseLocalTransform()
{
    return invLocalTransform;
}

//#########################################################################
void SketchObject::getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint,
                                                        q_vec_type worldCoordsOut) const
{
    localTransform->TransformPoint(modelPoint,worldCoordsOut);
}
//#########################################################################
void SketchObject::getWorldSpacePointInModelCoordinates(const q_vec_type worldPoint,
                                                        q_vec_type modelCoordsOut) const
{
    invLocalTransform->TransformPoint(worldPoint, modelCoordsOut);
}
//#########################################################################
void SketchObject::getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const
{
    invLocalTransform->TransformVector(worldVec,modelVecOut);
}
//#########################################################################
void SketchObject::getModelVectorInWorldSpace(const q_vec_type modelVec, q_vec_type worldVecOut) const
{
    localTransform->TransformVector(modelVec,worldVecOut);
}
//#########################################################################
void SketchObject::addForce(const q_vec_type point, const q_vec_type force)
{
    if (!propagateForce || parent == NULL)
    {
        q_vec_add(forceAccum,forceAccum,force);
        q_vec_type tmp, torque;
        getWorldVectorInModelSpace(force,tmp);
        q_vec_cross_product(torque,point,tmp);
        q_vec_add(torqueAccum,torqueAccum,torque);
    }
    else
    {
        q_vec_type tmp;
        // get point in parent coordinate system
        getModelSpacePointInWorldCoordinates(point,tmp);
        parent->addForce(tmp,force);
    }
    notifyForceObservers();
}

//#########################################################################
void SketchObject::getForce(q_vec_type out) const
{
    q_vec_copy(out,forceAccum);
}
//#########################################################################
void SketchObject::getTorque(q_vec_type out) const
{
    q_vec_copy(out,torqueAccum);
}
//#########################################################################
void SketchObject::setForceAndTorque(const q_vec_type force, const q_vec_type torque)
{
    q_vec_copy(forceAccum,force);
    q_vec_copy(torqueAccum,torque);
    notifyForceObservers();
}

//#########################################################################
void SketchObject::clearForces()
{
    q_vec_set(forceAccum,0,0,0);
    q_vec_set(torqueAccum,0,0,0);
}

//#########################################################################
void SketchObject::setLocalTransformPrecomputed(bool isComputed)
{
    localTransformPrecomputed = isComputed;
    if (!isComputed)
    {
        recalculateLocalTransform();
    }
}

//#########################################################################
bool SketchObject::isLocalTransformPrecomputed() const
{
    return localTransformPrecomputed;
}

//#########################################################################
void SketchObject::setLocalTransformDefiningPosition(bool isDefining)
{
    localTransformDefiningPosition = isDefining;
}

//#########################################################################
bool SketchObject::isLocalTransformDefiningPosition() const
{
    return localTransformDefiningPosition;
}

//#########################################################################
bool SketchObject::hasKeyframes() const
{
    return !(keyframes.isNull() || keyframes->empty());
}

//#########################################################################
int SketchObject::getNumKeyframes() const
{
    if (keyframes.isNull())
    {
        return 0;
    }
    else
    {
        return keyframes->size();
    }
}

//#########################################################################
const QMap< double, Keyframe > *SketchObject::getKeyframes() const
{
    return keyframes.data();
}

//#########################################################################
void SketchObject::addKeyframeForCurrentLocation(double t)
{
    if (t < 0)
    { // no negative times allowed
        return;
    }
    Keyframe frame(position, orientation,getColorMapType(),getArrayToColorBy(),visible,active);
    if (keyframes.isNull())
    {
        keyframes.reset(new QMap< double, Keyframe >());
    }
    keyframes->insert(t,frame);
    for (QSetIterator<ObjectChangeObserver *> it(observers); it.hasNext();)
    {
        it.next()->objectKeyframed(this,t);
    }
}

//#########################################################################
void SketchObject::removeKeyframeForTime(double t)
{
    if (keyframes.isNull())
    {
        return;
    }
    if (keyframes->contains(t))
    {
        keyframes->remove(t);
    }
}

//#########################################################################
void SketchObject::setPositionByAnimationTime(double t)
{
    // we don't support negative times and if the object has no keyframes, then
    // no need to do anything
    if (t < 0 || !hasKeyframes())
    {
        return;
    }
    QMapIterator< double, Keyframe > it(*keyframes.data());
    // last is the last keyframe we passed
    double last = it.peekNext().key();
    // go until the next one is greater than the time (the last one
    // will be less than the time unless the time is less than the first
    // time in the keyframes list
    while (it.hasNext() && it.peekNext().key() < t)
    {
        last = it.next().key();
    }
    // if we are after the end of the last keyframe defined
    if (!it.hasNext())
    {
        Keyframe f = keyframes->value(last);
        f.getPosition(position);
        f.getOrientation(orientation);
        // set color map stuff here
        setColorMapType(f.getColorMapType());
        setArrayToColorBy(f.getArrayToColorBy());
        setIsVisible(f.isVisibleAfter());
        setActive(f.isActive());
    // if we happenned to land on a keyframe
        // or if we are before the first keyframe
    }
    else if (it.peekNext().key() == t ||
               it.peekNext().key() == last)
    {
        Keyframe f = it.next().value();
        f.getPosition(position);
        f.getOrientation(orientation);
        // set color map stuff here
        setColorMapType(f.getColorMapType());
        setArrayToColorBy(f.getArrayToColorBy());
        setIsVisible(f.isVisibleAfter());
        setActive(f.isActive());
    }
    else
    {
        // if we have a next keyframe that is greater than the time
        double next = it.peekNext().key();
        // note, if the first keyframe is after the time given, these will
        // be the same and the state will be set to the state at that keyframe
        Keyframe f1 = keyframes->value(last), f2 = it.next().value();
        double diff1 = next - last;
        double diff2 = t - last;
        double ratio = diff2 / diff1;
        // - being lazy and just linearly interpolating... really should use a spline for pos
        //   but too much effort until we are asked for it
        q_vec_type pos1, pos2;
        q_type or1, or2;
        f1.getPosition(pos1);
        f1.getOrientation(or1);
        f2.getPosition(pos2);
        f2.getOrientation(or2);
        q_vec_subtract(pos2,pos2,pos1);
        q_vec_scale(pos2,ratio,pos2);
        q_vec_add(position,pos1,pos2); // set position to linearly interpolated location between points
        q_slerp(orientation,or1,or2,ratio); // set orientation to SLERP quaternion
        // set color map stuff here
        setColorMapType(f1.getColorMapType());
        setArrayToColorBy(f1.getArrayToColorBy());
        // set visibility stuff here
        setIsVisible(f1.isVisibleAfter());
        setActive(f1.isActive());
    }
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();)
    {
        it.next()->objectMoved(this);
    }
    recalculateLocalTransform();
    // set out own position first to avoid messing up sub-object positions
    QList<SketchObject *> *subObjects = getSubObjects();
    if (subObjects != NULL)
    {
        for (QListIterator<SketchObject *> itr(*subObjects); itr.hasNext(); )
        {
            itr.next()->setPositionByAnimationTime(t);
        }
    }
}

//#########################################################################
void SketchObject::addObserver(ObjectChangeObserver *obs)
{
        observers.insert(obs);
}

//#########################################################################
void SketchObject::removeObserver(ObjectChangeObserver *obs)
{
    observers.remove(obs);
}

//#########################################################################
void SketchObject::recalculateLocalTransform()
{
    if (isLocalTransformPrecomputed())
    {
        localTransformUpdated();
        return;
    }
    double angle,x,y,z;
    q_to_axis_angle(&x,&y,&z,&angle,orientation);
    angle = angle * 180 / Q_PI; // convert to degrees
    localTransform->PostMultiply();
    localTransform->Identity();
    localTransform->RotateWXYZ(angle,x,y,z);
    localTransform->Translate(position);
    if (parent != NULL)
    {
        localTransform->Concatenate(parent->getLocalTransform());
    }
    localTransform->Update();
    localTransformUpdated();
}

//#########################################################################
void SketchObject::localTransformUpdated()
{
}

//#########################################################################
void SketchObject::localTransformUpdated(SketchObject *obj)
{
    obj->localTransformUpdated();
}

//#########################################################################
void SketchObject::notifyForceObservers()
{
    for (QSetIterator<ObjectChangeObserver *> it(observers); it.hasNext(); )
    {
        it.next()->objectPushed(this);
    }
}

//#########################################################################
void SketchObject::setIsVisible(bool isVisible)
{
    visible = isVisible;
}

//#########################################################################
void SketchObject::setIsVisibleRecursive(SketchObject *obj, bool isVisible)
{
    obj->setIsVisible(isVisible);
    QList< SketchObject * > *children = obj->getSubObjects();
    if (children != NULL)
    {
        for (int i = 0; i < children->size(); i++)
        {
            setIsVisibleRecursive(children->at(i),isVisible);
        }
    }
}

//#########################################################################
bool SketchObject::isVisible() const
{
    return visible;
}

//#########################################################################
void SketchObject::setActive(bool isActive)
{
    active = isActive;
}

//#########################################################################
bool SketchObject::isActive() const
{
    return active;
}

//#########################################################################
void SketchObject::setPropagateForceToParent(bool propagate)
{
    propagateForce = propagate;
}

//#########################################################################
bool SketchObject::isPropagatingForceToParent()
{
    return propagateForce;
}

//#########################################################################
void SketchObject::notifyObjectAdded(SketchObject *child)
{
    for (QSetIterator<ObjectChangeObserver *> it(observers); it.hasNext(); )
    {
        it.next()->subobjectAdded(this,child);
    }
    if (parent != NULL)
        parent->notifyObjectAdded(child);
}
//#########################################################################
void SketchObject::notifyObjectRemoved(SketchObject *child)
{
    for (QSetIterator<ObjectChangeObserver *> it(observers); it.hasNext(); )
    {
        it.next()->subobjectRemoved(this,child);
    }
    if (parent != NULL)
        parent->notifyObjectRemoved(child);
}
