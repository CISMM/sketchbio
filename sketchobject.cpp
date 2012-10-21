#include "sketchobject.h"

SketchObject::SketchObject(vtkActor *a, int model, vtkTransform *worldEyeTransform)
{
    actor = a;
    modelId = model;
    q_vec_set(position,0,0,0);
    q_make(orientation,1,0,0,0);
    updateLocalTransform = true;
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
    if (updateLocalTransform) {
        localTransform->Identity();
        double x,y,z,angle;
        q_to_axis_angle(&x,&y,&z,&angle,orientation);
        localTransform->RotateWXYZ(angle *180/Q_PI,x,y,z);
        localTransform->Translate(position);
        localTransform->Update();
    }
}

void SketchObject::getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const {
    localTransform->TransformPoint(modelPoint,worldCoordsOut);
}
