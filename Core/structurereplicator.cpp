#include "structurereplicator.h"

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkTransform.h>

#include "sketchobject.h"
#include "modelinstance.h"
#include "objectgroup.h"
#include "worldmanager.h"

//############################################################################################
//############################################################################################
// StructureReplicator class methods
//############################################################################################
//############################################################################################

#define COLOR_MODIFIER  ( 1 / 2.0 )

StructureReplicator::StructureReplicator(SketchObject *object1, SketchObject *object2, WorldManager *w) :
    numShown(0),
    obj1(object1),
    obj2(object2),
    replicas(new ObjectGroup()),
    world(w)
{
    transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    transform->Concatenate(obj1->getInverseLocalTransform());
    transform->Concatenate(obj2->getLocalTransform());
    world->removeObject(obj1);
    replicas->addObject(obj1);
    world->removeObject(obj2);
    replicas->addObject(obj2);
    replicas->addObserver(this);
    world->addObject(replicas);
}

StructureReplicator::~StructureReplicator() {
    setNumShown(0);
}


void StructureReplicator::setNumShown(int num) {
    if (num < 0) {
        num = 0;
    }
    QList< SketchObject *> *objList = replicas->getSubObjects();
    if (num > numShown) {
        SketchObject *previous;
        if (numShown == 0) {
            previous = obj2;
        } else {
            previous = objList->last();
        }
        for (; numShown < num; numShown++) {
            SketchModel *modelId = previous->getModel();
            ModelInstance *next = new ModelInstance(modelId);
            replicas->addObject(next);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->PostMultiply();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
            vtkSmartPointer<vtkActor> actor = next->getActor();
            if (numShown % 2 == 0) {
                actor->GetProperty()->SetColor(obj1->getActor()->GetProperty()->GetColor());
            } else {
                double color[3];
                obj1->getActor()->GetProperty()->GetColor(color);
                color[0] *= COLOR_MODIFIER;
                color[1] *= COLOR_MODIFIER;
                color[2] *= COLOR_MODIFIER;
                actor->GetProperty()->SetColor(color);
            }
            previous = next;
        }
    } else {
        for (; numShown > num; numShown--) {
            SketchObject *removed = objList->last();
            replicas->removeObject(removed);
            delete removed;
        }
    }
}

int StructureReplicator::getNumShown() const {
    return numShown;
}

void StructureReplicator::updateTransform() {
    transform->Update();
}

QListIterator<SketchObject *> StructureReplicator::getReplicaIterator() const {
    return QListIterator<SketchObject *>(*replicas->getSubObjects());
}

void StructureReplicator::objectKeyframed(SketchObject *obj, double time)
{
    obj1->addKeyframeForCurrentLocation(time);
    obj2->addKeyframeForCurrentLocation(time);
}

ObjectGroup *StructureReplicator::getReplicaGroup()
{
    return replicas;
}
