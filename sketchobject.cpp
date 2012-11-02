#include "sketchobject.h"

SketchObject::SketchObject(vtkActor *a, int model, vtkTransform *worldEyeTransform)
{
    actor = a;
    modelId = model;
    q_vec_set(position,0,0,0);
    q_vec_copy(forceAccum,position);
    q_vec_copy(torqueAccum,forceAccum);
    q_make(orientation,1,0,0,0);
    allowTransformUpdate = true;
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


void SketchObject::addForce(q_vec_type point, q_vec_type force) {
    q_vec_type modelOrigin, worldPoint, worldOrigin, worldDifference, torque;
    getModelSpacePointInWorldCoordinates(point,worldPoint);
    q_vec_set(modelOrigin,0,0,0);
    getModelSpacePointInWorldCoordinates(modelOrigin,worldOrigin);
    q_vec_subtract(worldDifference,worldPoint,worldOrigin);
    q_vec_add(forceAccum,forceAccum,force);
    q_vec_cross_product(torque,worldDifference,force);
    q_vec_add(torqueAccum,torque,torqueAccum);
}
