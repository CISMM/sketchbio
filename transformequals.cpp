#include "transformequals.h"

ObjectPair::ObjectPair(SketchObject *first, SketchObject *second) :
    o1(first), o2(second)
{}

TransformEquals::TransformEquals(SketchObject *first, SketchObject *second) :
    ObjectForceObserver(), pairsList(), transform(vtkSmartPointer<vtkTransform>::New()),
    masterPairIdx(-1)
{
    addPair(first,second);
    masterPairIdx = 0;
    transform->Identity();
    transform->PostMultiply();
    transform->Concatenate(first->getLocalTransform()->GetLinearInverse());
    transform->Concatenate(second->getLocalTransform());
}

void TransformEquals::addPair(SketchObject *first, SketchObject *second) {
    ObjectPair o(first,second);
    second->addForceObserver(this);
    if (masterPairIdx >= 0) {
        // TODO - make pairs copy each other
        vtkSmartPointer<vtkTransform> tfrm = second->getLocalTransform();
        tfrm->Identity();
        tfrm->PostMultiply();
        tfrm->Concatenate(first->getLocalTransform());
        tfrm->Concatenate(transform);
        second->setLocalTransformDefiningPosition(true);
        second->setLocalTransformPrecomputed(true);
    }
    pairsList.append(o);
}

void TransformEquals::objectPushed(SketchObject *obj) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o2 == obj) {
            // TODO - make the pushed object the source of the transform
        }
    }
}
