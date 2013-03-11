#include "sketchobject.h"
#include <physicsstrategy.h>
#include <sketchtests.h>
#include <vtkExtractEdges.h>
#include <vtkCubeSource.h>

//#########################################################################
SketchObject::SketchObject() :
    localTransform(vtkSmartPointer<vtkTransform>::New()),
    parent(NULL),
    primaryCollisionGroup(OBJECT_HAS_NO_GROUP),
    localTransformPrecomputed(false),
    observers()
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
vtkActor *SketchObject::getActor() {
    return NULL;
}
//#########################################################################
void SketchObject::getPosition(q_vec_type dest) const {
    if (parent != NULL) {
        localTransform->GetPosition(dest);
    } else {
        q_vec_copy(dest,position);
    }
}
//#########################################################################
void SketchObject::getOrientation(q_type dest) const {
    if (parent != NULL) {
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
}

//#########################################################################
void SketchObject::setOrientation(const q_type newOrientation) {
    q_copy(orientation,newOrientation);
    recalculateLocalTransform();
}

//#########################################################################
void SketchObject::setPosAndOrient(const q_vec_type newPosition, const q_type newOrientation) {
    q_vec_copy(position,newPosition);
    q_copy(orientation,newOrientation);
    recalculateLocalTransform();
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
    return primaryCollisionGroup;
}

//#########################################################################
void SketchObject::setPrimaryCollisionGroupNum(int num) {
    primaryCollisionGroup = num;
}

//#########################################################################
bool SketchObject::isInCollisionGroup(int num) const {
    if (parent != NULL) {
        return parent->isInCollisionGroup(num);
    }
    return (num != OBJECT_HAS_NO_GROUP) ? num == primaryCollisionGroup: false;
}
//#########################################################################
vtkTransform *SketchObject::getLocalTransform() {
    return localTransform;
}

//#########################################################################
void SketchObject::getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint,
                                                                q_vec_type worldCoordsOut) const {
    localTransform->TransformPoint(modelPoint,worldCoordsOut);
}
//#########################################################################
void SketchObject::getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const {
    localTransform->Inverse();
    localTransform->TransformVector(worldVec,modelVecOut);
    localTransform->Inverse();
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
    notifyObservers();
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
    notifyObservers();
}

//#########################################################################
void SketchObject::clearForces() {
    q_vec_set(forceAccum,0,0,0);
    q_vec_set(torqueAccum,0,0,0);
}

//#########################################################################
void SketchObject::addForceObserver(ObjectForceObserver *obs) {
    if (!observers.contains(obs)) {
        observers.append(obs);
    }
}

//#########################################################################
void SketchObject::removeForceObserver(ObjectForceObserver *obs) {
    if (observers.contains(obs)) {
        observers.removeOne(obs);
    }
}

//#########################################################################
void SketchObject::recalculateLocalTransform()  {
    if (isLocalTransformPrecomputed())
        return;
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
void SketchObject::setLocalTransformPrecomputed(bool isComputed) {
    localTransformPrecomputed = isComputed;
}

//#########################################################################
bool SketchObject::isLocalTransformPrecomputed() {
    return localTransformPrecomputed;
}

//#########################################################################
void SketchObject::localTransformUpdated() {}

//#########################################################################
void SketchObject::localTransformUpdated(SketchObject *obj) {
    obj->localTransformUpdated();
}

//#########################################################################
void SketchObject::notifyObservers() {
    for (int i = 0; i < observers.size(); i++) {
        observers[i]->objectPushed(this);
    }
}

//#########################################################################
//#########################################################################
//#########################################################################
// Model Instance class
//#########################################################################
//#########################################################################
//#########################################################################
ModelInstance::ModelInstance(SketchModel *m) :
    SketchObject(),
    actor(vtkSmartPointer<vtkActor>::New()),
    model(m),
    modelTransformed(vtkSmartPointer<vtkTransformPolyDataFilter>::New()),
    orientedBB(vtkSmartPointer<vtkTransformPolyDataFilter>::New()),
    solidMapper(vtkSmartPointer<vtkPolyDataMapper>::New())
{
    vtkSmartPointer<vtkCubeSource> cube = vtkSmartPointer<vtkCubeSource>::New();
    cube->SetBounds(model->getModelData()->GetOutput()->GetBounds());
    cube->Update();
    vtkSmartPointer<vtkExtractEdges> cubeEdges = vtkSmartPointer<vtkExtractEdges>::New();
    cubeEdges->SetInputConnection(cube->GetOutputPort());
    cubeEdges->Update();
    orientedBB->SetInputConnection(cubeEdges->GetOutputPort());
    orientedBB->SetTransform(getLocalTransform());
    orientedBB->Update();
    modelTransformed->SetInputConnection(model->getModelData()->GetOutputPort());
    modelTransformed->SetTransform(getLocalTransform());
    modelTransformed->Update();
    solidMapper->SetInputConnection(modelTransformed->GetOutputPort());
    solidMapper->Update();
    actor->SetMapper(solidMapper);
}

//#########################################################################
ModelInstance::~ModelInstance() {}

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
        PQP_Collide(cr,r1,t1,model->getCollisionModel(),r2,t2,other->getModel()->getCollisionModel(),pqp_flags);
        if (cr->NumPairs() != 0) {
            physics->respondToCollision(this,other,cr,pqp_flags);
        }
        return cr->NumPairs() != 0;
    }
}

//#########################################################################
void ModelInstance::getAABoundingBox(double bb[]) {
    modelTransformed->GetOutput()->GetBounds(bb);
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
void ObjectGroup::getAABoundingBox(double bb[]) {
    if (children.size() == 0) {
        bb[0] = bb[1] = bb[2] = bb[3] = bb[4] = bb[5] = 0;
    } else {
        double cbb[6];
        children[0]->getAABoundingBox(bb);
        for (int i = 1; i < children.size(); i++) {
            children[i]->getAABoundingBox(cbb);
            bb[0] = min(bb[0],cbb[0]);
            bb[2] = min(bb[2],cbb[2]);
            bb[4] = min(bb[4],cbb[4]);
            bb[1] = max(bb[1],cbb[1]);
            bb[3] = max(bb[3],cbb[3]);
            bb[5] = max(bb[5],cbb[5]);
        }
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

