#include "sketchobject.h"
#include <vtkExtractEdges.h>
#include <vtkSphereSource.h>
#include <QDebug>

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
        parent->getModelSpacePointInWorldCoordinates(position,dest);
    } else {
        q_vec_copy(dest,position);
    }
}
//#########################################################################
void SketchObject::getOrientation(q_type dest) const {
    if (parent != NULL) {
        q_type tmp;
        parent->getOrientation(tmp);
        q_mult(dest,orientation,tmp);
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
    if (parent != NULL) {
        localTransform->Concatenate(parent->getLocalTransform());
    }
    localTransform->RotateWXYZ(angle,x,y,z);
    localTransform->Translate(position);
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
        obj = children.at(i);
        delete obj;
        children.replace(i,(SketchObject *)NULL);
    }
    children.clear();
}

//#########################################################################
int ObjectGroup::numInstances() {
    return -1 * children.size();
}

//#########################################################################
void ObjectGroup::addObject(const SketchObject *obj) {
    // todo
}

//#########################################################################
void ObjectGroup::removeObject(const SketchObject *obj) {
    // todo
}

//#########################################################################
const QList<SketchObject *> *ObjectGroup::getSubObjects() const {
    return NULL;
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
void ObjectGroup::getAABoundingBox(double bb[]) {} // todo

//#########################################################################
void ObjectGroup::localTransformUpdated() {
    for (int i = 0; i < children.size(); i++) {
        SketchObject::localTransformUpdated(children[i]);
    }
}


//#########################################################################
//#########################################################################
// Test code
//#########################################################################
//#########################################################################

inline bool q_vec_equals(const q_vec_type a, const q_vec_type b) {
    if (Q_ABS(a[Q_X] - b[Q_X]) > Q_EPSILON) {
        return false;
    }
    if (Q_ABS(a[Q_Y] - b[Q_Y]) > Q_EPSILON) {
        return false;
    }
    if (Q_ABS(a[Q_Z] - b[Q_Z]) > Q_EPSILON) {
        return false;
    }
    return true;
}

inline bool q_equals(const q_type a, const q_type b) {
    if (Q_ABS(a[Q_W] - b[Q_W]) > Q_EPSILON) {
        return false;
    }
    return q_vec_equals(a,b);
}

//#########################################################################
// tests if the SketchObject initialized correctly (note if methods used are
//  overridden, this should not be called, use your own test function to
//  check initial state).  This does not modify the SketchObject, just makes
//  sure that the initial state was set up correctly.
inline int testNewSketchObject(SketchObject *obj) {
    int errors = 0;
    q_vec_type nVec = Q_NULL_VECTOR;
    q_type idQuat = Q_ID_QUAT;
    q_vec_type testVec;
    q_type testQ;
    obj->getPosition(testVec);
    obj->getOrientation(testQ);
    if (!q_vec_equals(nVec,testVec)) {
        errors++;
        qDebug() << "Initial position wrong.";
    }
    if (!q_equals(idQuat,testQ)) {
        errors++;
        qDebug() << "Initial orientation wrong";
    }
    obj->getLastPosition(testVec);
    obj->getLastOrientation(testQ);
    if (!q_vec_equals(nVec,testVec)) {
        errors++;
        qDebug() << "Initial last position wrong";
    }
    if (!q_equals(idQuat,testQ)) {
        errors++;
        qDebug() << "Initial last orientation wrong";
    }
    obj->getForce(testVec);
    if (!q_vec_equals(nVec,testVec)) {
        errors++;
        qDebug() << "Initial state has a force";
    }
    obj->getTorque(testVec);
    if (!q_vec_equals(nVec,testVec)) {
        errors++;
        qDebug() << "Initial state has a torque";
    }
    if (obj->getParent() != NULL) {
        errors++;
        qDebug() << "Initial state has a parent";
    }
    vtkTransform *t = obj->getLocalTransform();
    if (t == NULL) {
        errors++;
        qDebug() << "Local transform not created";
    }
    vtkMatrix4x4 *tm = t->GetMatrix();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int k = tm->GetElement(i,j);
            if (i == j) {
                if (k != 1) {
                    errors++;
                    qDebug() << "Local transform not identity";
                }
            } else if (k != 0) {
                errors++;
                qDebug() << "Local transform not identity";
            }
        }
    }
    // collision group and calculate local transform are likely to be changed
    // in subclasses, so let subclasses handle those tests
    return errors;
}

