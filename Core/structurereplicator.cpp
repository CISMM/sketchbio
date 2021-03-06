#include "structurereplicator.h"

#include <iostream>
#include <cassert>
#include <stdlib.h>
#include <time.h>

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

StructureReplicator::StructureReplicator(SketchObject *object1, SketchObject *object2, WorldManager *w) :
    numShown(0),
    obj1(object1),
    obj2(object2),
    replicas(new ObjectGroup()),
    replicaList(),
    world(w),
    transform(vtkSmartPointer< vtkTransform >::New())
{
	// seed random number generator to generate luminance levels
	srand(time(NULL));

    assert(obj1 != NULL);
    assert(obj2 != NULL);
    // too complicated to figure out how this should work for now, far easier to just make
    // sure they don't have keyframes from when they are outside the replicated structure
    obj1->clearKeyframes();
    obj2->clearKeyframes();
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
    // this constructor should only be used to load a StrucutreReplicator object
    // based on a group that already has that structure.  Thus don't clear out keyframes.
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
    if (replicas != NULL) {
        replicas->removeObserver(this);
    }
    if (obj1 != NULL) {
        obj1->removeObserver(this);
    }
    if (obj2 != NULL) {
        obj2->removeObserver(this);
    }
}


void StructureReplicator::setNumShown(int num) {
	if ((replicas == NULL) || (obj1 == NULL) || (obj2 == NULL)) {
		return;
	}
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
                        obj1->getCopy() : obj2->getCopy();
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

            if (actor.GetPointer() != NULL) {
				int randnum = rand();
				double luminance = ((double(randnum)) / (double(RAND_MAX)));
				next->setLuminance(luminance);
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

void StructureReplicator::objectDeleted(SketchObject *obj)
{
	if ((obj == replicas) || (obj == obj1) || (obj == obj2)) {
        replicas = NULL;
        obj1 = NULL;
        obj2 = NULL;
        std::cout << "Replicator invalid, num shown: " << numShown << std::endl;
		foreach(StructureReplicatorObserver * structRepObserver, structRepObservers)
		{
			structRepObserver->structureReplicatorIsInvalid(this);
		}
		this->deleteLater();
	}
}

void StructureReplicator::objectKeyframed(SketchObject *obj, double time)
{
}

void StructureReplicator::subobjectAdded(SketchObject *parent, SketchObject *child)
{
    if (parent == replicas && child->getParent() == parent)
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
    if (parent == replicas && child->getParent() == parent)
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

void StructureReplicator::addObserver(StructureReplicatorObserver *structRepObserver)
{
	structRepObservers.append(structRepObserver);
}

void StructureReplicator::removeObserver(StructureReplicatorObserver *structRepObserver)
{
	structRepObservers.removeOne(structRepObserver);
	assert(!structRepObservers.contains(structRepObserver));
}

ObjectGroup *StructureReplicator::getReplicaGroup()
{
    return replicas;
}
