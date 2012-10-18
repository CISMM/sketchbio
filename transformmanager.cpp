#include "transformmanager.h"


TransformManager::TransformManager() {
    worldToRoom = vtkSmartPointer<vtkTransform>::New();
    roomToEyes = vtkSmartPointer<vtkTransform>::New();
    roomToEyes->Identity();
    roomToTrackerBase = vtkSmartPointer<vtkTransform>::New();
    roomToTrackerBase->Translate(0,0,0); // TBD
    roomToTrackerBase->Scale(TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE,
                             TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE,
                             TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE);
    q_type id_q = Q_ID_QUAT;
    q_vec_type null_vec = Q_NULL_VECTOR;
    q_copy(trackerBaseToLeftHand.quat ,id_q);
    q_copy(trackerBaseToRightHand.quat,id_q);
    q_vec_copy(trackerBaseToLeftHand.xyz ,null_vec);
    q_vec_copy(trackerBaseToRightHand.xyz,null_vec);
}

void TransformManager::getWorldToEyeTransform(vtkTransform *trans) {
    trans->Identity();
    trans->PostMultiply();
    trans->Concatenate(worldToRoom);
    trans->Concatenate(roomToEyes);
}

vtkTransform * TransformManager::getWorldToRoomTransform() {
    return worldToRoom;
}

void TransformManager::getLeftTrackerTransformInEyeCoords(vtkTransform *trans) {
    trans->Identity();
    trans->PostMultiply();
    double x, y, z, angle;
    q_to_axis_angle(&x,&y,&z,&angle,trackerBaseToLeftHand.quat);
    trans->RotateWXYZ(angle * 180 / Q_PI,x,y,z);
    trans->Translate(trackerBaseToLeftHand.xyz);
    roomToTrackerBase->Inverse();
    trans->Concatenate(roomToTrackerBase);
    roomToTrackerBase->Inverse();
    trans->Concatenate(roomToEyes);
}

void TransformManager::getLeftTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    roomToTrackerBase->Inverse();
    worldToRoom->Inverse();
    roomToTrackerBase->TransformPoint(trackerBaseToLeftHand.xyz,temp);
    worldToRoom->TransformPoint(temp,dest_vec);
    roomToTrackerBase->Inverse();
    worldToRoom->Inverse();
}

void TransformManager::getRightTrackerTransformInEyeCoords(vtkTransform *trans) {
    trans->Identity();
    trans->PostMultiply();
    double x, y, z, angle;
    q_to_axis_angle(&x,&y,&z,&angle,trackerBaseToRightHand.quat);
    trans->RotateWXYZ(angle * 180 / Q_PI,x,y,z);
    trans->Translate(trackerBaseToRightHand.xyz);
    roomToTrackerBase->Inverse();
    trans->Concatenate(roomToTrackerBase);
    roomToTrackerBase->Inverse();
    trans->Concatenate(roomToEyes);
}

void TransformManager::getRightTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    roomToTrackerBase->Inverse();
    worldToRoom->Inverse();
    roomToTrackerBase->TransformPoint(trackerBaseToRightHand.xyz,temp);
    worldToRoom->TransformPoint(temp,dest_vec);
    roomToTrackerBase->Inverse();
    worldToRoom->Inverse();
}

void TransformManager::getLeftTrackerOrientInWorldCoords(q_type dest) {
    q_type vect; // xyz w=0 gives a 3 space vector to apply transformations to
    double angle;
    q_to_axis_angle(&vect[0], &vect[1], &vect[2],&angle,trackerBaseToLeftHand.quat);
    vect[Q_W] = 0;
    worldToRoom->Inverse();
    worldToRoom->TransformVector(vect,vect);
    worldToRoom->Inverse();
    q_from_axis_angle(dest,vect[0],vect[1],vect[2],angle);
}

void TransformManager::getRightTrackerOrientInWorldCoords(q_type dest) {
    q_type vect; // xyz w=0 gives a 3 space vector to apply transformations to
    double angle;
    q_to_axis_angle(&vect[0], &vect[1], &vect[2],&angle,trackerBaseToRightHand.quat);
    vect[Q_W] = 0;
    worldToRoom->Inverse();
    worldToRoom->TransformVector(vect,vect);
    worldToRoom->Inverse();
    q_from_axis_angle(dest,vect[0],vect[1],vect[2],angle);
}


void TransformManager::getLeftToRightHandWorldVector(q_vec_type destVec) {
    q_vec_type left, right;
    getLeftTrackerPosInWorldCoords(left);
    getRightTrackerPosInWorldCoords(right);
    q_vec_subtract(destVec,right,left);
}

double TransformManager::getWorldDistanceBetweenHands() {
    q_vec_type vec;
    getLeftToRightHandWorldVector(vec);
    return q_vec_magnitude(vec);
}

void TransformManager::setLeftHandTransform(const q_xyz_quat_type *xyzQuat) {
    q_copy(trackerBaseToLeftHand.quat,xyzQuat->quat);
    q_vec_copy(trackerBaseToLeftHand.xyz,xyzQuat->xyz);
}

void TransformManager::setRightHandTransform(const q_xyz_quat_type *xyzQuat) {
    q_copy(trackerBaseToRightHand.quat,xyzQuat->quat);
    q_vec_copy(trackerBaseToRightHand.xyz,xyzQuat->xyz);
}
void TransformManager::getLeftHand(q_xyz_quat_type *xyzQuat) {
    q_copy(xyzQuat->quat,trackerBaseToLeftHand.quat);
    q_vec_copy(xyzQuat->xyz,trackerBaseToLeftHand.xyz);
}

void TransformManager::getRightHand(q_xyz_quat_type *xyzQuat) {
    q_copy(xyzQuat->quat,trackerBaseToRightHand.quat);
    q_vec_copy(xyzQuat->xyz,trackerBaseToRightHand.xyz);
}

void TransformManager::scaleWorldRelativeToRoom(double amount) {
    worldToRoom->Scale(amount,amount,amount);
}

void TransformManager::rotateWorldRelativeToRoom(const q_type quat) {
    double xyz[3],angle;
    q_to_axis_angle(&xyz[0],&xyz[1],&xyz[2],&angle,quat);
    worldToRoom->RotateWXYZ(angle *180/Q_PI,xyz);
}

void TransformManager::translateWorldRelativeToRoom(const q_vec_type vect) {
    worldToRoom->Translate(vect);
}

void TransformManager::translateWorldRelativeToRoom(double x, double y, double z) {
    worldToRoom->Translate(x,y,z);
}

void TransformManager::rotateWorldRelativeToRoomAboutLeftTracker(const q_type quat) {
    q_vec_type left;
    // get left tracker position in world space
    getLeftTrackerPosInWorldCoords(left);
    translateWorldRelativeToRoom(left);
    rotateWorldRelativeToRoom(quat);
    q_vec_invert(left,left);
    translateWorldRelativeToRoom(left);
}
void TransformManager::rotateWorldRelativeToRoomAboutRightTracker(const q_type quat) {
    q_vec_type right;
    // get right tracker position in world space
    getRightTrackerPosInWorldCoords(right);
    translateWorldRelativeToRoom(right);
    rotateWorldRelativeToRoom(quat);
    q_vec_invert(right,right);
    translateWorldRelativeToRoom(right);
}
