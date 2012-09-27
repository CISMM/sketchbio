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
    q_vec_copy(roomToTrackerBaseOffset, nullVect);
    roomToTrackerBaseScale = 1.0;
    q_vec_copy(roomToEyes,nullVect);
    q_vec_copy(trackerBaseToLeftHand.xyz, nullVect);
    q_vec_copy(trackerBaseToRightHand.xyz,nullVect);
    q_copy(trackerBaseToLeftHand.quat, quat_ident);
    q_copy(trackerBaseToRightHand.quat,quat_ident);
}

/*
 * Scales the room relative to the world
 */
void TransformManager::scaleRoom(double amount) {
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
void TransformManager::rotateRoom(double quat[4]) {
    qogl_matrix_type rotation;
    q_to_ogl_matrix(rotation,quat);
    qogl_matrix_mult(worldRoomTransform,rotation,worldRoomTransform);
}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateRoom(double vect[3]) {
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
void TransformManager::setLeftHandTransform(double pos[3],double quat[4]) {
    q_vec_copy(trackerBaseToLeftHand.xyz,pos);
    q_copy(trackerBaseToLeftHand.quat,quat);
}

/*
 * Sets the position and orientation of the right hand tracker
 *
 * Uses vrpn/quatlib quaternions for the orientation
 */
void TransformManager::setRightHandTransform(double pos[3],double quat[4]) {
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
void TransformManager::getLeftTrackerInEyeCoords(q_xyz_quat_type dest_xyz_quat) {
    q_vec_type tempVec;
    q_vec_scale(tempVec,1.0/roomToTrackerBaseScale,trackerBaseToLeftHand.xyz);
    q_vec_subtract(tempVec,tempVec,roomToTrackerBaseOffset);
    q_vec_add(dest_xyz_quat.xyz,roomToEyes,tempVec);
    q_copy(dest_xyz_quat.quat,trackerBaseToLeftHand.quat);
}

/*
 * Gets the position of the right tracker transformed to the eye coordinate system
 * along with the orientation of the vector in that same coordinate system
 */
void TransformManager::getRightTrackerInEyeCoords(q_xyz_quat_type dest_xyz_quat) {
    q_vec_type tempVec;
    q_vec_scale(tempVec,1.0/roomToTrackerBaseScale,trackerBaseToRightHand.xyz);
    q_vec_subtract(tempVec,tempVec,roomToTrackerBaseOffset);
    q_vec_add(dest_xyz_quat.xyz,roomToEyes,tempVec);
    q_copy(dest_xyz_quat.quat,trackerBaseToRightHand.quat);
}
