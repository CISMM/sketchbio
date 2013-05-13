#include "structurereplicator.h"
#include "sketchobject.h"
#include <QDebug>

//############################################################################################
//############################################################################################
// Replicated Object class
//############################################################################################
//############################################################################################

ReplicatedObject::ReplicatedObject(SketchModel *model, SketchObject *original0, SketchObject *original1,
                                   int num):
    ModelInstance(model)
{
    obj0 = original0;
    obj1 = original1;
    replicaNum = num;
    setLocalTransformPrecomputed(true);
    setLocalTransformDefiningPosition(true);
    setPrimaryCollisionGroupNum((replicaNum > 0 ? obj1 : obj0)->getPrimaryCollisionGroupNum());
    addToCollisionGroup((replicaNum > 0 ? obj0 : obj1)->getPrimaryCollisionGroupNum());
}

/*
 * Translates the force to the parent's orientation and applies a fraction of it,
 * based on how far down the chain this object is
 */
void ReplicatedObject::addForce(q_vec_type point, const q_vec_type force) {
    double divisor = (replicaNum > 0) ? replicaNum : (-replicaNum +1);
    SketchObject *original = (replicaNum > 0) ? obj1 : obj0;
    q_vec_type scaledForce;
    getWorldVectorInModelSpace(force,scaledForce);
    q_vec_scale(scaledForce, 1/divisor, scaledForce);
    original->getLocalTransform()->TransformVector(scaledForce,scaledForce);
    original->addForce(point,scaledForce);
}

void ReplicatedObject::addKeyframeForCurrentLocation(double t) {
    obj0->addKeyframeForCurrentLocation(t);
    obj1->addKeyframeForCurrentLocation(t);
}

//############################################################################################
//############################################################################################
// StructureReplicator class methods
//############################################################################################
//############################################################################################

StructureReplicator::StructureReplicator(SketchObject *object1, SketchObject *object2, WorldManager *w) :
    numShown(0),
    obj1(object1),
    obj2(object2),
    world(w),
    copies()
{
    transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    vtkSmartPointer<vtkTransform> other = obj1->getLocalTransform();
    transform->Concatenate(other->GetLinearInverse());
    transform->Concatenate(obj2->getLocalTransform());
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
            previous = copies.at(copies.size()-1);
        }
        for (; numShown < num; numShown++) {
            SketchModel *modelId = previous->getModel();
            ReplicatedObject *obj = new ReplicatedObject(modelId,
                                                         obj1,obj2,numShown+2);
            SketchObject *next = world->addObject(obj);
            copies.append(next);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->PostMultiply();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
            previous = next;
        }
    } else {
        for (; numShown > num; numShown--) {
            world->removeObject(copies.at(copies.size()-1));
            copies.removeLast();
        }
    }
}

int StructureReplicator::getNumShown() const {
    return numShown;
}

void StructureReplicator::updateTransform() {
    transform->Update();
}
