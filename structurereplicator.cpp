#include "structurereplicator.h"
#include "sketchobject.h"
#include <QDebug>

StructureReplicator::StructureReplicator(ObjectId object1Id, ObjectId object2Id, WorldManager *w) {
    id1 = object1Id;
    id2 = object2Id;
    world = w;
    numShown = 0;
    newIds = std::list<ObjectId>();
    transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    SketchObject *object1 = (*id1);
    SketchObject *object2 = (*id2);
    vtkSmartPointer<vtkTransform> other = object1->getLocalTransform();
    transform->Concatenate(other->GetLinearInverse());
    transform->Concatenate(object2->getLocalTransform());
}


void StructureReplicator::setNumShown(int num) {
    if (num < 0) {
        num = 0;
    }
    if (num > numShown) {
        SketchObject *previous;
        if (numShown == 0) {
            previous = (*id2);
        } else {
            previous = (*newIds.back());
        }
        q_vec_type pos = Q_NULL_VECTOR;
        q_type orient = Q_ID_QUAT;
        for (; numShown < num; numShown++) {
            ObjectId nextId = world->addObject(previous->getModelId(),pos,orient);
            newIds.push_back(nextId);
            SketchObject *next = (*nextId);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
            next->allowLocalTransformUpdates(false);
            next->setDoPhysics(false);
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

void StructureReplicator::updateTransform() {
    transform->Update();
}
