#include "sketchobject.h"

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

int SketchObject::getPrimaryGroupNum() {
   return groupNum;
}

bool SketchObject::isInGroup(int num) {
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
    q_vec_type modelOrigin, modelForce, torque;
    q_vec_set(modelOrigin,0,0,0);
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