//#########################################################################
// tests if the ModelInstance initialized correctly (note: if methods used
//  either here or in testNewSketchObject are overridden, this should not be
//  called, use your own test funciton to check initial state).  This does
//  not modify the ModelInstance, just checks to see if the initial state is
//  set up correctly
inline int testNewModelInstance(ModelInstance *obj) {
    int errors = testNewSketchObject(obj);
    if (obj->numInstances() != 1) {
        errors++;
        qDebug() << "How is one instance more than one instance?";
    }
    if (obj->getActor() == NULL) {
        errors++;
        qDebug() << "Actor for object is null!";
    }
    if (obj->getModel() == NULL) {
        errors++;
        qDebug() << "Model for object is null";
    }
    return errors;
}

//#########################################################################
// tests if the behavioral methods of a SketchObject are working.  This includes
// get/set methods.  Assumes a newly constructed SketchObject with no modifications
// made to it yet.
inline int testSketchObjectActions(SketchObject *obj) {
    int errors = 0;
    q_vec_type v1, v2, nVec, oneVec;
    q_vec_set(nVec,0,0,0);
    q_vec_set(oneVec,1,1,1);
    q_vec_set(v1,4004,5.827,Q_PI);
    // test set position
    obj->setPosition(v1);
    obj->getPosition(v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        qDebug() << "Get/set position failed";
    }
    obj->getLastPosition(v2);
    if (!q_vec_equals(v2,nVec)) {
        errors++;
        qDebug() << "Last location updated when position changed";
    }
    obj->getLocalTransform()->GetPosition(v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        qDebug() << "Set position incorrectly updated transform position";
    }
    obj->getLocalTransform()->GetOrientation(v2);
    if (!q_vec_equals(v2,nVec)) {
        errors++;
        qDebug() << "Set position incorrectly updated transform rotation";
    }
    obj->getLocalTransform()->GetScale(v2);
    if (!q_vec_equals(v2,oneVec)) {
        errors++;
        qDebug() << "Set position incorrectly updated transform scale";
    }
    q_type q1, q2, idQ = Q_ID_QUAT;
    q_make(q1,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
    q_normalize(q1,q1);
    // test set orientation
    obj->setOrientation(q1);
    obj->getOrientation(q2);
    if (!q_equals(q1,q2)) {
        errors++;
        qDebug() << "Get/set orientation failed";
    }
    obj->getLastOrientation(q2);
    if (!q_equals(q2,idQ)) {
        errors++;
        qDebug() << "Set orientation affected last orientation";
    }
    obj->getLocalTransform()->GetPosition(v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        qDebug() << "Set orientation incorrectly updated transform position";
    }
    double *wxyz = obj->getLocalTransform()->GetOrientationWXYZ();
    q_from_axis_angle(q2,wxyz[1],wxyz[2],wxyz[3],(Q_PI / 180) * wxyz[0]);
    if (!q_equals(q1,q2)) {
        errors++;
        qDebug() << "Set orientation incorrectly updated transform orientation";
    }
    obj->getLocalTransform()->GetScale(v2);
    if (!q_vec_equals(v2,oneVec)) {
        errors++;
        qDebug() << "Set orientation incorrectly updated transform scale";
    }
    // test save/restore position
    q_vec_type v3 = { 5, 9332.222, .1 * Q_PI};
    q_type q3;
    q_from_axis_angle(q3,0,-1,0,-.45);
    obj->setLastLocation();
    obj->setPosAndOrient(v3,q3);
    obj->getPosition(v2);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Set position/orientation at same time failed for position";
    }
    obj->getOrientation(q2);
    if (!q_equals(q2,q3)) {
        errors++;
        qDebug() << "Set position/orientation at same time failed for orientation";
    }
    obj->getLastPosition(v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        qDebug() << "Set last location failed on position";
    }
    obj->getLastOrientation(q2);
    if (!q_equals(q1,q2)) {
        errors++;
        qDebug() << "Set last location failed on orientation";
    }
    obj->restoreToLastLocation();
    obj->getPosition(v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        qDebug() << "Restore last location failed on position";
    }
    obj->getOrientation(q2);
    if (!q_vec_equals(q1,q2)) {
        errors++;
        qDebug() << "Restore last location failed on orientation";
    }
    // test setPrimaryCollisionGroupNum
    obj->setPrimaryCollisionGroupNum(45);
    if (obj->getPrimaryCollisionGroupNum() != 45) {
        errors++;
        qDebug() << "Set primary collision group num failed";
    }
    if (!obj->isInCollisionGroup(45)) {
        errors++;
        qDebug() << "Is in collision group failed";
    }
    // test modelSpacePointInWorldCoords
    obj->getModelSpacePointInWorldCoordinates(nVec,v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        qDebug() << "Model point in world coords failed on null vector";
    }
    obj->getModelSpacePointInWorldCoordinates(v3,v2);
    q_vec_subtract(v2,v2,v1);
    q_invert(q2,q1);
    q_xform(v2,q2,v2);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Model point in world coords failed on interesting point";
    }
    // test worldVectorInModelSpace
    obj->getWorldVectorInModelSpace(v3,v2);
    q_xform(v2,q1,v2);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "World vector to modelspace failed";
    }
    // test forces methods
    q_vec_type f1 = {3,4,5}, f2 = {7,-2,-6.37};
    obj->addForce(nVec,f1);
    obj->getForce(v2);
    if (!q_vec_equals(v2,f1)) {
        errors++;
        qDebug() << "Applying linear force failed";
    }
    obj->getTorque(v2);
    if (!q_vec_equals(v2,nVec)) {
        errors++;
        qDebug() << "Applying force at center caused torque";
    }
    obj->addForce(v3,f2);
    obj->getForce(v2);
    q_vec_subtract(v2,v2,f2);
    if (!q_vec_equals(v2,f1)) {
        errors++;
        qDebug() << "Combining forces failed.";
    }
    obj->getTorque(v2);
    q_vec_type tmp, t2;
    obj->getWorldVectorInModelSpace(f2,tmp);
    q_vec_cross_product(t2,v3,tmp);
    if (!q_vec_equals(t2,v2)) {
        errors++;
        qDebug() << "Torque calculations failed.";
    }
    obj->clearForces();
    obj->getForce(v2);
    if (!q_vec_equals(v2,nVec)) {
        errors++;
        qDebug() << "Clearing force failed.";
    }
    obj->getTorque(v2);
    if (!q_vec_equals(v2,nVec)) {
        errors++;
        qDebug() << "Clearing torques failed";
    }
    return errors;
}

