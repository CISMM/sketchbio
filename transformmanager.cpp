#include "transformmanager.h"
#include "sketchioconstants.h"


TransformManager::TransformManager() {
    worldToRoom = vtkSmartPointer<vtkTransform>::New();
    worldToRoom->Identity();
    worldToRoomScale = 1;
    roomToWorld = worldToRoom->GetLinearInverse();
    roomToEyes = vtkSmartPointer<vtkTransform>::New();
    worldEyeTransform = vtkSmartPointer<vtkTransform>::New();
    worldEyeTransform->Identity();
    worldEyeTransform->PostMultiply();
    worldEyeTransform->Concatenate(worldToRoom);
    worldEyeTransform->Concatenate(roomToEyes);
    roomToEyes->Identity();
    roomToTrackerBase = vtkSmartPointer<vtkTransform>::New();
    roomToTrackerBase->Translate(0,0,0); // TBD
    roomToTrackerBase->Scale(TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE,
                             TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE,
                             TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE);
    trackerBaseToRoom = roomToTrackerBase->GetLinearInverse();
    q_type id_q = Q_ID_QUAT;
    q_vec_type null_vec = Q_NULL_VECTOR;
    q_copy(trackerBaseToLeftHand.quat ,id_q);
    q_vec_copy(trackerBaseToLeftHand.xyz ,null_vec);
    q_xyz_quat_copy(&trackerBaseToRightHand,&trackerBaseToLeftHand);
    q_xyz_quat_copy(&trackerBaseToLeftHandOld,&trackerBaseToLeftHand);
    q_xyz_quat_copy(&trackerBaseToRightHandOld,&trackerBaseToLeftHand);
}

vtkTransform * TransformManager::getWorldToEyeTransform() {
    return worldEyeTransform;
}

vtkTransform * TransformManager::getWorldToRoomTransform() {
    return worldToRoom;
}

double TransformManager::getWorldToRoomScale() const {
    return worldToRoomScale;
}

void TransformManager::getLeftTrackerTransformInEyeCoords(vtkTransform *trans) {
    trans->Identity();
    trans->PostMultiply();
    double x, y, z, angle;
    q_to_axis_angle(&x,&y,&z,&angle,trackerBaseToLeftHand.quat);
    trans->RotateWXYZ(angle * 180 / Q_PI,x,y,z);
    trans->Translate(trackerBaseToLeftHand.xyz);
    trans->Concatenate(trackerBaseToRoom);
    trans->Concatenate(roomToEyes);
}

void TransformManager::getLeftTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    trackerBaseToRoom->TransformPoint(trackerBaseToLeftHand.xyz,temp);
    roomToWorld->TransformPoint(temp,dest_vec);
}

void TransformManager::getOldLeftTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    trackerBaseToRoom->TransformPoint(trackerBaseToLeftHandOld.xyz,temp);
    roomToWorld->TransformPoint(temp,dest_vec);
}

void TransformManager::getRightTrackerTransformInEyeCoords(vtkTransform *trans) {
    trans->Identity();
    trans->PostMultiply();
    double x, y, z, angle;
    q_to_axis_angle(&x,&y,&z,&angle,trackerBaseToRightHand.quat);
    trans->RotateWXYZ(angle * 180 / Q_PI,x,y,z);
    trans->Translate(trackerBaseToRightHand.xyz);
    trans->Concatenate(trackerBaseToRoom);
    trans->Concatenate(roomToEyes);
}

void TransformManager::getRightTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    trackerBaseToRoom->TransformPoint(trackerBaseToRightHand.xyz,temp);
    roomToWorld->TransformPoint(temp,dest_vec);
}

void TransformManager::getOldRightTrackerPosInWorldCoords(q_vec_type dest_vec) {
    q_vec_type temp;
    trackerBaseToRoom->TransformPoint(trackerBaseToRightHandOld.xyz,temp);
    roomToWorld->TransformPoint(temp,dest_vec);
}

void TransformManager::getLeftTrackerOrientInWorldCoords(q_type dest) {
    q_type vect; // xyz w=0 gives a 3 space vector to apply transformations to
    double angle;
    q_to_axis_angle(&vect[0], &vect[1], &vect[2],&angle,trackerBaseToLeftHand.quat);
    vect[Q_W] = 0;
    roomToWorld->TransformVector(vect,vect);
    q_from_axis_angle(dest,vect[0],vect[1],vect[2],angle);
}

