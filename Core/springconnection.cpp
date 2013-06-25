#include "springconnection.h"

#include "sketchobject.h"

SpringConnection::SpringConnection(SketchObject *o1, SketchObject *o2, double minRestLen,
                                   double maxRestLen, double k, const q_vec_type obj1Pos,
                                   const q_vec_type obj2Pos) :
    object1(o1),
    object2(o2),
    minRestLength(minRestLen),
    maxRestLength(maxRestLen),
    stiffness(k)
    #ifdef SHOW_DEBUGGING_FORCE_LINES
  ,
    end1(-1),
    end2(-1),
    cellId(-1)
  #endif
{
    q_vec_copy(object1ConnectionPosition,obj1Pos);
    q_vec_copy(object2ConnectionPosition,obj2Pos);
}

void SpringConnection::setObject1(SketchObject *obj)
{
    q_vec_type wPos;
    getEnd1WorldPosition(wPos);
    object1 = obj;
    setEnd1WorldPosition(wPos);
}

void SpringConnection::setObject2(SketchObject *obj)
{
    q_vec_type wPos;
    getEnd2WorldPosition(wPos);
    object2 = obj;
    setEnd2WorldPosition(wPos);
}

void SpringConnection::getEnd1WorldPosition(q_vec_type out) const {
    if (object1 != NULL)
        object1->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,out);
    else
        q_vec_copy(out,object1ConnectionPosition);
}

void SpringConnection::setEnd1WorldPosition(const q_vec_type newPos) {
    if (object1 != NULL)
        object1->getWorldSpacePointInModelCoordinates(newPos,object1ConnectionPosition);
    else
        q_vec_copy(object1ConnectionPosition,newPos);
}

void SpringConnection::getEnd2WorldPosition(q_vec_type out) const {
    if (object2 != NULL)
        object2->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,out);
    else
        q_vec_copy(out,object2ConnectionPosition);
}

void SpringConnection::setEnd2WorldPosition(const q_vec_type newPos) {
    if (object2 != NULL)
        object2->getWorldSpacePointInModelCoordinates(newPos,object2ConnectionPosition);
    else
        q_vec_copy(object2ConnectionPosition,newPos);
}

void SpringConnection::addForce() {
    if (object1 == object2)
        return; // No point if both ends connected to same object
    q_vec_type end1pos, end2pos, difference, f1, f2;
    double length, displacement;

    if (object1 != NULL)
        object1->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,end1pos);
    else
        q_vec_copy(end1pos,object1ConnectionPosition);
    if (object2 != NULL)
        object2->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,end2pos);
    else
        q_vec_copy(end2pos,object2ConnectionPosition);
    q_vec_subtract(difference,end2pos,end1pos);
    length = q_vec_magnitude(difference);
    if (length < Q_EPSILON) {
        return; // even if we don't have rest length 0, what direction do we push in?
                // just return...
    }
    q_vec_normalize(difference,difference);
    if (length < minRestLength) {
        displacement = minRestLength - length;
    } else if (length > maxRestLength) {
        displacement = maxRestLength - length;
    } else {
        displacement = 0;
    }
    q_vec_scale(f2,displacement*stiffness,difference);
    q_vec_scale(f1,-1,f2);
    if (object1 != NULL)
        object1->addForce(object1ConnectionPosition,f1);
    if (object2 != NULL)
        object2->addForce(object2ConnectionPosition,f2);
}

SpringConnection *SpringConnection::makeSpring(SketchObject *o1, SketchObject *o2, const q_vec_type pos1,
                   const q_vec_type pos2, bool worldRelativePos, double k, double minLen, double maxLen) {
    q_type newPos1, newPos2;
    if (worldRelativePos) {
        if (o1 != NULL)
            o1->getWorldSpacePointInModelCoordinates(pos1,newPos1);
        else
            q_vec_copy(newPos1,pos1);
        if (o2 != NULL)
            o2->getWorldSpacePointInModelCoordinates(pos2,newPos2);
        else
            q_vec_copy(newPos2,pos2);
    } else {
        q_vec_copy(newPos1,pos1);
        q_vec_copy(newPos2,pos2);
    }
    SpringConnection *spring = new SpringConnection(o1,o2,minLen,maxLen,k,newPos1,newPos2);
    return spring;
}
