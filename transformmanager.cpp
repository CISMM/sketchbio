#include "transformmanager.h"

TransformManager::TransformManager()
{
    qogl_matrix_type  identity = {1.0,0.0,0.0,0.0,
                                  0.0,1.0,0.0,0.0,
                                  0.0,0.0,1.0,0.0,
                                  0.0,0.0,0.0,1.0};
    q_vec_type nullVect = Q_NULL_VECTOR;
    q_type quat_ident = Q_ID_QUAT;
    q_matrix_type identityMatrix = Q_ID_MATRIX;
    qogl_matrix_copy(worldRoomTransform,identity);
    q_matrix_copy(roomToWorldTransform,identityMatrix);
    q_vec_set(roomToTrackerBaseOffset, 0,0,10);
    roomToTrackerBaseScale = 0.125;
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
    double iamount = 1/amount;
    qogl_matrix_type scale = {amount,0.0,   0.0,   0.0,
                              0.0,   amount,0.0,   0.0,
                              0.0,   0.0,   amount,0.0,
                              0.0,   0.0,   0.0,   1.0};
    q_matrix_type invScale = {{iamount,0.0,0.0,0.0},
                              {0.0,iamount,0.0,0.0},
                              {0.0,0.0,iamount,0.0},
                              {0.0,0.0,0.0,1.0}};
    qogl_matrix_mult(worldRoomTransform,scale,worldRoomTransform);
    q_matrix_mult(roomToWorldTransform,roomToWorldTransform,invScale);
}

/*
 * Rotates the room relative to the world
 *
 * Uses vrpn/quatlib quaternions for the rotation
 */
void TransformManager::rotateWorldRelativeToRoom(q_type quat) {
    qogl_matrix_type rotation;
    q_to_ogl_matrix(rotation,quat);


    // find the inverse quaternion
    q_matrix_type invRotation;
    q_type inverse;
    double x, y, z, angle;
    q_to_axis_angle(&x, &y, &z, &angle,quat);
    q_from_axis_angle(inverse,x,y,z,-angle);
    q_to_row_matrix(invRotation,inverse);


    qogl_matrix_mult(worldRoomTransform,rotation,worldRoomTransform);
    q_matrix_mult(roomToWorldTransform,roomToWorldTransform,invRotation);
}

void TransformManager::rotateWorldRelativeToRoomAboutLeftTracker(q_type quat) {
    qogl_matrix_type rotation;
    q_to_ogl_matrix(rotation,quat);

    // find the inverse quaternion
    q_matrix_type invRotation;
    q_type inverse;
    double x, y, z, angle;
    q_to_axis_angle(&x, &y, &z, &angle,quat);
    q_from_axis_angle(inverse,x,y,z,-angle);
    q_normalize(inverse,inverse);
    q_to_row_matrix(invRotation,inverse);

    translateRoom(roomToTrackerBaseOffset);
    scaleWorldRelativeToRoom(roomToTrackerBaseScale);
    translateRoom(trackerBaseToLeftHand.xyz);
    qogl_matrix_mult(worldRoomTransform,rotation,worldRoomTransform);
    q_matrix_mult(roomToWorldTransform,roomToWorldTransform,invRotation);
    q_vec_type invVect;
    q_vec_invert(invVect,trackerBaseToLeftHand.xyz);
    translateRoom(invVect);
    scaleWorldRelativeToRoom(1/roomToTrackerBaseScale);
    q_vec_invert(invVect,roomToTrackerBaseOffset);
    translateRoom(invVect);
}

void TransformManager::rotateWorldRelativeToRoomAboutRightTracker(q_type quat) {

}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateRoom(q_vec_type vect) {
    qogl_matrix_type translation;
    q_matrix_type invTranslation;
    q_xyz_quat_type xyzQuat;
    q_type identity_quat = Q_ID_QUAT;
    q_vec_copy(xyzQuat.xyz,vect);
    q_copy(xyzQuat.quat,identity_quat);
    q_xyz_quat_to_ogl_matrix(translation,&xyzQuat);
    q_vec_invert(xyzQuat.xyz,xyzQuat.xyz);
//    q_xyz_quat_to_row_matrix(invTranslation,xyzQuat);
    qogl_matrix_mult(worldRoomTransform,translation,worldRoomTransform);
//    q_matrix_mult(roomToWorldTransform,roomToWorldTransform,invTranslation);
}

/*
 * Translates the room relative to the world
 */
void TransformManager::translateRoom(double x, double y, double z) {
    q_vec_type vec;
    q_vec_set(vec,x,y,z);
    translateRoom(vec);
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
