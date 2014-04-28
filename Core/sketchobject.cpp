#include "sketchobject.h"

#include <vtkTransform.h>
#include <vtkNew.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkCardinalSpline.h>
#include <vtkMath.h>

#include "keyframe.h"
#include "sketchtests.h"
#include "objectchangeobserver.h"

//#########################################################################
void SketchObject::setParentRelativePositionForAbsolutePosition(
    SketchObject *obj, SketchObject *parent, const q_vec_type absPos,
    const q_type abs_orient)
{
    vtkNew< vtkTransform > trans;
    setTransformToPositionAndOrientation(trans.GetPointer(), absPos,
                                         abs_orient);
    trans->Concatenate(parent->getInverseLocalTransform());
    q_vec_type relPos;
    q_type relOrient;
    getPositionAndOrientationFromTransform(trans.GetPointer(), relPos,
                                           relOrient);
    obj->setPosAndOrient(relPos, relOrient);
}

//#########################################################################
void SketchObject::getPositionAndOrientationFromLinearTransform(
    vtkLinearTransform *trans, q_vec_type pos, q_type orient)
{
    vtkNew< vtkTransform > transform;
    transform->Identity();
    transform->Concatenate(trans);
    getPositionAndOrientationFromTransform(transform.GetPointer(), pos, orient);
}

//#########################################################################
void SketchObject::getPositionAndOrientationFromTransform(vtkTransform *trans,
                                                          q_vec_type pos,
                                                          q_type orient)
{
    double wxyz[4];
    trans->GetPosition(pos);
    trans->GetOrientationWXYZ(wxyz);
    q_from_axis_angle(orient, wxyz[1], wxyz[2], wxyz[3],
                      Q_PI / 180.0 * wxyz[0]);
}

//#########################################################################
void SketchObject::setTransformToPositionAndOrientation(vtkTransform *trans,
                                                        const q_vec_type pos,
                                                        const q_type orient)
{
    double angle, x, y, z;
    q_to_axis_angle(&x, &y, &z, &angle, orient);
    angle = angle * 180.0 / Q_PI;
    trans->Identity();
    trans->PostMultiply();
    trans->RotateWXYZ(angle, x, y, z);
    trans->Translate(pos);
}

//#########################################################################
vtkTransform *SketchObject::computeRelativeTransform(SketchObject *o1,
                                                     SketchObject *o2) {
    vtkTransform *trans = vtkTransform::New();
    trans->Identity();
    trans->PreMultiply();
    trans->Concatenate(o1->getInverseLocalTransform());
    trans->Concatenate(o2->getLocalTransform());
    trans->Update();
    return trans;
}


//#########################################################################
//#########################################################################
//#########################################################################
SketchObject::SketchObject()
    : localTransform(vtkSmartPointer< vtkTransform >::New()),
      invLocalTransform(localTransform->GetLinearInverse()),
      parent(NULL),
      visible(true),
      active(false),
      propagateForce(false),
      collisionGroups(),
      localTransformPrecomputed(false),
      localTransformDefiningPosition(false),
      observers(),
      keyframes(NULL),
      map(ColorMapType::SOLID_COLOR_RED, "modelNum"),
      xsplines(NULL),
      ysplines(NULL),
      zsplines(NULL)
{
    q_vec_set(forceAccum, 0, 0, 0);
    q_vec_set(torqueAccum, 0, 0, 0);
    q_vec_set(position, 0, 0, 0);
    q_vec_set(lastPosition, 0, 0, 0);
    q_from_axis_angle(orientation, 1, 0, 0, 0);
    q_from_axis_angle(lastOrientation, 1, 0, 0, 0);
    recalculateLocalTransform();
}

//#########################################################################
SketchObject::~SketchObject() {
	// Notify observers that this object is being deleted
    foreach(ObjectChangeObserver * obs, observers)
    {
        obs->objectDeleted(this);
    }
}

//#########################################################################
SketchObject *SketchObject::getParent() { return parent; }

//#########################################################################
const SketchObject *SketchObject::getParent() const { return parent; }

//#########################################################################
void SketchObject::setParent(SketchObject *p)
{
    parent = p;
    recalculateLocalTransform();
}

//#########################################################################
QList< SketchObject * > *SketchObject::getSubObjects() { return NULL; }

//#########################################################################
const QList< SketchObject * > *SketchObject::getSubObjects() const
{
    return NULL;
}

