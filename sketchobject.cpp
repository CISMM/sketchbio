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
// tests if the ObjectGroup was properly initialized by the constructor
inline int testNewObjectGroup() {
    ObjectGroup *obj = new ObjectGroup();
    int errors = testNewSketchObject(obj);
    if (obj->numInstances() != 0) {
        errors++;
        qDebug() << "Group does not have correct number of instances";
    }
    if (obj->getSubObjects() == NULL) {
       errors++;
       qDebug() << "Getting list of sub objects failed";
    } else if (!obj->getSubObjects()->empty()) {
        errors++;
        qDebug() << "Group already contains something.";
    }
    delete obj;
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
inline SketchModel *getSphereModel() {
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
    return new SketchModel("abc",sourceData,1.0,1.0);
}

//#########################################################################
// test the actions performed on the object group work correctly
inline int testObjectGroupActions() {
    ObjectGroup grp, grp2;
    int errors = testSketchObjectActions(&grp2);
    SketchModel *m = getSphereModel();
    ModelInstance a(m), b(m), c(m);
    q_vec_type va = {2,0,0}, vb = {0,4,0}, vc = {0,-1,-5};
    q_vec_type v1 = {0,0,1}, v2, v3;
    q_type q1, q2, q3, qtmp;
    q_from_axis_angle(q1,1,0,0,Q_PI/4);
    q_from_axis_angle(q2,0,1,0,Q_PI/2);
    a.setPosition(va);
    a.setOrientation(q1);
    b.setPosition(vb);
    c.setPosition(vc);
    // test adding an object
    grp.addObject(&a);
    a.getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        qDebug() << "Item in group in wrong position";
    }
    // test group position
    grp.getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        qDebug() << "Group position wrong with one item";
    }
    if (grp.numInstances() != -1) {
        errors++;
        qDebug() << "Group number instances wrong after add";
    }
    if (a.getParent() != &grp) {
        errors++;
        qDebug() << "Parent not set correctly";
    }
    // add another item and test positions
    grp.addObject(&b);
    a.getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        qDebug() << "Item in group in wrong position";
    }
    b.getPosition(v2);
    if (!q_vec_equals(v2,vb)) {
        errors++;
        qDebug() << "Item in group in wrong position";
    }
    // test group position (should be average of item positions)
    grp.getPosition(v2);
    q_vec_add(v3,va,vb);
    q_vec_scale(v3,.5,v3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Group position wrong with one item";
    }
    grp.addObject(&c);
    // test getting number of instances in group
    if (grp.numInstances() != -3) {
        errors++;
        qDebug() << "Group number instances wrong after add";
    }
    // check net positions of group items
    a.getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        qDebug() << "Item in group in wrong position";
    }
    b.getPosition(v2);
    if (!q_vec_equals(v2,vb)) {
        errors++;
        qDebug() << "Item in group in wrong position";
    }
    c.getPosition(v2);
    if (!q_vec_equals(v2,vc)) {
        errors++;
        qDebug() << "Item in group in wrong position";
    }
    // test group position (should be average position of members)
    grp.getPosition(v2);
    q_vec_add(v3,va,vb);
    q_vec_add(v3,v3,vc);
    q_vec_scale(v3,1/3.0,v3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Group position wrong with one item";
    }
    // check position of group members after group translation
    grp.setPosition(v1);
    q_vec_add(v2,v1,vb);
    b.getPosition(v3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Group member didn't move right";
    }
    // check orientation of members after rotation
    grp.setOrientation(q2);
    q_mult(qtmp,q2,q1);
    a.getOrientation(q3);
    if (!q_equals(qtmp,q3)) {
        errors++;
        qDebug() << "Group member didn't rotate right";
    }
    // check positions of group members after rotation
    q_xform(v2,q2,va);
    q_vec_add(v2,v2,v1);
    a.getPosition(v3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Group member not in right position after rotation/movement";
    }
    // check combination of move, rotate, add (group orientation should reset, but net rotation of
    //         objects should not change)
    ModelInstance d(m);
    q_type idQ = Q_ID_QUAT;
    q_vec_type oldCtr;
    grp.getPosition(oldCtr);
    d.setPosition(va);
    a.getPosition(v2);
    a.getOrientation(qtmp);
    grp.addObject(&d);
    a.getPosition(v3);
    a.getOrientation(q3);
    // check net position/orientation of object already in group to ensure it didn't change
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Group member moved when new one added";
    }
    if (!q_equals(qtmp,q3)) {
        errors++;
        qDebug() << "Group member changed orientation when new one added";
    }
    // check position/orientation of new object to see if it changed
    d.getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        qDebug() << "Object changed position when added to group";
    }
    d.getOrientation(qtmp);
    if (!q_equals(qtmp,idQ)) {
        errors++;
        qDebug() << "Object changed orientation when added to group";
    }
    // check resulting position/orientation of group
    grp.getPosition(v2);
    q_vec_scale(v3,3,oldCtr);
    q_vec_add(v3,v3,va);
    q_vec_scale(v3,.25,v3);
    if (!q_vec_equals(v3,v2)) {
        errors++;
        qDebug() << "Group position not set correctly when new object added";
    }
    grp.getOrientation(qtmp);
    if (!q_equals(qtmp,idQ)) {
        errors++;
        qDebug() << "Group orientation set incorrectly after new object added";
    }
    // check remove object (resets group rotation, changes center position)
    grp.setOrientation(q1);
    q_vec_type vbtmp;
    q_type qbtmp;
    a.getPosition(v2);
    a.getOrientation(qtmp);
    b.getPosition(vbtmp);
    b.getOrientation(qbtmp);
    grp.getPosition(oldCtr);
    grp.removeObject(&b);
    // check net position/orientation of object still in group to ensure it didn't change
    a.getPosition(v3);
    a.getOrientation(q3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        qDebug() << "Group member moved when new one added";
    }
    if (!q_equals(qtmp,q3)) {
        errors++;
        qDebug() << "Group member changed orientation when new one added";
    }
    // check net position/orientation of object removed to ensure it didn't change
    b.getPosition(v3);
    b.getOrientation(q3);
    if (!q_vec_equals(v3,vbtmp)) {
        errors++;
        qDebug() << "Object position changed when removed from group";
    }
    if (!q_equals(q3,qbtmp)) {
        errors++;
        qDebug() << "Object orientation changed when removed from group";
    }
    // check group center and orientation after removal
    q_vec_scale(oldCtr,4,oldCtr);
    q_vec_subtract(oldCtr,oldCtr,v3);
    q_vec_scale(oldCtr,3,oldCtr);
    grp.getPosition(v3);
    if (!q_vec_equals(oldCtr,v3)) {
        errors++;
        qDebug() << "Group position wrong after removal.";
    }
    grp.getOrientation(qtmp);
    if (!q_equals(qtmp,idQ)) {
        errors++;
        qDebug() << "Group orientation wrong after removal.";
    }
    delete m;
    return errors;
}

//#########################################################################
int ModelInstance::testModelInstance() {
    // set up
    SketchModel *model = getSphereModel();
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
    int errors = 0;
    errors += testNewObjectGroup();
    if (errors == 0) {
        errors += testObjectGroupActions();
    }
    return errors;
}
