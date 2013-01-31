#include "structurereplicator.h"
#include "sketchobject.h"
#include <QDebug>

//############################################################################################
//############################################################################################
// Replicated Object class
//############################################################################################
//############################################################################################

ReplicatedObject::ReplicatedObject(vtkActor *actor, SketchModel *model, vtkTransform *worldEyeTransform,
                                   SketchObject *original0, SketchObject *original1, int num):
    SketchObject(actor, model, worldEyeTransform)
{
    obj0 = original0;
    obj1 = original1;
    replicaNum = num;
    allowTransformUpdate = false;
}

/*
 * Gets the updated position from the local transform instead of
 * getting the outdated variable from the parent class
 */
void ReplicatedObject::getPosition(q_vec_type dest) const {
    q_vec_type origin = Q_NULL_VECTOR;
    localTransform->TransformPoint(origin,dest);
}

/*
 * Uses the updated local transform to get the orientation instead of
 * using the outdated variable in the parent class
 */
void ReplicatedObject::getOrientation(q_type dest) const {
    q_vec_type xaxis = {1, 0, 0};
    q_vec_type yaxis = {0, 1, 0};
    q_vec_type xout,yout;
    localTransform->TransformVector(xaxis,xout);
    if (xout[0] == 1 && xout[1] == 0 && xout[2] == 0) {
        // localtransform shouldn't have scales, so if it is still the x_axis use y instead
        localTransform->TransformVector(yaxis,yout);
        q_from_two_vecs(dest,yaxis,yout); // if y is also the same, it is not rotated
    } else {
        q_from_two_vecs(dest,xaxis,xout);
    }
}

/*
 * Translates the force to the parent's orientation and applies a fraction of it,
 * based on how far down the chain this object is
 */
void ReplicatedObject::addForce(q_vec_type point, const q_vec_type force) {
    double divisor = (replicaNum > 0) ? replicaNum : (-replicaNum +1);
    SketchObject *original = (replicaNum > 0) ? obj1 : obj0;
    q_vec_type scaledForce;
    localTransform->Inverse();
    localTransform->TransformVector(force,scaledForce);
    localTransform->Inverse();
    q_vec_scale(scaledForce, 1/divisor, scaledForce);
    original->getLocalTransform()->TransformVector(scaledForce,scaledForce);
    original->addForce(point,scaledForce);
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
    obj1->recalculateLocalTransform();
    obj2->recalculateLocalTransform();
    transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    vtkSmartPointer<vtkTransform> other = obj1->getLocalTransform();
    transform->Concatenate(other->GetLinearInverse());
    transform->Concatenate(obj2->getLocalTransform());
    this->worldEyeTransform = worldEyeTransform;
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
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            SketchModel *modelId = previous->getModel();
            actor->SetMapper(modelId->getSolidMapper());
            ReplicatedObject *obj = new ReplicatedObject(actor,modelId,
                                                         worldEyeTransform,
                                                         obj1,obj2,numShown+2);
            SketchObject *next = world->addObject(obj);
            copies.append(next);
            vtkSmartPointer<vtkTransform> tform = next->getLocalTransform();
            tform->Identity();
            tform->Concatenate(previous->getLocalTransform());
            tform->Concatenate(transform);
            next->allowLocalTransformUpdates(false);
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

int StructureReplicator::getNumShown() {
    return numShown;
}

void StructureReplicator::updateTransform() {
    transform->Update();
}
