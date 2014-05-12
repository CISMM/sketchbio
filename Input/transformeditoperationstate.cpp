#include <transformeditoperationstate.h>

#include <cassert>

#include <quat.h>

#include <vtkSmartPointer.h>
#include <vtkTransform.h>

#include <sketchobject.h>
#include <sketchproject.h>

const char TransformEditOperationState::SET_TRANSFORMS_OPERATION_FUNCTION[30] = "Set Transforms";

TransformEditOperationState::TransformEditOperationState(SketchBio::Project* p)
    : OperationState(), proj(p) {}

void TransformEditOperationState::addObject(SketchObject* obj) {
    objs.append(obj);
}
QVector<SketchObject*>& TransformEditOperationState::getObjs() {
    return objs;
}

void TransformEditOperationState::setObjectTransform(double x, double y,
                                                     double z, double rX,
                                                     double rY, double rZ)
{
    assert(objs.size() == 2);
    vtkSmartPointer< vtkTransform > transform =
        vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    transform->Concatenate(objs[0]->getLocalTransform());
    transform->Translate(x, y, z);
    transform->RotateZ(rZ);
    transform->RotateX(rX);
    transform->RotateY(rY);
    q_vec_type pos;
    q_type orient;
    SketchObject::getPositionAndOrientationFromTransform(transform, pos,
                                                         orient);
    if (objs[1]->getParent() != NULL) {
        SketchObject::setParentRelativePositionForAbsolutePosition(
            objs[1], objs[1]->getParent(), pos, orient);
    } else {
        objs[1]->setPosAndOrient(pos, orient);
    }
    // clears operation state and calls deleteLater
    cancelOperation();
}

void TransformEditOperationState::cancelOperation()
{
    proj->clearOperationState(SET_TRANSFORMS_OPERATION_FUNCTION);
}

