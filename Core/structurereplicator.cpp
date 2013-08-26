#include "structurereplicator.h"

#include <iostream>

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
    replicaList(),
    world(w),
    transform(vtkSmartPointer< vtkTransform >::New())
{
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
    obj1->addObserver(this);
    obj2->addObserver(this);
    transform->Update();
}

StructureReplicator::StructureReplicator(
        SketchObject *object1, SketchObject *object2, WorldManager *w,
        ObjectGroup *grp, QList<SketchObject *> &replicaLst)
    :
      numShown(replicaLst.size()),
      obj1(object1),
      obj2(object2),
      replicas(grp),
      replicaList(),
      world(w),
      transform(vtkSmartPointer< vtkTransform >::New())
{
    transform->Identity();
    transform->PostMultiply();
    transform->Concatenate(obj1->getInverseLocalTransform());
    transform->Concatenate(obj2->getLocalTransform());
    transform->Update();

    SketchObject *previous = obj2;
    for (QList< SketchObject * >::iterator it = replicaLst.begin();
         it != replicaLst.end(); ++it)
    {
        SketchObject *next = *it;
        if (next->getParent() != replicas)
        {
            ObjectGroup *grp = dynamic_cast< ObjectGroup * >(next->getParent());
            if (grp != NULL)
            {
                grp->removeObject(next);
            }
            else
            {
                world->removeObject(next);
            }
            replicas->addObject(next);
        }
        vtkTransform *lTrans = next->getLocalTransform();
        lTrans->Identity();
        lTrans->PostMultiply();
        lTrans->Concatenate(previous->getLocalTransform());
        lTrans->Concatenate(transform);
        lTrans->Update();
        next->setLocalTransformPrecomputed(true);
        next->setPropagateForceToParent(true);
        previous = next;
        replicaList.append(next);
    }
    obj1->addObserver(this);
    obj2->addObserver(this);
    replicas->addObserver(this);
}

StructureReplicator::~StructureReplicator() {
    setNumShown(0);
}


void StructureReplicator::setNumShown(int num) {
    if (num < 0) {
        num = 0;
    }
    if (num > numShown) {
        SketchObject *previous;
        if (numShown == 0) {
            previous = obj2;
        } else {
            previous = replicaList.last();
        }
        for (; numShown < num; numShown++) {
            SketchObject *next = (numShown % 2 == 0) ?
                        obj1->deepCopy() : obj2->deepCopy();
            replicaList.append(next);
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
            SketchObject *removed = replicaList.last();
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
    return QListIterator<SketchObject *>(replicaList);
}

void StructureReplicator::objectKeyframed(SketchObject *obj, double time)
{
    if (obj != replicas)
    {
        replicas->addKeyframeForCurrentLocation(time);
        if (obj != obj1 && obj != obj2)
        {
            obj1->addKeyframeForCurrentLocation(time);
            obj2->addKeyframeForCurrentLocation(time);
        }
    }
}

void StructureReplicator::subobjectAdded(SketchObject *parent, SketchObject *child)
{
    if (parent == replicas)
    {
        int idx = replicaList.indexOf(child);
        if (idx == -1)
        {
            SketchObject *pt = replicas->getParent();
            ObjectGroup *parent = dynamic_cast< ObjectGroup * >(pt);
            if (parent == NULL)
            {
                // if we don't have a parent, create one to add the new object and
                // the replicas group to
                parent = new ObjectGroup();
                world->removeObject(replicas);
                replicas->removeObject(child);
                parent->addObject(replicas);
                parent->addObject(child);
                world->addObject(parent);
            }
            else
            {
                // if we have a parent of the replicas group, add the child object there
                replicas->removeObject(child);
                parent->addObject(child);
            }
        }
    }
}

void StructureReplicator::subobjectRemoved(SketchObject *parent, SketchObject *child)
{
    if (parent == replicas)
    {
        if (child == obj1)
        {
            obj1 = NULL;
            setNumShown(0);
        }
        if (child == obj2)
        {
            obj2 = NULL;
            setNumShown(0);
        }
        int idx = replicaList.indexOf(child);
        if ( idx != -1 )
        {
            replicaList.removeAt(idx);
            while (replicaList.size() > idx)
            {
                SketchObject *rep = replicaList.last();
                replicaList.pop_back();
                replicas->removeObject(rep);
                delete rep;
            }
        }
    }
}

ObjectGroup *StructureReplicator::getReplicaGroup()
{
    return replicas;
}
