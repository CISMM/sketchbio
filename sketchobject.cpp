#include "sketchobject.h"
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
SketchObject *SketchObject::getParent() {
    return parent;
}

//#########################################################################
void SketchObject::setParent(SketchObject *p) {
    parent = p;
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
    q_vec_copy(dest,position);
}
//#########################################################################
void SketchObject::getOrientation(q_type dest) const {
    q_copy(dest,orientation);
}
//#########################################################################
void SketchObject::getOrientation(PQP_REAL matrix[3][3]) const {
    quatToPQPMatrix(orientation,matrix);
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
    return primaryCollisionGroup;
}

//#########################################################################
void SketchObject::setPrimaryCollisionGroupNum(int num) {
    primaryCollisionGroup = num;
}

//#########################################################################
bool SketchObject::isInCollisionGroup(int num) const {
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
    q_vec_type tmp;
    getWorldVectorInModelSpace(force,tmp);
    q_vec_cross_product(tmp,point,tmp);
    q_vec_add(torqueAccum,torqueAccum,tmp);
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
    localTransform->Identity();
    localTransform->RotateWXYZ((180/Q_PI)*angle,x,y,z);
    localTransform->Translate(position);
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
/*
SketchObject::SketchObject(vtkActor *a, SketchModel *model, vtkTransform *worldEyeTransform)
{
    actor = a;
    modelId = model;
    q_vec_set(position,0,0,0);
    q_vec_copy(lastPosition,position);
    q_vec_copy(forceAccum,position);
    q_vec_copy(torqueAccum,forceAccum);
    q_make(orientation,1,0,0,0);
    q_copy(lastOrientation,orientation);
    groupNum = OBJECT_HAS_NO_GROUP;
    vtkSmartPointer<vtkTransform> trans = (vtkTransform *) a->GetUserTransform();
    if (trans == NULL) {
        trans = vtkSmartPointer<vtkTransform>::New();
    }
    trans->Identity();
    trans->PostMultiply();
    localTransform = vtkSmartPointer<vtkTransform>::New();
    localTransform->Identity();
    localTransform->PostMultiply();
    trans->Concatenate(localTransform);
    trans->Concatenate(worldEyeTransform);
    a->SetUserTransform(trans);
    physicsEnabled = true;
    allowTransformUpdate = true;
}

void SketchObject::getPosition(q_vec_type dest) const {
    q_vec_copy(dest,position);
}

void SketchObject::getOrientation(q_type dest) const {
    q_copy(dest,orientation);
}

void SketchObject::setPrimaryGroupNum(int num) {
    groupNum = num;
}

int SketchObject::getPrimaryGroupNum() const {
   return groupNum;
}

bool SketchObject::isInGroup(int num) const {
    return num == groupNum;
}

void SketchObject::recalculateLocalTransform() {
    if (allowTransformUpdate) {
        localTransform->Identity();
        double x,y,z,angle;
        q_to_axis_angle(&x,&y,&z,&angle,orientation);
        localTransform->RotateWXYZ(angle *180/Q_PI,x,y,z);
        localTransform->Translate(position);
        localTransform->Update();
    } else {
        localTransform->Update();
    }
}

void SketchObject::getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const {
    localTransform->TransformPoint(modelPoint,worldCoordsOut);
}


void SketchObject::addForce(q_vec_type point, const q_vec_type force) {
    q_vec_type modelForce, torque;
    q_vec_add(forceAccum,forceAccum,force);
    localTransform->Inverse();
    localTransform->TransformVector(force,modelForce);
    localTransform->Inverse();
    q_vec_cross_product(torque,point,modelForce);
    q_vec_add(torqueAccum,torque,torqueAccum);
}

void SketchObject::collide(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqpFlags) {
    // get the collision models:
    SketchModel *model1 = ((o1)->getModel());
    SketchModel *model2 = ((o2)->getModel());
    PQP_Model *pqp_model1 = model1->getCollisionModel();
    PQP_Model *pqp_model2 = model2->getCollisionModel();

    // get the offsets and rotations in PQP's format
    PQP_REAL r1[3][3], t1[3], r2[3][3],t2[3];
    q_type quat1, quat2;
    (o1)->getOrientation(quat1);
    quatToPQPMatrix(quat1,r1);
    (o1)->getPosition(t1);
    (o2)->getOrientation(quat2);
    quatToPQPMatrix(quat2,r2);
    (o2)->getPosition(t2);

    // collide them
    PQP_Collide(cr,r1,t1,pqp_model1,r2,t2,pqp_model2,pqpFlags);
}
*/
