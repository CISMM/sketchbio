#include "transformequals.h"

#include <vtkTransform.h>

#include "sketchobject.h"
#include "groupidgenerator.h"

ObjectPair::ObjectPair(SketchObject *first, SketchObject *second) :
    o1(first), o2(second)
{}
ObjectPair::ObjectPair() :
    o1(NULL), o2(NULL)
{}

// inline helper function, sets object back to using its own transform and internal vectors
inline void setObjectBackToNormal(SketchObject *obj) {
    q_vec_type pos;
    q_type orient;
    obj->getPosition(pos);
    obj->getOrientation(orient);
    obj->setLocalTransformDefiningPosition(false);
    obj->setLocalTransformPrecomputed(false);
    obj->setPosAndOrient(pos,orient); // transform should be updated now
}

TransformEquals::TransformEquals(SketchObject *first, SketchObject *second, GroupIdGenerator *gen) :
    ObjectChangeObserver(), pairsList(), transform(vtkSmartPointer<vtkTransform>::New()),
    transformEqualsGroupId(gen->getNextGroupId()), isMovingBase(false)
{
    addPair(first,second);
    transform->Identity();
    transform->PostMultiply();
    transform->Concatenate(pairsList[0].o2->getLocalTransform());
    transform->Concatenate(pairsList[0].o1->getInverseLocalTransform());
    q_vec_set(posOffset,0,0,0);
    q_make(orientOffset,1,0,0,0);
}

TransformEquals::~TransformEquals() {
    for (int i = 0; i < pairsList.size(); i++) {
        setObjectBackToNormal(pairsList[i].o2);
    }
}

bool TransformEquals::addPair(SketchObject *first, SketchObject *second) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (first == pairsList[i].o1 || first == pairsList[i].o2 ||
                second == pairsList[i].o1 || second == pairsList[i].o2)
            return false;
    }
    ObjectPair o(first,second);
    second->addObserver(this);
    if (pairsList.empty()) {
        first->addObserver(this);
    }
    second->addToCollisionGroup(first->getPrimaryCollisionGroupNum());
    second->setPrimaryCollisionGroupNum(transformEqualsGroupId);
    // make the new objects follow the master pair
    if (!pairsList.empty()) {
        vtkSmartPointer<vtkTransform> tfrm = second->getLocalTransform();
        tfrm->Identity();
        tfrm->PostMultiply();
        tfrm->Concatenate(transform);
        tfrm->Concatenate(first->getLocalTransform());
        second->setLocalTransformDefiningPosition(true);
        second->setLocalTransformPrecomputed(true);
    }
    pairsList.append(o);
    return true;
}

void TransformEquals::removePairAt(int i) {
    // TODO - if the master pair is removed, currently this clears keyframes...
    // may not want this
    setObjectBackToNormal(pairsList[i].o2);
    pairsList[i].o1->removeObserver(this);
    pairsList[i].o2->removeObserver(this);
    pairsList.remove(i);
}

void TransformEquals::removePair(SketchObject *first, SketchObject *second) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o1 == first && pairsList[i].o2 == second) {
            removePairAt(i);
        }
    }
}

void TransformEquals::removePairByFirst(SketchObject *first) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o1 == first) {
            removePairAt(i);
        }
    }
}

void TransformEquals::removePairBySecond(SketchObject *second) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o2 == second) {
            removePairAt(i);
        }
    }
}

const QVector<ObjectPair> *TransformEquals::getPairsList() const {
    return &pairsList;
}

void TransformEquals::objectPushed(SketchObject *obj) {
    if (obj == pairsList[0].o1) {
        isMovingBase = true;
        transform->GetPosition(posOffset);
        double wxyz[4];
        transform->GetOrientationWXYZ(wxyz);
        q_from_axis_angle(orientOffset,wxyz[1],wxyz[2],wxyz[3],wxyz[0] * Q_PI / 180.0);
        return;
    }
    for (int i = 1; i < pairsList.size(); i++) {
        if (pairsList[i].o2 == obj) {
            // convert the force on the pushed object to force on the
            // master pair of objects that will cause the same result
            q_vec_type force, torque;
            pairsList[0].o2->getForce(force);
            pairsList[0].o2->getTorque(torque);
            // get the force and torque added
            q_vec_type newForce, newTorque;
            obj->getForce(newForce);
            obj->getTorque(newTorque);
            // convert them to the new coordinates
            obj->getWorldVectorInModelSpace(newForce,newForce);
            pairsList[0].o2->getModelVectorInWorldSpace(newForce,newForce);
            // add them in
            q_vec_add(force,force,newForce);
            q_vec_add(torque,torque,newTorque);
            pairsList[0].o2->setForceAndTorque(force,torque);
            // clear the force and torque on the 'copy'
            obj->clearForces();
            break;
        }
    }
}

void TransformEquals::objectKeyframed(SketchObject *obj, double time) {
    for (int i = 1; i < pairsList.size(); i++) {
        // add a keyframe for that time to the master pair of objects
        if (pairsList[i].o1 == obj) {
            // remove the keyframe from the object it was given to
            obj->removeKeyframeForTime(time);
            pairsList[0].o1->addKeyframeForCurrentLocation(time);
            break;
        } else if (pairsList[i].o2 == obj) {
            // remove the keyframe from the object it was given to
            obj->removeKeyframeForTime(time);
            pairsList[0].o2->addKeyframeForCurrentLocation(time);
            break;
        }
    }
}

void TransformEquals::objectMoved(SketchObject *obj) {
    if ( isMovingBase && obj == pairsList[0].o1) {
        isMovingBase = false;
        q_vec_type pos;
        q_type orient;
        pairsList[0].o1->getModelSpacePointInWorldCoordinates(posOffset,pos);
        vtkSmartPointer<vtkTransform> tfrm = pairsList[0].o1->getLocalTransform();
        double wxyz[4];
        tfrm->GetOrientationWXYZ(wxyz);
        q_from_axis_angle(orient,wxyz[1],wxyz[2],wxyz[3],wxyz[0] * Q_PI / 180.0);
        q_mult(orient,orient,orientOffset);
        pairsList[0].o2->setPosAndOrient(pos,orient);

    } else if (isMovingBase && obj == pairsList[0].o2) {
        transform->GetPosition(posOffset);
        double wxyz[4];
        transform->GetOrientationWXYZ(wxyz);
        q_from_axis_angle(orientOffset,wxyz[1],wxyz[2],wxyz[3],wxyz[0] * Q_PI / 180.0);
    }
}
