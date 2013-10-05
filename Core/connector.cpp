#include "connector.h"

#include "sketchobject.h"

Connector::Connector(SketchObject *o1, SketchObject *o2,
                     const q_vec_type o1Pos, const q_vec_type o2Pos,
                     double a, double rad)
{
    object1 = o1;
    object2 = o2;
    q_vec_copy(object1ConnectionPosition,o1Pos);
    q_vec_copy(object2ConnectionPosition,o2Pos);
    alpha = a;
    radius = rad;
}

Connector::~Connector()
{
}

void Connector::setObject1(SketchObject *obj)
{
    q_vec_type wPos;
    getEnd1WorldPosition(wPos);
    object1 = obj;
    setEnd1WorldPosition(wPos);
}

void Connector::setObject2(SketchObject *obj)
{
    q_vec_type wPos;
    getEnd2WorldPosition(wPos);
    object2 = obj;
    setEnd2WorldPosition(wPos);
}

void Connector::getEnd1WorldPosition(q_vec_type out) const {
    if (object1 != NULL)
        object1->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,out);
    else
        q_vec_copy(out,object1ConnectionPosition);
}

void Connector::setEnd1WorldPosition(const q_vec_type newPos) {
    if (object1 != NULL)
        object1->getWorldSpacePointInModelCoordinates(newPos,object1ConnectionPosition);
    else
        q_vec_copy(object1ConnectionPosition,newPos);
}

void Connector::getEnd2WorldPosition(q_vec_type out) const {
    if (object2 != NULL)
        object2->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,out);
    else
        q_vec_copy(out,object2ConnectionPosition);
}

void Connector::setEnd2WorldPosition(const q_vec_type newPos) {
    if (object2 != NULL)
        object2->getWorldSpacePointInModelCoordinates(newPos,object2ConnectionPosition);
    else
        q_vec_copy(object2ConnectionPosition,newPos);
}
