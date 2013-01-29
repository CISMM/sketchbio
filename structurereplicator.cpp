#include "structurereplicator.h"
#include "sketchobject.h"
#include <QDebug>

//############################################################################################
//############################################################################################
// Replicated Object class
//############################################################################################
//############################################################################################

/*
 * This class is a bit of a cheat, it allows replicated objects to transfer any forces
 * applied to them back to the original objects so that it appears that the force was applied
 * to the replica.  The forces applied to the original objects are scaled based on how many
 * copies away the force is coming from.
 */
class ReplicatedObject : public SketchObject {
public:
    ReplicatedObject(vtkActor *actor, SketchModel *model, vtkTransform *worldEyeTransform,
                     ObjectId original0, ObjectId original1, int num);
    virtual void addForce(q_vec_type point, const q_vec_type force);
    virtual void getPosition(q_vec_type dest) const;
    virtual void getOrientation(q_type dest) const;
private:
    // need original(s) here
    ObjectId id0; // first original
    ObjectId id1; // second original
    int replicaNum; // how far down the chain we are indexed as follows:
                    // -1 is first copy in negative direction
                    // 0 is first original
                    // 1 is second original
                    // 2 is first copy in positive direction
};

ReplicatedObject::ReplicatedObject(vtkActor *actor, SketchModel *model, vtkTransform *worldEyeTransform,
                                   ObjectId original0, ObjectId original1, int num):
    SketchObject(actor, model, worldEyeTransform)
{
    id0 = original0;
    id1 = original1;
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
    SketchObject *original = (replicaNum > 0) ? *id1 : *id0;
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

StructureReplicator::StructureReplicator(ObjectId object1Id, ObjectId object2Id, WorldManager *w,
                                         vtkTransform *worldEyeTransform) {
    id1 = object1Id;
    id2 = object2Id;
    (*id1)->recalculateLocalTransform();
    (*id2)->recalculateLocalTransform();
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
    this->worldEyeTransform = worldEyeTransform;
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
        for (; numShown < num; numShown++) {
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            SketchModel *modelId = previous->getModelId();
            actor->SetMapper(modelId->getSolidMapper());
            ReplicatedObject *obj = new ReplicatedObject(actor,modelId,
                                                         worldEyeTransform,
                                                         id1,id2,numShown+2);
            ObjectId nextId = world->addObject(obj);
            newIds.push_back(nextId);
            SketchObject *next = (*nextId);
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
