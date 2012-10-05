#include "transformmanager.h"

void qogl_matrix_vec_mult(q_vec_type dest, const qogl_matrix_type matrix, const q_vec_type vec, bool point) {
    double d0 = matrix[0]*vec[0]+matrix[4]*vec[1]
            +matrix[8] *vec[2] + ((point)? matrix[12]: 0);
    double d1 = matrix[1]*vec[0]+matrix[5]*vec[1]
            +matrix[9] *vec[2] + ((point)? matrix[13]: 0);
    double d2 = matrix[2]*vec[0]+matrix[6]*vec[1]
            +matrix[10]*vec[2] + ((point)? matrix[14]: 0);
    dest[0] = d0;
    dest[1] = d1;
    dest[2] = d2;
}

TransformManager::TransformManager()
{
    qogl_matrix_type  identity = {1.0,0.0,0.0,0.0,
                                  0.0,1.0,0.0,0.0,
                                  0.0,0.0,1.0,0.0,
                                  0.0,0.0,0.0,1.0};
    q_vec_type nullVect = Q_NULL_VECTOR;
    q_type quat_ident = Q_ID_QUAT;
    qogl_matrix_copy(worldRoomTransform,identity);
    qogl_matrix_copy(roomToWorldTransform,identity);
    q_vec_set(roomToTrackerBaseOffset, 0,0,10);
    roomToTrackerBaseScale = 0.125;
    q_vec_set(roomToEyes,0,0,0); // TODO figure out a good value for this
    q_vec_copy(trackerBaseToLeftHand.xyz, nullVect);
    q_vec_copy(trackerBaseToRightHand.xyz,nullVect);
    q_copy(trackerBaseToLeftHand.quat, quat_ident);
    q_copy(trackerBaseToRightHand.quat,quat_ident);
}


/*
 * Gets the matrix describing the transformation from world to room
 */
void TransformManager::getWorldToRoomMatrix(qogl_matrix_type destMatrix) {
    qogl_matrix_copy(destMatrix,worldRoomTransform);
}

/*
 * Gets the matrix describing the transformation from room to world
 */
void TransformManager::getRoomToWorldMatrix(qogl_matrix_type destMatrix) {
    qogl_matrix_copy(destMatrix,roomToWorldTransform);
}


/*
 * Scales the room relative to the world
 */
void TransformManager::scaleWorldRelativeToRoom(double amount) {
    double iamount = 1/amount;
    qogl_matrix_type scale = {amount,0.0,   0.0,   0.0,
                              0.0,   amount,0.0,   0.0,
                              0.0,   0.0,   amount,0.0,
                              0.0,   0.0,   0.0,   1.0};
    qogl_matrix_type invScale = {iamount,0.0,0.0,0.0,
                               0.0,iamount,0.0,0.0,
                               0.0,0.0,iamount,0.0,
                               0.0,0.0,0.0,1.0};
    qogl_matrix_mult(worldRoomTransform,worldRoomTransform,scale);
    qogl_matrix_mult(roomToWorldTransform,invScale,roomToWorldTransform);
}

/*
 * Rotates the room relative to the world
 *
 * Uses vrpn/quatlib quaternions for the rotation
 */
void TransformManager::rotateWorldRelativeToRoom(const q_type quat) {
    qogl_matrix_type rotation;
    q_to_ogl_matrix(rotation,quat);


    // find the inverse quaternion
    qogl_matrix_type invRotation;
    q_type inverse;
    double x, y, z, angle;
    q_to_axis_angle(&x, &y, &z, &angle,quat);
    q_from_axis_angle(inverse,-x,-y,-z,angle);
    q_to_ogl_matrix(invRotation,inverse);


    qogl_matrix_mult(worldRoomTransform,worldRoomTransform,rotation);
    qogl_matrix_mult(roomToWorldTransform,invRotation,roomToWorldTransform);
}

void TransformManager::rotateWorldRelativeToRoomAboutLeftTracker(const q_type quat) {
    // translate to the left tracker, rotate, translate back
    q_vec_type lpos;
    getLeftTrackerPosInWorldCoords(lpos);
    translateWorld(lpos);
    rotateWorldRelativeToRoom(quat);
    q_vec_invert(lpos,lpos);
    translateWorld(lpos);
}

void TransformManager::rotateWorldRelativeToRoomAboutRightTracker(const q_type quat) {
    // translate to right tracker, rotate, translate back
    q_vec_type rpos;
    getRightTrackerPosInWorldCoords(rpos);
    translateWorld(rpos);
    rotateWorldRelativeToRoom(quat);
    q_vec_invert(rpos,rpos);
    translateWorld(rpos);
}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateWorld(const q_vec_type vect) {
    qogl_matrix_type translation;
    qogl_matrix_type invTranslation;
    q_xyz_quat_type xyzQuat;
    q_type identity_quat = Q_ID_QUAT;
    q_vec_copy(xyzQuat.xyz,vect);
    q_copy(xyzQuat.quat,identity_quat);
    q_xyz_quat_to_ogl_matrix(translation,&xyzQuat);
    q_vec_invert(xyzQuat.xyz,xyzQuat.xyz);
    q_xyz_quat_to_ogl_matrix(invTranslation,&xyzQuat);
    qogl_matrix_mult(worldRoomTransform,worldRoomTransform,translation);
    qogl_matrix_mult(roomToWorldTransform,invTranslation,roomToWorldTransform);
}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateWorld(double x, double y, double z) {
    q_vec_type vec;
    q_vec_set(vec,x,y,z);
    translateWorld(vec);
}
/*
 * Sets the position and orientation of the left hand tracker
 *
 * Uses vrpn/quatlib quaternions for the orientation
 */
