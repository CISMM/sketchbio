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
    groupNum = -1;
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