//#########################################################################
// tests if the behavioral methods of a ModelInstance are working  this includes
// get/set methods.  Assumes a newly constructed ModelInstance with no modifications
// made to it yet.
inline int testModelInstanceActions(ModelInstance *obj) {
    int errors = testSketchObjectActions(obj);
    // test set wireframe / set solid... can only test if mapper changed easily, no testing
    // the vtk pipeline to see if it is actually wireframed
    vtkMapper *m1 = obj->getActor()->GetMapper();
    obj->setWireFrame();
    vtkMapper *m2 = obj->getActor()->GetMapper();
    obj->setSolid();
    vtkMapper *m3 = obj->getActor()->GetMapper();
    if (m2 == m3) {
        errors++;
        qDebug() << "set solid failed";
    } else if (m1 != m3) {
        errors++;
        qDebug() << "initial mapper not solid";
    } else if (m1 == m2) {
        errors++;
        qDebug() << "set wireframe failed";
    }
    // test the bounding box .... assumes a radius 4 sphere
    double bb[6];
    q_vec_type pos;
    obj->getPosition(pos); // position set in getSketchObjectActions
    obj->getAABoundingBox(bb);
    for (int i = 0; i < 3; i++) {
        // due to tessalation, bounding box has some larger fluctuations.
        if (Q_ABS(bb[2*i+1] - bb[2*i] - 8) > .025) {
            errors++;
            qDebug() << "Object bounding box wrong";
        }
        // average of fluctuations has larger fluctuation
        if (Q_ABS((bb[2*i+1] +bb[2*i])/2 - pos[i]) > .05) {
            errors++;
            qDebug() << "Object bounding box position wrong";
        }
    }
    return errors;
}

//#########################################################################
int ModelInstance::testModelInstance() {
    // set up
    vtkSmartPointer<vtkSphereSource> source = vtkSmartPointer<vtkSphereSource>::New();
    source->SetRadius(4);
    source->SetThetaResolution(40);
    source->SetPhiResolution(40);
    source->Update();
    vtkSmartPointer<vtkTransformPolyDataFilter> sourceData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    sourceData->SetInputConnection(source->GetOutputPort());
    sourceData->SetTransform(transform);
    sourceData->Update();
    SketchModel *model = new SketchModel("abc",sourceData,1.0,1.0);
    ModelInstance *obj = new ModelInstance(model);
    int errors = 0;

    // test that a new ModelInstance is right
    errors += testNewModelInstance(obj);
    errors += testModelInstanceActions(obj);
    // maybe
    // testCollisions(obj,obj2);

    // clean up
    delete obj;
    delete model;

    return errors;
}

//#########################################################################
int ObjectGroup::testObjectGroup() {
    // set up
    int errors = 0;
    ObjectGroup *grp = new ObjectGroup();
    errors += testNewSketchObject(grp);
    delete grp;
    return errors;
}