void TransformManager::setLeftHandTransform(const q_vec_type pos,const q_type quat) {
    q_vec_copy(trackerBaseToLeftHand.xyz,pos);
    q_copy(trackerBaseToLeftHand.quat,quat);
}

/*
 * Sets the position and orientation of the right hand tracker
 *
 * Uses vrpn/quatlib quaternions for the orientation
 */
void TransformManager::setRightHandTransform(const q_vec_type pos,const q_type quat) {
    q_vec_copy(trackerBaseToRightHand.xyz,pos);
    q_copy(trackerBaseToRightHand.quat,quat);
}


/*
 * Gets the matrix describing the transformation from the camera coordinates
 * to the world coordinates
 */
void TransformManager::getWorldToEyeMatrix(qogl_matrix_type destMatrix) {
    qogl_matrix_copy(destMatrix,worldRoomTransform);
    qogl_matrix_type translation;
    q_xyz_quat_type xyzQuat;
    q_type identity_quat = Q_ID_QUAT;
    q_vec_copy(xyzQuat.xyz,roomToEyes);
    q_copy(xyzQuat.quat,identity_quat);
    q_xyz_quat_to_ogl_matrix(translation,&xyzQuat);
    qogl_matrix_mult(destMatrix,translation,destMatrix);
}

/*
 * Gets the position of the left tracker transformed to the eye coordinate system
 * along with the orientation of the vector in that same coordinate system
 */
void TransformManager::getLeftTrackerInEyeCoords(q_xyz_quat_type *dest_xyz_quat) {
    q_vec_type tempVec;
    q_xyz_quat_type temp;
    q_xyz_quat_invert(&temp,&trackerBaseToLeftHand);
    q_vec_scale(tempVec,1.0/roomToTrackerBaseScale,temp.xyz);
    q_vec_subtract(tempVec,tempVec,roomToTrackerBaseOffset);
    q_vec_add(dest_xyz_quat->xyz,roomToEyes,tempVec);
    q_normalize(dest_xyz_quat->quat,temp.quat);
}

/*
 * Gets the position of the right tracker in world coordinates
 */
void TransformManager::getLeftTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    // start with the offset from the tracker base
    q_vec_copy(temp,trackerBaseToLeftHand.xyz);
    // scale it back to room coordinates size
    q_vec_scale(temp,1.0/roomToTrackerBaseScale,temp);
    // translate origin back to room origin
    q_vec_add(temp,temp,roomToTrackerBaseOffset);
    // matrix-vector multiply to get in world coordinatess
    qogl_matrix_vec_mult(dest_vec,roomToWorldTransform,temp,true);
}

/*
 * Gets the position of the right tracker transformed to the eye coordinate system
 * along with the orientation of the vector in that same coordinate system
 */
void TransformManager::getRightTrackerInEyeCoords(q_xyz_quat_type *dest_xyz_quat) {
    q_vec_type tempVec;
    q_xyz_quat_type temp;
    q_xyz_quat_invert(&temp,&trackerBaseToRightHand);
    q_vec_scale(tempVec,1.0/roomToTrackerBaseScale,temp.xyz);
    q_vec_subtract(tempVec,tempVec,roomToTrackerBaseOffset);
    q_vec_add(dest_xyz_quat->xyz,roomToEyes,tempVec);
    q_copy(dest_xyz_quat->quat,temp.quat);
}

/*
 * Gets the position of the left tracker in world coordinates
 */
void TransformManager::getRightTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    // start with the offset from the tracker base
    q_vec_copy(temp,trackerBaseToRightHand.xyz);
    // scale it back to room coordinates size
    q_vec_scale(temp,1.0/roomToTrackerBaseScale,temp);
    // translate origin back to room origin
    q_vec_add(temp,temp,roomToTrackerBaseOffset);
    // matrix-vector multiply to get in world coordinatess
    qogl_matrix_vec_mult(dest_vec,roomToWorldTransform,temp,true);
}

/*
 * Gets the vector (in room coordinates) of the left hand to the right hand
 */
void TransformManager::getLeftToRightHandVector(q_vec_type destVec) {
    q_vec_subtract(destVec,trackerBaseToRightHand.xyz,trackerBaseToLeftHand.xyz);
    q_vec_scale(destVec,1/roomToTrackerBaseScale,destVec);
}

/*
 * Gets the distance (in room coordinates) of the left hand from the right hand
 */
double TransformManager::getDistanceBetweenHands() {
    q_vec_type destVec;
    q_vec_subtract(destVec,trackerBaseToRightHand.xyz,trackerBaseToLeftHand.xyz);
    q_vec_scale(destVec,1/roomToTrackerBaseScale,destVec);
    return q_vec_magnitude(destVec);
}