//#########################################################################
SketchModel *SketchObject::getModel() { return NULL; }
//#########################################################################
const SketchModel *SketchObject::getModel() const { return NULL; }
//#########################################################################
int SketchObject::getModelConformation() const { return -1; }
//#########################################################################
vtkActor *SketchObject::getActor() { return NULL; }
//#########################################################################
void SketchObject::getPosition(q_vec_type dest) const
{
    if (parent != NULL || localTransformDefiningPosition) {
        localTransform->GetPosition(dest);
    } else {
        q_vec_copy(dest, position);
    }
}
//#########################################################################
void SketchObject::getOrientation(q_type dest) const
{
    if (parent != NULL || localTransformDefiningPosition) {
        double wxyz[4];
        localTransform->GetOrientationWXYZ(wxyz);
        q_from_axis_angle(dest, wxyz[1], wxyz[2], wxyz[3],
                          wxyz[0] * Q_PI / 180.0);
    } else {
        q_copy(dest, orientation);
    }
}
//#########################################################################
void SketchObject::getOrientation(double matrix[3][3]) const
{
    q_type tmp;
    getOrientation(tmp);  // simpler than duplicating case code
    quatToPQPMatrix(tmp, matrix);
}

//#########################################################################
void SketchObject::setPosition(const q_vec_type newPosition)
{
    q_vec_copy(position, newPosition);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::setOrientation(const q_type newOrientation)
{
    q_copy(orientation, newOrientation);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::setPosAndOrient(const q_vec_type newPosition,
                                   const q_type newOrientation)
{
    q_vec_copy(position, newPosition);
    q_copy(orientation, newOrientation);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::getLastPosition(q_vec_type dest) const
{
    q_vec_copy(dest, lastPosition);
}
//#########################################################################
void SketchObject::getLastOrientation(q_type dest) const
{
    q_copy(dest, lastOrientation);
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
    if (parent == NULL) {
        setPosAndOrient(lastPosition, lastOrientation);
    } else {
        setParentRelativePositionForAbsolutePosition(this, parent, lastPosition,
                                                     lastOrientation);
    }
}

//#########################################################################
int SketchObject::getPrimaryCollisionGroupNum()
{
    if (collisionGroups.empty()) {
        return OBJECT_HAS_NO_GROUP;
    } else {
        return collisionGroups[0];
    }
}

//#########################################################################
void SketchObject::setPrimaryCollisionGroupNum(int num)
{
    if (collisionGroups.contains(num)) {
        collisionGroups.removeOne(num);
    }
    collisionGroups.prepend(num);
}
//#########################################################################
void SketchObject::addToCollisionGroup(int num)
{
    if (collisionGroups.contains(num) || num == OBJECT_HAS_NO_GROUP) return;
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
vtkTransform *SketchObject::getLocalTransform() { return localTransform; }
//#########################################################################
vtkLinearTransform *SketchObject::getInverseLocalTransform()
{
    return invLocalTransform;
}

//#########################################################################
void SketchObject::getModelSpacePointInWorldCoordinates(
    const q_vec_type modelPoint, q_vec_type worldCoordsOut) const
{
    localTransform->TransformPoint(modelPoint, worldCoordsOut);
}
//#########################################################################
void SketchObject::getWorldSpacePointInModelCoordinates(
    const q_vec_type worldPoint, q_vec_type modelCoordsOut) const
{
    invLocalTransform->TransformPoint(worldPoint, modelCoordsOut);
}
//#########################################################################
void SketchObject::getWorldVectorInModelSpace(const q_vec_type worldVec,
                                              q_vec_type modelVecOut) const
{
    invLocalTransform->TransformVector(worldVec, modelVecOut);
}
//#########################################################################
void SketchObject::getModelVectorInWorldSpace(const q_vec_type modelVec,
                                              q_vec_type worldVecOut) const
{
    localTransform->TransformVector(modelVec, worldVecOut);
}
//#########################################################################
void SketchObject::addForce(const q_vec_type point, const q_vec_type force)
{
    if (!propagateForce || parent == NULL) {
        q_vec_add(forceAccum, forceAccum, force);
        q_vec_type tmp, torque;
        getWorldVectorInModelSpace(force, tmp);
        q_vec_cross_product(torque, point, tmp);
        q_vec_add(torqueAccum, torqueAccum, torque);
    } else {
        q_vec_type tmp;
        // get point in parent coordinate system
        getModelSpacePointInWorldCoordinates(point, tmp);
        parent->addForce(tmp, force);
    }
    notifyForceObservers();
}

//#########################################################################
void SketchObject::getForce(q_vec_type out) const
{
    q_vec_copy(out, forceAccum);
}
//#########################################################################
void SketchObject::getTorque(q_vec_type out) const
{
    q_vec_copy(out, torqueAccum);
}
//#########################################################################
void SketchObject::setForceAndTorque(const q_vec_type force,
                                     const q_vec_type torque)
{
    q_vec_copy(forceAccum, force);
    q_vec_copy(torqueAccum, torque);
    notifyForceObservers();
}

//#########################################################################
void SketchObject::clearForces()
{
    q_vec_set(forceAccum, 0, 0, 0);
    q_vec_set(torqueAccum, 0, 0, 0);
}

//#########################################################################
void SketchObject::setLocalTransformPrecomputed(bool isComputed)
{
    localTransformPrecomputed = isComputed;
    if (!isComputed) {
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
    if (keyframes.isNull()) {
        return 0;
    } else {
        return keyframes->size();
    }
}

//#########################################################################
const QMap< double, Keyframe > *SketchObject::getKeyframes() const
{
    return keyframes.data();
}

//#########################################################################
int SketchObject::getGroupingLevel()
{
    // Returns 0 if not in a group
    int groupLevel = 0;
    SketchObject *levelObj;
    SketchObject *levelParent = getParent();

    while (levelParent != NULL) {
        groupLevel++;
        levelObj = levelParent;
        levelParent = levelObj->getParent();
    }

    return groupLevel;
}

//#########################################################################
void SketchObject::addKeyframeForCurrentLocation(double t)
{
    if (t < 0) {  // no negative times allowed
        return;
    }
    q_vec_type keyframe_position;
    q_type keyframe_orientation;
    if (getParent() == NULL) {
        q_vec_copy(keyframe_position, position);
        q_copy(keyframe_orientation, orientation);
    } else {
        getPosition(keyframe_position);
        getOrientation(keyframe_orientation);
    }
    Keyframe frame(position, keyframe_position, orientation,
                   keyframe_orientation, getColorMapType(), getArrayToColorBy(),
                   getGroupingLevel(), getParent(), visible, active);
    if (keyframes.isNull()) {
        keyframes.reset(new QMap< double, Keyframe >());
    }
    keyframes->insert(t, frame);
    QList< SketchObject * > *subObjects = getSubObjects();
    if (subObjects != NULL) {
        for (int i = 0; i < subObjects->length(); i++) {
            subObjects->at(i)->addKeyframeForCurrentLocation(t);
        }
    }
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectKeyframed(this, t);
    }
    if (keyframes->size() > 1) {
        computeSplines();
    }
}

//#########################################################################
void SketchObject::insertKeyframe(double time, const Keyframe &keyframe)
{
    if (time < 0) {  // no negative times allowed
        return;
    }
    if (keyframes.isNull()) {
        keyframes.reset(new QMap< double, Keyframe >());
    }
    keyframes->insert(time, keyframe);
    if (keyframes->size() > 1) {
        computeSplines();
    }
}

//#########################################################################
bool SketchObject::hasChangedSinceKeyframe(double t)
{
    if (keyframes.isNull() || !keyframes->contains(t)) {
        return true;
    }
    const Keyframe &frame = keyframes->value(t);
    q_vec_type framePos;
    q_type frameOrient;
    frame.getPosition(framePos);
    frame.getOrientation(frameOrient);
    const ColorMapType::ColorMap &frameCMap = frame.getColorMap();
    bool a, b, c, d, e, f, g;
    a = q_vec_equals(position, framePos);
    b = q_equals(orientation, frameOrient);
    c = map.first == frameCMap.first;
    d = map.isSolidColor();
    e = frameCMap.isSolidColor();
    f = frameCMap.second == map.second;
    g = (isVisible() == frame.isVisibleAfter()) &&
        (isActive() == frame.isActive());
    bool objectChanged = !(a && b && c && ((d && e) || f) && g);
    if (objectChanged) {
        return true;
    }
    QList< SketchObject * > *subObjects = getSubObjects();
    if (subObjects != NULL) {
        bool changed = false;
        for (int i = 0; i < subObjects->length() && !changed; i++) {
            changed = changed || subObjects->at(i)->hasChangedSinceKeyframe(t);
        }
        return changed;
    } else {
        return false;
    }
}

//#########################################################################
void SketchObject::removeKeyframeForTime(double t)
{
    if (keyframes.isNull()) {
        return;
    }
    if (keyframes->contains(t)) {
        keyframes->remove(t);
    }
    computeSplines();
}

//#########################################################################
void SketchObject::clearKeyframes()
{
    if (keyframes.isNull()) {
        return;
    }
    keyframes->clear();
}

//#########################################################################
void SketchObject::setPositionByAnimationTime(double t)
{
    // we don't support negative times and if the object has no keyframes, then
    // no need to do anything
    if (t < 0 || !hasKeyframes()) {
        return;
    }
    QMapIterator< double, Keyframe > it(*keyframes.data());
    // last is the last keyframe we passed
    double last = it.peekNext().key();
    // go until the next one is greater than the time (the last one
    // will be less than the time unless the time is less than the first
    // time in the keyframes list
    while (it.hasNext() && it.peekNext().key() < t) {
        last = it.next().key();
    }
    // if we are after the end of the last keyframe defined
    if (!it.hasNext()) {
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
    } else if (it.peekNext().key() == t || it.peekNext().key() == last) {
        Keyframe f = it.next().value();
        f.getPosition(position);
        f.getOrientation(orientation);

        // set color map stuff here
        setColorMapType(f.getColorMapType());
        setArrayToColorBy(f.getArrayToColorBy());
        setIsVisible(f.isVisibleAfter());
        setActive(f.isActive());
    } else {
        // if we have a next keyframe that is greater than the time
        double next = it.peekNext().key();
        // note, if the first keyframe is after the time given, these will
        // be the same and the state will be set to the state at that keyframe
        Keyframe f1 = keyframes->value(last), f2 = it.next().value();
        double diff1 = next - last;
        double diff2 = t - last;
        double ratio = diff2 / diff1;

        // if object is in a group, just keep its position static relative to
        // the group
        if (getParent() != NULL) {
            f1.getPosition(position);
            f1.getOrientation(orientation);
        } else {  // otherwise, find the spline for the current time and
                  // evaluate position there
            // spline interpolation for position
            q_vec_type pos;
            QMapIterator< double, vtkSmartPointer< vtkCardinalSpline > > it(
                *xsplines.data());
            // last is the last keyframe we passed
            double last = it.peekNext().key();
            while (it.hasNext() && it.peekNext().key() < t) {
                last = it.next().key();
            }

            double spline_end = last;
            if (it.hasNext()) {
                spline_end = it.peekNext().key();
            }
            vtkSmartPointer< vtkCardinalSpline > xspline =
                xsplines->value(spline_end);
            vtkSmartPointer< vtkCardinalSpline > yspline =
                ysplines->value(spline_end);
            vtkSmartPointer< vtkCardinalSpline > zspline =
                zsplines->value(spline_end);
            vtkSmartPointer< vtkCardinalSpline > yaw_spline =
                yaw_splines->value(spline_end);
            vtkSmartPointer< vtkCardinalSpline > pitch_spline =
                pitch_splines->value(spline_end);
            vtkSmartPointer< vtkCardinalSpline > roll_spline =
                roll_splines->value(spline_end);

            pos[0] = xspline->Evaluate(t);
            pos[1] = yspline->Evaluate(t);
            pos[2] = zspline->Evaluate(t);
            setPosition(pos);

            q_type orient;
            q_from_euler(orient, yaw_spline->Evaluate(t),
                         pitch_spline->Evaluate(t), roll_spline->Evaluate(t));
            setOrientation(orient);
        }

        if (numInstances() == 1) {
            // set color map stuff here
            const ColorMapType::ColorMap &c1 = f1.getColorMap(),
                                         &c2 = f2.getColorMap();
            if (c1.isSolidColor() && c2.isSolidColor()) {
                vtkSmartPointer< vtkColorTransferFunction > map =
                    vtkSmartPointer< vtkColorTransferFunction >::Take(
                        c1.getColorMap(0, 1));
                double color1[3], color2[3], netColor[3];
                map->GetColor(1.0, color1);
                map.TakeReference(c2.getColorMap(0, 1));
                map->GetColor(1.0, color2);
                double tmpC1[3], tmpC2[3];
                q_vec_scale(tmpC2, ratio, color2);
                q_vec_scale(tmpC1, 1.0 - ratio, color1);
                q_vec_add(netColor, tmpC1, tmpC2);
                setSolidColor(netColor);
            } else {
                setColorMapType(f1.getColorMapType());
                setArrayToColorBy(f1.getArrayToColorBy());
            }
        }
        // set visibility stuff here
        setIsVisible(f1.isVisibleAfter());
        setActive(f1.isActive());
    }
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
    recalculateLocalTransform();
    // set out own position first to avoid messing up sub-object positions
    QList< SketchObject * > *subObjects = getSubObjects();
    if (subObjects != NULL) {
        for (QListIterator< SketchObject * > itr(*subObjects); itr.hasNext();) {
            itr.next()->setPositionByAnimationTime(t);
        }
    }
}

//#########################################################################
void SketchObject::computeSplines()
{
    double pi = 4 * atan(1.0);
    vtkSmartPointer< vtkCardinalSpline > xspline =
        vtkSmartPointer< vtkCardinalSpline >::New();
    vtkSmartPointer< vtkCardinalSpline > yspline =
        vtkSmartPointer< vtkCardinalSpline >::New();
    vtkSmartPointer< vtkCardinalSpline > zspline =
        vtkSmartPointer< vtkCardinalSpline >::New();
    vtkSmartPointer< vtkCardinalSpline > yaw_spline =
        vtkSmartPointer< vtkCardinalSpline >::New();
    vtkSmartPointer< vtkCardinalSpline > pitch_spline =
        vtkSmartPointer< vtkCardinalSpline >::New();
    vtkSmartPointer< vtkCardinalSpline > roll_spline =
        vtkSmartPointer< vtkCardinalSpline >::New();
    xsplines.reset(new QMap< double, vtkSmartPointer< vtkCardinalSpline > >());
    ysplines.reset(new QMap< double, vtkSmartPointer< vtkCardinalSpline > >());
    zsplines.reset(new QMap< double, vtkSmartPointer< vtkCardinalSpline > >());
    yaw_splines.reset(
        new QMap< double, vtkSmartPointer< vtkCardinalSpline > >());
    pitch_splines.reset(
        new QMap< double, vtkSmartPointer< vtkCardinalSpline > >());
    roll_splines.reset(
        new QMap< double, vtkSmartPointer< vtkCardinalSpline > >());

    if (hasKeyframes()) {
        QMapIterator< double, Keyframe > it(*keyframes.data());
        q_vec_type last_or;
        int last_level = 1;
        double next = it.peekNext().key();
        Keyframe f = keyframes->value(next);
        q_type orient;
        f.getOrientation(orient);
        q_to_euler(last_or, orient);
        bool first_keyframe = true;

        while (it.hasNext()) {
            next = it.next().key();
            f = keyframes->value(next);
            q_vec_type pos;
            f.getAbsolutePosition(pos);
            f.getAbsoluteOrientation(orient);
            q_vec_type euler;
            q_to_euler(euler, orient);
            int current_level = f.getLevel();

            // Make sure we choose the shortest path instead of rotating around
            // the long way
            if (yaw_spline->GetNumberOfPoints() > 0) {
                for (int i = 0; i < 3; i++) {
                    while (fabs(euler[i] - last_or[i]) > pi) {
                        if (euler[i] - last_or[i] > pi) {
                            euler[i] -= 2 * pi;
                        } else if (euler[i] - last_or[i] < -pi) {
                            euler[i] += 2 * pi;
                        }
                    }
                }
            }

            // Add to, finish, or start splines based on grouping status
            if (first_keyframe) {
                first_keyframe = false;
                if (current_level == 0) {
                    // start off not in a group, so create the first spline
                    xspline = vtkSmartPointer< vtkCardinalSpline >::New();
                    yspline = vtkSmartPointer< vtkCardinalSpline >::New();
                    zspline = vtkSmartPointer< vtkCardinalSpline >::New();
                    yaw_spline = vtkSmartPointer< vtkCardinalSpline >::New();
                    pitch_spline = vtkSmartPointer< vtkCardinalSpline >::New();
                    roll_spline = vtkSmartPointer< vtkCardinalSpline >::New();

                    xspline->AddPoint(next, pos[0]);
                    yspline->AddPoint(next, pos[1]);
                    zspline->AddPoint(next, pos[2]);
                    yaw_spline->AddPoint(next, euler[0]);
                    pitch_spline->AddPoint(next, euler[1]);
                    roll_spline->AddPoint(next, euler[2]);
                }
            } else {
                // if level changes:
                if (current_level != last_level) {
                    if (last_level == 0) {
                        // we are entering a grouped phase, so add ending point
                        // of spline and compute
                        xspline->AddPoint(next, pos[0]);
                        yspline->AddPoint(next, pos[1]);
                        zspline->AddPoint(next, pos[2]);
                        yaw_spline->AddPoint(next, euler[0]);
                        pitch_spline->AddPoint(next, euler[1]);
                        roll_spline->AddPoint(next, euler[2]);

                        xspline->Compute();
                        yspline->Compute();
                        zspline->Compute();
                        yaw_spline->Compute();
                        pitch_spline->Compute();
                        roll_spline->Compute();

                        xsplines->insert(next, xspline);
                        ysplines->insert(next, yspline);
                        zsplines->insert(next, zspline);
                        yaw_splines->insert(next, yaw_spline);
                        pitch_splines->insert(next, pitch_spline);
                        roll_splines->insert(next, roll_spline);
                    } else {
                        if (current_level == 0) {
                            xspline->AddPoint(next, pos[0]);
                            yspline->AddPoint(next, pos[1]);
                            zspline->AddPoint(next, pos[2]);
                            yaw_spline->AddPoint(next, euler[0]);
                            pitch_spline->AddPoint(next, euler[1]);
                            roll_spline->AddPoint(next, euler[2]);
                        }
                    }
                } else {
                    if (current_level == 0) {
                        // currently in an ungrouped phase, so add a point to
                        // the current spline
                        xspline->AddPoint(next, pos[0]);
                        yspline->AddPoint(next, pos[1]);
                        zspline->AddPoint(next, pos[2]);
                        yaw_spline->AddPoint(next, euler[0]);
                        pitch_spline->AddPoint(next, euler[1]);
                        roll_spline->AddPoint(next, euler[2]);
                    } else if (it.hasNext()) {
                        double next_time = it.peekNext().key();
                        Keyframe frame = keyframes->value(next_time);
                        if (frame.getLevel() == 0) {
                            // will come out of grouped phase in next keyframe,
                            // so start new spline
                            xspline =
                                vtkSmartPointer< vtkCardinalSpline >::New();
                            yspline =
                                vtkSmartPointer< vtkCardinalSpline >::New();
                            zspline =
                                vtkSmartPointer< vtkCardinalSpline >::New();
                            yaw_spline =
                                vtkSmartPointer< vtkCardinalSpline >::New();
                            pitch_spline =
                                vtkSmartPointer< vtkCardinalSpline >::New();
                            roll_spline =
                                vtkSmartPointer< vtkCardinalSpline >::New();

                            xspline->AddPoint(next, pos[0]);
                            yspline->AddPoint(next, pos[1]);
                            zspline->AddPoint(next, pos[2]);
                            yaw_spline->AddPoint(next, euler[0]);
                            pitch_spline->AddPoint(next, euler[1]);
                            roll_spline->AddPoint(next, euler[2]);
                        }
                    }
                }

                if (current_level == 0 && !it.hasNext()) {
                    xspline->Compute();
                    yspline->Compute();
                    zspline->Compute();
                    yaw_spline->Compute();
                    pitch_spline->Compute();
                    roll_spline->Compute();

                    xsplines->insert(next, xspline);
                    ysplines->insert(next, yspline);
                    zsplines->insert(next, zspline);
                    yaw_splines->insert(next, yaw_spline);
                    pitch_splines->insert(next, pitch_spline);
                    roll_splines->insert(next, roll_spline);
                }
            }

            q_vec_copy(last_or, euler);
            last_level = current_level;
        }
    }

    QList< SketchObject * > *subObjects = getSubObjects();
    if (subObjects != NULL) {
        for (int i = 0; i < subObjects->length(); i++) {
            subObjects->at(i)->computeSplines();
        }
    }
}

//#########################################################################
void SketchObject::getPosAndOrFromSpline(q_vec_type pos_dest, q_type or_dest,
                                         double t)
{
    q_vec_type pos;
    QMapIterator< double, vtkSmartPointer< vtkCardinalSpline > > it(
        *xsplines.data());
    // last is the last keyframe we passed
    double last = it.peekNext().key();
    while (it.hasNext() && it.peekNext().key() < t) {
        last = it.next().key();
    }

    double spline_end = last;
    if (it.hasNext()) {
        spline_end = it.peekNext().key();
    }
    vtkSmartPointer< vtkCardinalSpline > xspline = xsplines->value(spline_end);
    vtkSmartPointer< vtkCardinalSpline > yspline = ysplines->value(spline_end);
    vtkSmartPointer< vtkCardinalSpline > zspline = zsplines->value(spline_end);
    vtkSmartPointer< vtkCardinalSpline > yaw_spline =
        yaw_splines->value(spline_end);
    vtkSmartPointer< vtkCardinalSpline > pitch_spline =
        pitch_splines->value(spline_end);
    vtkSmartPointer< vtkCardinalSpline > roll_spline =
        roll_splines->value(spline_end);

    pos[0] = xspline->Evaluate(t);
    pos[1] = yspline->Evaluate(t);
    pos[2] = zspline->Evaluate(t);
    q_vec_copy(pos_dest, pos);
    q_from_euler(or_dest, yaw_spline->Evaluate(t), pitch_spline->Evaluate(t),
                 roll_spline->Evaluate(t));
}

//#########################################################################
void SketchObject::addObserver(ObjectChangeObserver *obs)
{
    observers.insert(obs);
}

//#########################################################################
void SketchObject::removeObserver(ObjectChangeObserver *obs)
{
    assert(observers.contains(obs));
    observers.remove(obs);
}

//#########################################################################
void SketchObject::recalculateLocalTransform()
{
    if (isLocalTransformPrecomputed()) {
        localTransformUpdated();
        return;
    }
    setTransformToPositionAndOrientation(localTransform.GetPointer(), position,
                                         orientation);
    if (parent != NULL) {
        localTransform->Concatenate(parent->getLocalTransform());
    }
    localTransform->Update();
    localTransformUpdated();
}

//#########################################################################
void SketchObject::localTransformUpdated() {}

//#########################################################################
void SketchObject::localTransformUpdated(SketchObject *obj)
{
    obj->localTransformUpdated();
}

//#########################################################################
void SketchObject::notifyForceObservers()
{
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectPushed(this);
    }
}

//#########################################################################
void SketchObject::setIsVisible(bool isVisible)
{
    if (visible != isVisible) {
        foreach(ObjectChangeObserver * obs, observers)
        {
            obs->objectVisibilityChanged(this);
        }
    }
    visible = isVisible;
}

//#########################################################################
void SketchObject::setIsVisibleRecursive(SketchObject *obj, bool isVisible)
{
    obj->setIsVisible(isVisible);
    QList< SketchObject * > *children = obj->getSubObjects();
    if (children != NULL) {
        for (int i = 0; i < children->size(); i++) {
            setIsVisibleRecursive(children->at(i), isVisible);
        }
    }
}

//#########################################################################
bool SketchObject::isVisible() const { return visible; }

//#########################################################################
void SketchObject::setActive(bool isActive) { active = isActive; }

//#########################################################################
bool SketchObject::isActive() const { return active; }

//#########################################################################
void SketchObject::setPropagateForceToParent(bool propagate)
{
    propagateForce = propagate;
}

//#########################################################################
bool SketchObject::isPropagatingForceToParent() { return propagateForce; }

//#########################################################################
void SketchObject::notifyObjectAdded(SketchObject *child)
{
    foreach(ObjectChangeObserver * obs, observers)
    {
        obs->subobjectAdded(this, child);
    }
}
//#########################################################################
void SketchObject::notifyObjectRemoved(SketchObject *child)
{
    foreach(ObjectChangeObserver * obs, observers)
    {
        obs->subobjectRemoved(this, child);
    }
}

void SketchObject::makeDeepCopyOf(SketchObject *other)
{
    q_vec_copy(position, other->position);
    q_copy(orientation, other->orientation);
    visible = other->visible;
    active = other->active;
    propagateForce = other->propagateForce;
    if (other->keyframes.data() != NULL) {
        keyframes.reset(new QMap< double, Keyframe >(*other->keyframes.data()));
    }
}
