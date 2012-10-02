#include "transformmanager.h"

TransformManager::TransformManager()
{
    qogl_matrix_type  identity = {1.0,0.0,0.0,0.0,
                                  0.0,1.0,0.0,0.0,
                                  0.0,0.0,1.0,0.0,
                                  0.0,0.0,0.0,1.0};
    q_vec_type nullVect = Q_NULL_VECTOR;
    q_type quat_ident = Q_ID_QUAT;
    qogl_matrix_copy(worldRoomTransform,identity);
    q_vec_set(roomToTrackerBaseOffset, 0,0,5);
    roomToTrackerBaseScale = 0.25;
    q_vec_set(roomToEyes,0,0,0); // TODO figure out a good value for this
    q_vec_copy(trackerBaseToLeftHand.xyz, nullVect);
    q_vec_copy(trackerBaseToRightHand.xyz,nullVect);
    q_copy(trackerBaseToLeftHand.quat, quat_ident);
    q_copy(trackerBaseToRightHand.quat,quat_ident);
}

/*
 * Scales the room relative to the world
 */
void TransformManager::scaleWorldRelativeToRoom(double amount) {
    qogl_matrix_type scale = {amount,0.0,   0.0,   0.0,
                              0.0,   amount,0.0,   0.0,
                              0.0,   0.0,   amount,0.0,
                              0.0,   0.0,   0.0,   1.0};
    qogl_matrix_mult(worldRoomTransform,scale,worldRoomTransform);
}

/*
 * Rotates the room relative to the world
 *
 * Uses vrpn/quatlib quaternions for the rotation
 */
void TransformManager::rotateRoom(q_type quat) {
    qogl_matrix_type rotation;
    q_to_ogl_matrix(rotation,quat);
    qogl_matrix_mult(worldRoomTransform,rotation,worldRoomTransform);
}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateRoom(q_vec_type vect) {
    qogl_matrix_type translation;
    q_xyz_quat_type xyzQuat;
    q_type identity_quat = Q_ID_QUAT;
    q_vec_copy(xyzQuat.xyz,vect);
    q_copy(xyzQuat.quat,identity_quat);
    q_xyz_quat_to_ogl_matrix(translation,&xyzQuat);
    qogl_matrix_mult(worldRoomTransform,translation,worldRoomTransform);
}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateRoom(double x, double y, double z) {
    qogl_matrix_type translation;
    q_xyz_quat_type xyzQuat;
    q_type identity_quat = Q_ID_QUAT;
    q_vec_set(xyzQuat.xyz,x,y,z);
    q_copy(xyzQuat.quat,identity_quat);
    q_xyz_quat_to_ogl_matrix(translation,&xyzQuat);
    qogl_matrix_mult(worldRoomTransform,translation,worldRoomTransform);
}
/*
 * Sets the position and orientation of the left hand tracker
 *
 * Uses vrpn/quatlib quaternions for the orientation
 */
void TransformManager::setLeftHandTransform(q_vec_type pos,q_type quat) {
    q_vec_copy(trackerBaseToLeftHand.xyz,pos);
    q_copy(trackerBaseToLeftHand.quat,quat);
}

/*
 * Sets the position and orientation of the right hand tracker
 *
 * Uses vrpn/quatlib quaternions for the orientation
 */
void TransformManager::setRightHandTransform(q_vec_type pos,q_type quat) {
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