void TransformManager::getOldLeftTrackerOrientInWorldCoords(q_type dest) {
    q_type vect; // xyz w=0 gives a 3 space vector to apply transformations to
    double angle;
    q_to_axis_angle(&vect[0], &vect[1], &vect[2],&angle,trackerBaseToLeftHandOld.quat);
    vect[Q_W] = 0;
    roomToWorld->TransformVector(vect,vect);
    q_from_axis_angle(dest,vect[0],vect[1],vect[2],angle);
}

void TransformManager::getRightTrackerOrientInWorldCoords(q_type dest) {
    q_type vect; // xyz w=0 gives a 3 space vector to apply transformations to
    double angle;
    q_to_axis_angle(&vect[0], &vect[1], &vect[2],&angle,trackerBaseToRightHand.quat);
    vect[Q_W] = 0;
    roomToWorld->TransformVector(vect,vect);
    q_from_axis_angle(dest,vect[0],vect[1],vect[2],angle);
}

void TransformManager::getOldRightTrackerOrientInWorldCoords(q_type dest) {
    q_type vect; // xyz w=0 gives a 3 space vector to apply transformations to
    double angle;
    q_to_axis_angle(&vect[0], &vect[1], &vect[2],&angle,trackerBaseToRightHandOld.quat);
    vect[Q_W] = 0;
    roomToWorld->TransformVector(vect,vect);
    q_from_axis_angle(dest,vect[0],vect[1],vect[2],angle);
}


void TransformManager::getLeftToRightHandWorldVector(q_vec_type destVec) {
    q_vec_type left, right;
    getLeftTrackerPosInWorldCoords(left);
    getRightTrackerPosInWorldCoords(right);
    q_vec_subtract(destVec,right,left);
}

void TransformManager::getOldLeftToRightHandWorldVector(q_vec_type destVec) {
    q_vec_type left, right;
    getOldLeftTrackerPosInWorldCoords(left);
    getOldRightTrackerPosInWorldCoords(right);
    q_vec_subtract(destVec,right,left);
}

double TransformManager::getWorldDistanceBetweenHands() {
    q_vec_type vec;
    getLeftToRightHandWorldVector(vec);
    return q_vec_magnitude(vec);
}

double TransformManager::getOldWorldDistanceBetweenHands() {
    q_vec_type vec;
    getOldLeftToRightHandWorldVector(vec);
    return q_vec_magnitude(vec);
}

void TransformManager::setLeftHandTransform(const q_xyz_quat_type *xyzQuat) {
    q_xyz_quat_copy(&trackerBaseToLeftHand,xyzQuat);
}

void TransformManager::setRightHandTransform(const q_xyz_quat_type *xyzQuat) {
    q_xyz_quat_copy(&trackerBaseToRightHand,xyzQuat);
}
void TransformManager::getLeftHand(q_xyz_quat_type *xyzQuat) {
    q_xyz_quat_copy(xyzQuat,&trackerBaseToLeftHand);
}

void TransformManager::getRightHand(q_xyz_quat_type *xyzQuat) {
    q_xyz_quat_copy(xyzQuat,&trackerBaseToRightHand);
}

void TransformManager::copyCurrentHandTransformsToOld() {
    q_xyz_quat_copy(&trackerBaseToLeftHandOld,&trackerBaseToLeftHand);
    q_xyz_quat_copy(&trackerBaseToRightHandOld,&trackerBaseToRightHand);
}

void TransformManager::scaleWorldRelativeToRoom(double amount) {
    worldToRoom->Scale(amount,amount,amount);
    worldToRoomScale *= amount;
}

void TransformManager::scaleWithLeftTrackerFixed(double amount) {
    q_vec_type left;
    getLeftTrackerPosInWorldCoords(left);
    translateWorldRelativeToRoom(left);
    scaleWorldRelativeToRoom(amount);
    q_vec_invert(left,left);
    translateWorldRelativeToRoom(left);
}

void TransformManager::scaleWithRightTrackerFixed(double amount) {
    q_vec_type right;
    getRightTrackerPosInWorldCoords(right);
    translateWorldRelativeToRoom(right);
    scaleWorldRelativeToRoom(amount);
    q_vec_invert(right,right);
    translateWorldRelativeToRoom(right);
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
