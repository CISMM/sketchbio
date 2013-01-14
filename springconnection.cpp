#include "springconnection.h"
#include <QDebug>

SpringConnection::SpringConnection(ObjectId o1, double minRestLen, double maxRestLen,
                                   double k, const q_vec_type obj1Pos)
{
    object1 = o1;
    minRestLength = minRestLen;
    maxRestLength = maxRestLen;
    stiffness = k;
    q_vec_copy(object1ConnectionPosition,obj1Pos);
    end1 = end2 = cellId = -1;
}

void SpringConnection::getEnd1WorldPosition(q_vec_type out) const {
    (*object1)->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,out);
}


//######################################################################################
//######################################################################################
//######################################################################################

InterObjectSpring::InterObjectSpring(ObjectId o1, ObjectId o2, double minRestLen, double maxRestLen,
                                     double k, const q_vec_type obj1Pos, const q_vec_type obj2Pos):
    SpringConnection(o1,minRestLen,maxRestLen,k,obj1Pos)
{
    object2 = o2;
    q_vec_copy(object2ConnectionPosition, obj2Pos);
}

void InterObjectSpring::addForce() {
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
    if (length < minRestLength) {
        displacement = minRestLength - length;
    } else if (length > maxRestLength) {
        displacement = maxRestLength - length;
    } else {
        displacement = 0;
    }
    q_vec_scale(f2,displacement*stiffness,difference);
    q_vec_scale(f1,-1,f2);
    (*object1)->addForce(object1ConnectionPosition,f1);
    (*object2)->addForce(object2ConnectionPosition,f2);
}

void InterObjectSpring::getEnd2WorldPosition(q_vec_type out) const {
    (*object2)->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,out);
}


//######################################################################################
//######################################################################################
//######################################################################################

ObjectPointSpring::ObjectPointSpring(ObjectId o1, double minRestLen, double maxRestLen, double k,
                                        const q_vec_type obj1Pos, const q_vec_type worldPoint)
    : SpringConnection(o1,minRestLen,maxRestLen,k,obj1Pos)
{
    q_vec_copy(point,worldPoint);
}

void ObjectPointSpring::getEnd2WorldPosition(q_vec_type out) const {
    q_vec_copy(out,point);
}

void ObjectPointSpring::addForce() {
    q_vec_type end1pos, difference, f;
    double length, displacement;

    (*object1)->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,end1pos);
    q_vec_subtract(difference,point,end1pos);
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
    q_vec_scale(f,-displacement*stiffness,difference);
    (*object1)->addForce(object1ConnectionPosition,f);
}
