#include "structurereplicator.h"
#include "sketchobject.h"
#include <QDebug>

StructureReplicator::StructureReplicator(int object1Id, int object2Id, WorldManager *w, TransformManager *trans) {
    id1 = object1Id;
    id2 = object2Id;
    world = w;
    numShown = 0;
    newIds = std::list<int>();
    transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    SketchObject *object1 = world->getObject(id1);
    SketchObject *object2 = world->getObject(id2);
    vtkSmartPointer<vtkTransform> other = object1->getLocalTransform();
    other->Inverse();
    transform->Concatenate(other);
    other->Inverse();
    transform->Concatenate(object2->getLocalTransform());
    transforms = trans;
}


void StructureReplicator::setNumShown(int num) {
    if (num < 0) {
        num = 0;
    }
    if (num > numShown) {
        SketchObject *previous;
        if (numShown == 0) {
            previous = world->getObject(id2);
        } else {
            previous = world->getObject(newIds.back());
        }
        q_vec_type pos = Q_NULL_VECTOR;
        q_type orient = Q_ID_QUAT;
        for (; numShown < num; numShown++) {
            int nextId = world->addObject(previous->getModelId(),pos,orient,transforms->getWorldToEyeTransform());
            newIds.push_back(nextId);
            SketchObject *next = world->getObject(nextId);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
            next->allowLocalTransformUpdates(false);
            previous = next;
        }
    } else {
        for (; numShown > num; numShown--) {
            world->removeObject(newIds.back());
            newIds.pop_back();
        }
    }
}

int StructureReplicator::getNumShown() {
    return numShown;
}
