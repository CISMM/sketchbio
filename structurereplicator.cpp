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
}

/*
 * Gets the updated position from the local transform instead of
 * getting the outdated variable from the parent class
 */
void ReplicatedObject::getPosition(q_vec_type dest) const {
    localTransform->GetPosition(dest);
//    q_vec_type origin = Q_NULL_VECTOR;
//    localTransform->TransformPoint(origin,dest);
}

/*
 * Uses the updated local transform to get the orientation instead of
 * using the outdated variable in the parent class
 */
void ReplicatedObject::getOrientation(q_type dest) const {
    double ori[4];
    localTransform->GetOrientationWXYZ(ori);
    double angle = ori[0] * Q_PI/180.0;
    q_from_axis_angle(dest,ori[1],ori[2],ori[3],angle);
//    q_vec_type xaxis = {1, 0, 0};
//    q_vec_type yaxis = {0, 1, 0};
//    q_vec_type xout,yout;
//    localTransform->TransformVector(xaxis,xout);
//    if (xout[0] == 1 && xout[1] == 0 && xout[2] == 0) {
//        // localtransform shouldn't have scales, so if it is still the x_axis use y instead
//        localTransform->TransformVector(yaxis,yout);
//        q_from_two_vecs(dest,yaxis,yout); // if y is also the same, it is not rotated
//    } else {
//        q_from_two_vecs(dest,xaxis,xout);
//    }
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

void ReplicatedObject::setPrimaryCollisionGroupNum(int num) {} // does nothing, group num based on originals

int ReplicatedObject::getPrimaryCollisionGroupNum() const {
    return ((replicaNum > 0) ? obj1 : obj0)->getPrimaryCollisionGroupNum();
}

bool ReplicatedObject::isInCollisionGroup(int num) const {
    return obj1->isInCollisionGroup(num) || obj0->isInCollisionGroup(num);
}

//############################################################################################
//############################################################################################
// StructureReplicator class methods
//############################################################################################
//############################################################################################

StructureReplicator::StructureReplicator(SketchObject *object1, SketchObject *object2, WorldManager *w,
                                         vtkTransform *worldEyeTransform) :
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
    this->worldEyeTransform = worldEyeTransform;
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
            vtkSmartPointer<vtkActor> actor;
            SketchModel *modelId = previous->getModel();
            actor->SetMapper(modelId->getSolidMapper());
            ReplicatedObject *obj = new ReplicatedObject(modelId,
                                                         obj1,obj2,numShown+2);
            actor = obj->getActor();
            SketchObject *next = world->addObject(obj);
            copies.append(next);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
//            next->setDoPhysics(false);
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
