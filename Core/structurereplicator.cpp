#include "structurereplicator.h"

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkTransform.h>

#include "keyframe.h"
#include "sketchobject.h"
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
    if (obj1->hasKeyframes())
    {
        QMapIterator< double, Keyframe > it(*obj1->getKeyframes());
        while (it.hasNext())
        {
            replicas->addKeyframeForCurrentLocation(it.next().key());
        }
    }
    if (obj2->hasKeyframes())
    {
        QMapIterator< double, Keyframe > it = QMapIterator< double, Keyframe >(*obj2->getKeyframes());
        while (it.hasNext())
        {
            replicas->addKeyframeForCurrentLocation(it.next().key());
        }
    }
    world->removeObject(obj1);
    replicas->addObject(obj1);
    world->removeObject(obj2);
    replicas->addObject(obj2);
    replicas->addObserver(this);
    world->addObject(replicas);
    transform->Update();
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
            SketchObject *next = (numShown % 2 == 0) ?
                        obj1->deepCopy() : obj2->deepCopy();
            replicas->addObject(next);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->PostMultiply();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
            next->setLocalTransformPrecomputed(true);
            next->setPropagateForceToParent(true);
            vtkSmartPointer<vtkActor> actor = next->getActor();
            if (actor.GetPointer() != NULL)
            { // TODO -- really we should be using color maps here...
                // I haven't got around to changing it yet
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
    replicas->addKeyframeForCurrentLocation(time);
}

ObjectGroup *StructureReplicator::getReplicaGroup()
{
    return replicas;
}
