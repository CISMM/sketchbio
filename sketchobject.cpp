#include "sketchobject.h"
#include <sketchtests.h>
#include <vtkExtractEdges.h>

SketchObject::SketchObject() :
    localTransform(vtkSmartPointer<vtkTransform>::New()),
    parent(NULL),
    primaryCollisionGroup(OBJECT_HAS_NO_GROUP),
    localTransformPrecomputed(false)
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
void SketchObject::setParent(SketchObject *p) {
    parent = p;
    recalculateLocalTransform();
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
    if (parent != NULL && parent->isInCollisionGroup(num)) {
        return true;
    }
    return num == primaryCollisionGroup;
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
    q_vec_add(forceAccum,forceAccum,force);
    q_vec_type tmp, torque;
    getWorldVectorInModelSpace(force,tmp);
    q_vec_cross_product(torque,point,tmp);
    q_vec_add(torqueAccum,torqueAccum,torque);
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
}

//#########################################################################
void SketchObject::clearForces() {
    q_vec_set(forceAccum,0,0,0);
    q_vec_set(torqueAccum,0,0,0);
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
    solidMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    wireframeMapper(vtkSmartPointer<vtkPolyDataMapper>::New())
{
    modelTransformed->SetInputConnection(model->getModelData()->GetOutputPort());
    modelTransformed->SetTransform(getLocalTransform());
    modelTransformed->Update();
    solidMapper->SetInputConnection(modelTransformed->GetOutputPort());
    solidMapper->Update();
    vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
    edges->SetInputConnection(modelTransformed->GetOutputPort());
    edges->Update();
    wireframeMapper->SetInputConnection(edges->GetOutputPort());
    wireframeMapper->Update();
    actor->SetMapper(solidMapper);
}

//#########################################################################
ModelInstance::~ModelInstance() {}

//#########################################################################
int ModelInstance::numInstances() {
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
void ModelInstance::setSolid() {
    actor->SetMapper(solidMapper);
}

//#########################################################################
void ModelInstance::setWireFrame() {
    actor->SetMapper(wireframeMapper);
}

//#########################################################################
PQP_CollideResult *ModelInstance::collide(SketchObject *other, int pqp_flags) {
    if (other->numInstances() != 1 || other->getModel() == NULL) {
        return other->collide(this);
    } else {
        PQP_CollideResult *cr = new PQP_CollideResult();
        PQP_REAL r1[3][3], r2[3][3], t1[3], t2[3];
        getPosition(t1);
        getOrientation(r1);
        other->getPosition(t2);
        other->getOrientation(r2);
        PQP_Collide(cr,r1,t1,model->getCollisionModel(),r2,t2,other->getModel()->getCollisionModel(),pqp_flags);
        return cr;
    }
}

//#########################################################################
void ModelInstance::getAABoundingBox(double bb[]) {
    modelTransformed->GetOutput()->GetBounds(bb);
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
ObjectGroup::ObjectGroup() : SketchObject(), children() {}

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
int ObjectGroup::numInstances() {
    int sum = 0;
    for (int i = 0; i < children.length(); i++) {
        int num = children[i]->numInstances();
        sum += (num <= 0) ? num : -1 * num;
    }
    return sum;
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
const QList<SketchObject *> *ObjectGroup::getSubObjects() const {
    return &children;
}

//#########################################################################
void ObjectGroup::setWireFrame() {
    for (int i = 0; i < children.size(); i++) {
        children[i]->setWireFrame();
    }
}

//#########################################################################
void ObjectGroup::setSolid() {
    for (int i = 0; i < children.size(); i++) {
        children[i]->setSolid();
    }
}

//#########################################################################
PQP_CollideResult *ObjectGroup::collide(SketchObject *other, int pqp_flags) {
    // todo
    return NULL;
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
} // todo

//#########################################################################
void ObjectGroup::localTransformUpdated() {
    for (int i = 0; i < children.size(); i++) {
        SketchObject::localTransformUpdated(children[i]);
    }
}

