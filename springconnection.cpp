#include "springconnection.h"
#include <QDebug>

SpringConnection::SpringConnection(ObjectId o1, ObjectId o2, double restLen,
                                   double k, q_vec_type obj1Pos, q_vec_type obj2Pos)
{
    object1 = o1;
    object2 = o2;
    restLength = restLen;
    stiffness = k;
    q_vec_copy(object1ConnectionPosition,obj1Pos);
    q_vec_copy(object2ConnectionPosition,obj2Pos);
    end1 = end2 = cellId = -1;
}

void SpringConnection::addForce() {
    q_vec_type end1pos, end2pos, difference, f1, f2;
    double length, displacement;

    (*object1)->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,end1pos);
    (*object2)->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,end2pos);
    q_vec_subtract(difference,end2pos,end1pos);
    length = q_vec_magnitude(difference);
    if (length < Q_EPSILON) {
        return; // even if we don't have rest length 0, what direction do we push in?
                // just return...
    }
    q_vec_normalize(difference,difference);
    displacement = restLength - length;
    q_vec_scale(f2,displacement*stiffness,difference);
    q_vec_scale(f1,-1,f2);
    (*object1)->addForce(object1ConnectionPosition,f1);
    (*object2)->addForce(object2ConnectionPosition,f2);
}

void SpringConnection::getEnd1WorldPosition(q_vec_type out) const {
    (*object1)->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,out);
}

void SpringConnection::getEnd2WorldPosition(q_vec_type out) const {
    (*object2)->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,out);
}
