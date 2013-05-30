#include "sketchobject.h"
#include <physicsstrategy.h>
#include <sketchtests.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkExtractEdges.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>

//#########################################################################
SketchObject::SketchObject() :
    localTransform(vtkSmartPointer<vtkTransform>::New()),
    invLocalTransform(localTransform->GetLinearInverse()),
    parent(NULL),
    visible(true),
    active(false),
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
SketchObject::~SketchObject() {}

//#########################################################################
SketchObject *SketchObject::getParent() {
    return parent;
}

//#########################################################################
const SketchObject *SketchObject::getParent() const {
    return parent;
}

//#########################################################################
void SketchObject::setParent(SketchObject *p) {
    parent = p;
    recalculateLocalTransform();
}

//#########################################################################
QList<SketchObject *> *SketchObject::getSubObjects() {
    return NULL;
}

//#########################################################################
const QList<SketchObject *> *SketchObject::getSubObjects() const {
    return NULL;
}

//#########################################################################
SketchModel *SketchObject::getModel() {
    return NULL;
}
//#########################################################################
const SketchModel *SketchObject::getModel() const {
    return NULL;
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
void SketchObject::getPosition(q_vec_type dest) const {
    if (parent != NULL || localTransformDefiningPosition) {
        localTransform->GetPosition(dest);
    } else {
        q_vec_copy(dest,position);
    }
}
//#########################################################################
void SketchObject::getOrientation(q_type dest) const {
    if (parent != NULL || localTransformDefiningPosition) {
        double wxyz[4];
        localTransform->GetOrientationWXYZ(wxyz);
        q_from_axis_angle(dest,wxyz[1],wxyz[2],wxyz[3],wxyz[0]*Q_PI/180.0);
    } else {
        q_copy(dest,orientation);
    }
}
//#########################################################################
void SketchObject::getOrientation(PQP_REAL matrix[3][3]) const {
    q_type tmp;
    getOrientation(tmp); // simpler than duplicating case code
    quatToPQPMatrix(tmp,matrix);
}

//#########################################################################
void SketchObject::setPosition(const q_vec_type newPosition) {
    q_vec_copy(position,newPosition);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::setOrientation(const q_type newOrientation) {
    q_copy(orientation,newOrientation);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::setPosAndOrient(const q_vec_type newPosition, const q_type newOrientation) {
    q_vec_copy(position,newPosition);
    q_copy(orientation,newOrientation);
    recalculateLocalTransform();
    for (QSetIterator< ObjectChangeObserver * > it(observers); it.hasNext();) {
        it.next()->objectMoved(this);
    }
}

//#########################################################################
void SketchObject::getLastPosition(q_vec_type dest) const {
    q_vec_copy(dest,lastPosition);
}
//#########################################################################
void SketchObject::getLastOrientation(q_type dest) const {
    q_copy(dest,lastOrientation);
}
//#########################################################################
void SketchObject::setLastLocation() {
    getPosition(lastPosition);
    getOrientation(lastOrientation);
}

//#########################################################################
void SketchObject::restoreToLastLocation() {
    setPosAndOrient(lastPosition,lastOrientation);
}

//#########################################################################
int SketchObject::getPrimaryCollisionGroupNum() {
    if (parent != NULL) {
        return parent->getPrimaryCollisionGroupNum();
    }
    if (collisionGroups.empty()) {
        return OBJECT_HAS_NO_GROUP;
    } else {
        return collisionGroups[0];
    }
}

//#########################################################################
void SketchObject::setPrimaryCollisionGroupNum(int num) {
    if (collisionGroups.contains(num)) {
        collisionGroups.removeOne(num);
    }
    collisionGroups.prepend(num);
}
//#########################################################################
void SketchObject::addToCollisionGroup(int num) {
    if (collisionGroups.contains(num) || num == OBJECT_HAS_NO_GROUP)
        return;
    collisionGroups.append(num);
}

//#########################################################################
bool SketchObject::isInCollisionGroup(int num) const {
    if (parent != NULL) {
        return parent->isInCollisionGroup(num);
    }
    return collisionGroups.contains(num);
}
//#########################################################################
void SketchObject::removeFromCollisionGroup(int num) {
    collisionGroups.removeAll(num);
}
//#########################################################################
vtkTransform *SketchObject::getLocalTransform() {
    return localTransform;
}
//#########################################################################
vtkLinearTransform *SketchObject::getInverseLocalTransform() {
    return invLocalTransform;
}

//#########################################################################
void SketchObject::getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint,
                                                        q_vec_type worldCoordsOut) const {
    localTransform->TransformPoint(modelPoint,worldCoordsOut);
}
//#########################################################################
void SketchObject::getWorldSpacePointInModelCoordinates(const q_vec_type worldPoint,
                                                        q_vec_type modelCoordsOut) const {
    invLocalTransform->TransformPoint(worldPoint, modelCoordsOut);
}
//#########################################################################
void SketchObject::getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const {
    invLocalTransform->TransformVector(worldVec,modelVecOut);
}
//#########################################################################
void SketchObject::getModelVectorInWorldSpace(const q_vec_type modelVec, q_vec_type worldVecOut) const {
    localTransform->TransformVector(modelVec,worldVecOut);
}
//#########################################################################
void SketchObject::addForce(const q_vec_type point, const q_vec_type force) {
    if (parent == NULL) {
        q_vec_add(forceAccum,forceAccum,force);
        q_vec_type tmp, torque;
        getWorldVectorInModelSpace(force,tmp);
        q_vec_cross_product(torque,point,tmp);
        q_vec_add(torqueAccum,torqueAccum,torque);
    } else { // if we have a parent, then we are not moving ourself, the parent
                // should be doing the moving.  The object should be rigid relative
                // to the parent.
        q_vec_type tmp;
        q_xform(tmp,orientation,point);
        q_vec_add(tmp,position,tmp); // get point in parent coordinate system
        parent->addForce(tmp,force);
    }
    notifyForceObservers();
}

//#########################################################################
void SketchObject::getForce(q_vec_type out) const {
    q_vec_copy(out,forceAccum);
}
//#########################################################################
void SketchObject::getTorque(q_vec_type out) const {
    q_vec_copy(out,torqueAccum);
}
//#########################################################################
void SketchObject::setForceAndTorque(const q_vec_type force, const q_vec_type torque) {
    q_vec_copy(forceAccum,force);
    q_vec_copy(torqueAccum,torque);
    notifyForceObservers();
}

//#########################################################################
void SketchObject::clearForces() {
    q_vec_set(forceAccum,0,0,0);
    q_vec_set(torqueAccum,0,0,0);
}

//#########################################################################
void SketchObject::setLocalTransformPrecomputed(bool isComputed) {
    localTransformPrecomputed = isComputed;
    if (!isComputed) {
        recalculateLocalTransform();
    }
}

//#########################################################################
bool SketchObject::isLocalTransformPrecomputed() {
    return localTransformPrecomputed;
}

//#########################################################################
void SketchObject::setLocalTransformDefiningPosition(bool isDefining) {
    localTransformDefiningPosition = isDefining;
}

//#########################################################################
bool SketchObject::isLocalTransformDefiningPosition() {
    return localTransformDefiningPosition;
}

//#########################################################################
bool SketchObject::hasKeyframes() const {
    return !(keyframes.isNull() || keyframes->empty());
}

//#########################################################################
int SketchObject::getNumKeyframes() const {
    if (keyframes.isNull()) {
        return 0;
    } else {
        return keyframes->size();
    }
}

//#########################################################################
const QMap< double, Keyframe > *SketchObject::getKeyframes() const {
    return keyframes.data();
}

//#########################################################################
void SketchObject::addKeyframeForCurrentLocation(double t) {
    if (t < 0) { // no negative times allowed
        return;
    }
    Keyframe frame(position, orientation,visible,active);
    if (keyframes.isNull()) {
        keyframes.reset(new QMap< double, Keyframe >());
    }
    keyframes->insert(t,frame);
    for (QSetIterator<ObjectChangeObserver *> it(observers); it.hasNext();) {
        it.next()->objectKeyframed(this,t);
    }
}

//#########################################################################
void SketchObject::removeKeyframeForTime(double t) {
    if (keyframes->contains(t)) {
        keyframes->remove(t);
    }
}

//#########################################################################
void SketchObject::setPositionByAnimationTime(double t) {
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
        setIsVisible(f.isVisibleAfter());
        setActive(f.isActive());
    // if we happenned to land on a keyframe
    } else if (it.peekNext().key() == t) {
        Keyframe f = it.next().value();
        f.getPosition(position);
        f.getOrientation(orientation);
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
        // TODO -- set visibility stuff here
        setIsVisible(f1.isVisibleAfter());
        setActive(f1.isActive());
    }
    recalculateLocalTransform();
}

//#########################################################################
void SketchObject::addObserver(ObjectChangeObserver *obs) {
        observers.insert(obs);
}

//#########################################################################
void SketchObject::removeObserver(ObjectChangeObserver *obs) {
    observers.remove(obs);
}

//#########################################################################
void SketchObject::recalculateLocalTransform()  {
    if (isLocalTransformPrecomputed()) {
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
    if (parent != NULL) {
        localTransform->Concatenate(parent->getLocalTransform());
    }
    localTransform->Update();
    localTransformUpdated();
}

//#########################################################################
void SketchObject::localTransformUpdated() {}

//#########################################################################
void SketchObject::localTransformUpdated(SketchObject *obj) {
    obj->localTransformUpdated();
}

//#########################################################################
void SketchObject::notifyForceObservers() {
    for (QSetIterator<ObjectChangeObserver *> it(observers); it.hasNext(); ) {
        it.next()->objectPushed(this);
    }
}

//#########################################################################
void SketchObject::setIsVisible(bool isVisible) {
    visible = isVisible;
}

//#########################################################################
bool SketchObject::isVisible() const {
    return visible;
}

//#########################################################################
void SketchObject::setActive(bool isActive) {
    active = isActive;
}

//#########################################################################
bool SketchObject::isActive() const {
    return active;
}

//#########################################################################
//#########################################################################
//#########################################################################
// Model Instance class
//#########################################################################
//#########################################################################
//#########################################################################
ModelInstance::ModelInstance(SketchModel *m, int confNum) :
    SketchObject(),
    actor(vtkSmartPointer<vtkActor>::New()),
    model(m),
    conformation(confNum),
    modelTransformed(vtkSmartPointer<vtkTransformPolyDataFilter>::New()),
    orientedBB(vtkSmartPointer<vtkTransformPolyDataFilter>::New()),
    solidMapper(vtkSmartPointer<vtkPolyDataMapper>::New())
{
    vtkSmartPointer<vtkCubeSource> cube = vtkSmartPointer<vtkCubeSource>::New();
    cube->SetBounds(model->getVTKSource(conformation)->GetOutput()->GetBounds());
    cube->Update();
    vtkSmartPointer<vtkExtractEdges> cubeEdges = vtkSmartPointer<vtkExtractEdges>::New();
    cubeEdges->SetInputConnection(cube->GetOutputPort());
    cubeEdges->Update();
    orientedBB->SetInputConnection(cubeEdges->GetOutputPort());
    orientedBB->SetTransform(getLocalTransform());
    orientedBB->Update();
    modelTransformed->SetInputConnection(model->getVTKSource(conformation)->GetOutputPort());
    modelTransformed->SetTransform(getLocalTransform());
    modelTransformed->Update();
    solidMapper->SetInputConnection(modelTransformed->GetOutputPort());
    solidMapper->Update();
    actor->SetMapper(solidMapper);
    model->incrementUses(conformation);
}

//#########################################################################
ModelInstance::~ModelInstance() {
    model->decrementUses(conformation);
}

//#########################################################################
int ModelInstance::numInstances() const {
    return 1;
}

//#########################################################################
SketchModel *ModelInstance::getModel() {
    return model;
}

//#########################################################################
const SketchModel *ModelInstance::getModel() const {
    return model;
}

//#########################################################################
vtkActor *ModelInstance::getActor() {
    return actor;
}

//#########################################################################
int ModelInstance::getModelConformation() const {
    return conformation;
}

//#########################################################################
bool ModelInstance::collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags) {
    if (other->numInstances() != 1 || other->getModel() == NULL) {
        return other->collide(this,physics,pqp_flags);
    } else {
        PQP_CollideResult *cr = new PQP_CollideResult();
        PQP_REAL r1[3][3], r2[3][3], t1[3], t2[3];
        getPosition(t1);
        getOrientation(r1);
        other->getPosition(t2);
        other->getOrientation(r2);
        PQP_Collide(cr,r1,t1,model->getCollisionModel(conformation),r2,t2,
                    other->getModel()->getCollisionModel(conformation),pqp_flags);
        if (cr->NumPairs() != 0) {
            physics->respondToCollision(this,other,cr,pqp_flags);
        }
        return cr->NumPairs() != 0;
    }
}

//#########################################################################
void ModelInstance::getBoundingBox(double bb[]) {
    model->getVTKSource(conformation)->GetOutput()->GetBounds(bb);
}

//#########################################################################
vtkPolyDataAlgorithm *ModelInstance::getOrientedBoundingBoxes() {
    return orientedBB;
}

//#########################################################################
void ModelInstance::localTransformUpdated() {
    modelTransformed->Update();
}

//#########################################################################
SketchObject *ModelInstance::deepCopy() {
    SketchObject *nObj = new ModelInstance(model);
    q_vec_type pos;
    q_type orient;
    getPosition(pos);
    getOrientation(orient);
    nObj->setPosAndOrient(pos,orient);
    // TODO -- keyframes, etc...
    return nObj;
}

//#########################################################################
//#########################################################################
//#########################################################################
// Object Group class
//#########################################################################
//#########################################################################
//#########################################################################

//#########################################################################
ObjectGroup::ObjectGroup() :
    SketchObject(),
    children(),
    orientedBBs(vtkSmartPointer<vtkAppendPolyData>::New())
{
}

//#########################################################################
ObjectGroup::~ObjectGroup() {
    SketchObject *obj;
    for (int i = 0; i < children.length(); i++) {
        obj = children[i];
        delete obj;
        children[i] = (SketchObject *) NULL;
    }
    children.clear();
}

//#########################################################################
int ObjectGroup::numInstances() const {
    int sum = 0;
    for (int i = 0; i < children.length(); i++) {
        sum += children[i]->numInstances();
    }
    return sum;
}

//#########################################################################
SketchModel *ObjectGroup::getModel() {
    SketchModel *retVal = NULL;
    if (numInstances() == 1) {
        for (int i = 0; i < children.length(); i++) {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getModel();
        }
    }
    return retVal;
}

//#########################################################################
const SketchModel *ObjectGroup::getModel() const {
    const SketchModel *retVal = NULL;
    if (numInstances() == 1) {
        for (int i = 0; i < children.length(); i++) {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getModel();
        }
    }
    return retVal;
}

//#########################################################################
vtkActor *ObjectGroup::getActor() {
    vtkActor *retVal = NULL;
    if (numInstances() == 1) {
        for (int i = 0; i < children.length(); i++) {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getActor();
        }
    }
    return retVal;
}

//#########################################################################
void ObjectGroup::addObject(SketchObject *obj) {
    SketchObject *p = this;
    // no "I'm my own grandpa" allowed
    while (p != NULL) {
        if (obj == p) return;
        p = p->getParent();
    }
    // calculate new position & orientation
    if (children.empty()) { // special case when first thing is added
        q_vec_type pos, idV = Q_NULL_VECTOR;
        q_type idQ = Q_ID_QUAT;
        obj->getPosition(pos);
        setPosAndOrient(pos,idQ);
        // object gets position equal to group center, orientation is not messed with
        obj->setPosition(idV);
    } else { // if we already have some items in the list...
        q_vec_type pos, nPos, oPos;
        q_type idQ = Q_ID_QUAT;
        getPosition(pos);
        obj->getPosition(oPos);
        q_vec_scale(nPos,children.length(),pos);
        q_vec_add(nPos,oPos,nPos);
        q_vec_scale(nPos,1.0/(children.length()+1),nPos);
        // calculate change and apply it to children
        for (int i = 0; i < children.length(); i++) {
            q_vec_type cPos;
            q_type cOrient;
            children[i]->getPosition(cPos);
            children[i]->getOrientation(cOrient);
            q_vec_subtract(cPos,cPos,nPos);
            children[i]->setPosAndOrient(cPos,cOrient);
        }
        // set group's new position and orientation
        setPosAndOrient(nPos,idQ);
        // set the new object's position
        q_vec_subtract(oPos,oPos,nPos);
        obj->setPosition(oPos);
    }
    // set parent and child relation for new object
    obj->setParent(this);
    children.append(obj);
    orientedBBs->AddInputConnection(0,obj->getOrientedBoundingBoxes()->GetOutputPort(0));
    orientedBBs->Update();
}

//#########################################################################
void ObjectGroup::removeObject(SketchObject *obj) {
    q_vec_type pos, nPos, oPos;
    q_type oOrient, idQ = Q_ID_QUAT;
    // get inital positions/orientations
    getPosition(pos);
    obj->getPosition(oPos);
    obj->getOrientation(oOrient);
    // compute new group position...
    q_vec_scale(nPos,children.length(),pos);
    q_vec_subtract(nPos,nPos,oPos);
    q_vec_scale(nPos,1.0/(children.length()-1),nPos);
    // remove the item
    children.removeOne(obj);
    obj->setParent((SketchObject *)NULL);
    obj->setPosAndOrient(oPos,oOrient);
    orientedBBs->RemoveInputConnection(0,obj->getOrientedBoundingBoxes()->GetOutputPort(0));
    orientedBBs->Update();
    // apply the change to the group's children
    for (int i = 0; i < children.length(); i++) {
        q_vec_type cPos;
        q_type cOrient;
        children[i]->getPosition(cPos);
        children[i]->getOrientation(cOrient);
        q_vec_subtract(cPos,cPos,nPos);
        children[i]->setPosAndOrient(cPos,cOrient);
    }
    // reset the group's center and orientation
    setPosAndOrient(nPos,idQ);
}

//#########################################################################
QList<SketchObject *> *ObjectGroup::getSubObjects() {
    return &children;
}

//#########################################################################
const QList<SketchObject *> *ObjectGroup::getSubObjects() const {
    return &children;
}

//#########################################################################
bool ObjectGroup::collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags) {
    bool isCollision = false;
    for (int i = 0; i < children.length(); i++) {
        isCollision = isCollision || children[i]->collide(other,physics,pqp_flags);
        if (isCollision && pqp_flags == PQP_FIRST_CONTACT) {
            break;
        }
    }
    return isCollision;
}

//#########################################################################
void ObjectGroup::getBoundingBox(double bb[]) {
    if (children.size() == 0) {
        bb[0] = bb[1] = bb[2] = bb[3] = bb[4] = bb[5] = 0;
    } else {
        orientedBBs->GetOutput()->GetBounds(bb);
    }
}

//#########################################################################
vtkPolyDataAlgorithm *ObjectGroup::getOrientedBoundingBoxes() {
    return orientedBBs;
}

//#########################################################################
void ObjectGroup::localTransformUpdated() {
    for (int i = 0; i < children.size(); i++) {
        SketchObject::localTransformUpdated(children[i]);
    }
}

//#########################################################################
void ObjectGroup::setIsVisible(bool isVisible) {
    SketchObject::setIsVisible(isVisible);
    for (int i = 0; i < children.size(); i++) {
        children[i]->setIsVisible(isVisible);
    }
}

//#########################################################################
SketchObject *ObjectGroup::deepCopy() {
    ObjectGroup *nObj = new ObjectGroup();
    for (int i = 0; i < children.length(); i++) {
        nObj->addObject(children[i]->deepCopy());
    }
    // TODO - keyframes, etc.
    return nObj;
}
