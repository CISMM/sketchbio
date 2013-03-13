#include "transformequals.h"

ObjectPair::ObjectPair(SketchObject *first, SketchObject *second) :
    o1(first), o2(second)
{}

TransformEquals::TransformEquals(SketchObject *first, SketchObject *second) :
    ObjectForceObserver(), pairsList(), transform(vtkSmartPointer<vtkTransform>::New()),
    masterPairIdx(-1), mode(TransformEquals::POSITION_COPIES)
{
    addPair(first,second);
    masterPairIdx = 0;
    setupTransform(0);
}

bool TransformEquals::addPair(SketchObject *first, SketchObject *second) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (first == pairsList[i].o1 || first == pairsList[i].o2 ||
                second == pairsList[i].o1 || second == pairsList[i].o2)
            return false;
    }
    ObjectPair o(first,second);
    first->addForceObserver(this);
    second->addForceObserver(this);
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

void TransformEquals::removePairAt(int i) {
    setObjectBackToNormal(pairsList[i].o2);
    pairsList.removeAt(i);
}

void TransformEquals::removePair(SketchObject *first, SketchObject *second) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o1 == first && pairsList[i].o2 == second) {
            setObjectBackToNormal(pairsList[i].o2);
            pairsList.removeAt(i);
        }
    }
}

void TransformEquals::removePairByFirst(SketchObject *first) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o1 == first) {
            setObjectBackToNormal(pairsList[i].o2);
            pairsList.removeAt(i);
        }
    }
}

void TransformEquals::removePairBySecond(SketchObject *second) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o2 == second) {
            setObjectBackToNormal(pairsList[i].o2);
            pairsList.removeAt(i);
        }
    }
}

const QList<ObjectPair> *TransformEquals::getPairsList() const {
    return &pairsList;
}

void TransformEquals::objectPushed(SketchObject *obj) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o2 == obj) {
            // make the pushed object the source of the transform
            if (i != masterPairIdx || mode == POSITION_COPIES) {
                mode = EDIT_TRANSFORM;
                setupTransform(i);
            }
            break;
        } else if (pairsList[i].o1 == obj) {
            if (mode != POSITION_COPIES) {
                mode = POSITION_COPIES;
                setupTransform(i);
            }
            break;
        }
    }
}

void TransformEquals::setupTransform(int newMaster) {
    // make the new master defined in terms of its own position instead of the old master
    setObjectBackToNormal(pairsList[newMaster].o2);
    // redefine the transform in terms of the new master pair
    transform->Identity();
    transform->PostMultiply();
    transform->Concatenate(pairsList[newMaster].o2->getLocalTransform());
    transform->Concatenate(pairsList[newMaster].o1->getLocalTransform()->GetLinearInverse());
    if ( mode == POSITION_COPIES) {
        // set the matrix as fixed in the transform
        vtkMatrix4x4 *mat = transform->GetMatrix();
        transform->Identity();
        transform->SetMatrix(mat);
        // reset the new master to be defined in terms of the matrix
        vtkSmartPointer<vtkTransform> tfrm = pairsList[newMaster].o2->getLocalTransform();
        tfrm->Identity();
        tfrm->PostMultiply();
        tfrm->Concatenate(transform);
        tfrm->Concatenate(pairsList[newMaster].o1->getLocalTransform());
        pairsList[newMaster].o2->setLocalTransformDefiningPosition(true);
        pairsList[newMaster].o2->setLocalTransformPrecomputed(true);
    }
    if (masterPairIdx != -1) {
        vtkSmartPointer<vtkTransform> tfrm = pairsList[masterPairIdx].o2->getLocalTransform();
        tfrm->Identity();
        tfrm->PostMultiply();
        tfrm->Concatenate(transform);
        tfrm->Concatenate(pairsList[masterPairIdx].o1->getLocalTransform());
        pairsList[masterPairIdx].o2->setLocalTransformDefiningPosition(true);
        pairsList[masterPairIdx].o2->setLocalTransformPrecomputed(true);
    }
    if (mode == EDIT_TRANSFORM) {
        // swap which pair is master
        masterPairIdx = newMaster;
    } else {
        masterPairIdx = -1;
    }
}
