#include "transformequals.h"

ObjectPair::ObjectPair(SketchObject *first, SketchObject *second) :
    o1(first), o2(second)
{}

TransformEquals::TransformEquals(SketchObject *first, SketchObject *second) :
    ObjectForceObserver(), pairsList(), masterPairIdx(-1)
{
    addPair(first,second);
    masterPairIdx = 0;
}

void TransformEquals::addPair(SketchObject *first, SketchObject *second) {
    ObjectPair o(first,second);
    second->addForceObserver(this);
    if (masterPairIdx > 0) {
        // TODO - make pairs copy each other
    }
    pairsList.append(o);
}

void TransformEquals::objectPushed(SketchObject *obj) {
    for (int i = 0; i < pairsList.size(); i++) {
        if (pairsList[i].o2 == obj) {
            // TODO - make the pushed object the source of the transform
        }
    }
}
