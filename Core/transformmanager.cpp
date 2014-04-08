#include "transformmanager.h"

#include <vtkCamera.h>
#include <vtkTransform.h>
#include <vtkLinearTransform.h>

#include "sketchioconstants.h"

static inline void q_xyz_quat_copy(q_xyz_quat_struct *dest,
                                   const q_xyz_quat_struct *src)
{
    *dest = *src;
}

TransformManager::TransformManager()
{
  worldToRoom = vtkSmartPointer< vtkTransform >::New();
  worldToRoom->Identity();
  worldToRoomScale = 1;
  roomToWorld = worldToRoom->GetLinearInverse();
  roomToEyes = vtkSmartPointer< vtkTransform >::New();
  roomToEyes->Identity();
  roomToEyes->RotateWXYZ(180, 0, 1, 0);
  roomToEyes->Translate(0, 0, -STARTING_CAMERA_POSITION);
  worldEyeTransform = vtkSmartPointer< vtkTransform >::New();
  worldEyeTransform->Identity();
  worldEyeTransform->PostMultiply();
  worldEyeTransform->Concatenate(worldToRoom);
  worldEyeTransform->Concatenate(roomToEyes);
  roomToTrackerBase = vtkSmartPointer< vtkTransform >::New();
  roomToTrackerBase->Translate(0, 0, 0);  // TBD
  roomToTrackerBase->RotateWXYZ(90.0, 1.0, 0.0, 0.0);
  roomToTrackerBase->Scale(TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE,
                           TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE,
                           TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE);
  trackerBaseToRoom = roomToTrackerBase->GetLinearInverse();
  q_type id_q = Q_ID_QUAT;
  q_vec_type null_vec = Q_NULL_VECTOR;
  q_copy(trackerBaseToHand[0].quat, id_q);
  q_vec_copy(trackerBaseToHand[0].xyz, null_vec);
  q_xyz_quat_copy(&trackerBaseToHand[1], &trackerBaseToHand[0]);
  q_xyz_quat_copy(&trackerBaseToHandOld[0], &trackerBaseToHand[0]);
  q_xyz_quat_copy(&trackerBaseToHandOld[1], &trackerBaseToHand[0]);
  globalCamera = vtkSmartPointer< vtkCamera >::New();
  globalCamera->SetClippingRange(100, 3000);
}

vtkTransform *TransformManager::getWorldToEyeTransform()
{
  return worldEyeTransform;
}

vtkTransform *TransformManager::getWorldToRoomTransform()
{
  return worldToRoom;
}

const vtkMatrix4x4 *TransformManager::getWorldToRoomMatrix() const
{
  return worldToRoom->GetMatrix();
}

vtkLinearTransform *TransformManager::getRoomToWorldTransform()
{
  return roomToWorld;
}

void TransformManager::setWorldToRoomMatrix(vtkMatrix4x4 *matrix)
{
  worldToRoom->Identity();
  worldToRoom->SetMatrix(matrix);
}

void TransformManager::setTrackerToRoomMatrix(vtkMatrix4x4 *matrix)
{
    vtkSmartPointer< vtkMatrix4x4 > inverse =
            vtkSmartPointer< vtkMatrix4x4 >::New();
    vtkMatrix4x4::Invert(matrix,inverse);
    roomToTrackerBase->Identity();
    roomToTrackerBase->SetMatrix(inverse);
}

const vtkMatrix4x4 *TransformManager::getRoomToEyeMatrix() const
{
  return roomToEyes->GetMatrix();
}

vtkTransform *TransformManager::getRoomToEyeTransform() { return roomToEyes; }

void TransformManager::setRoomToEyeMatrix(vtkMatrix4x4 *matrix)
{
  roomToEyes->Identity();
  roomToEyes->SetMatrix(matrix);
}

void TransformManager::setRoomEyeOrientation(double x, double y)
{
  roomToEyes->Identity();
  roomToEyes->RotateWXYZ(180, 0, 1, 0);
  roomToEyes->Translate(0, 0, -STARTING_CAMERA_POSITION);
  roomToEyes->RotateX(x);
  roomToEyes->RotateY(y);
}

double TransformManager::getWorldToRoomScale() const
{
  return worldToRoomScale;
}

void TransformManager::getTrackerTransformInEyeCoords(
    vtkTransform *trans, SketchBioHandId::Type side)
{
  trans->Identity();
  trans->PostMultiply();
  double x, y, z, angle;
  q_to_axis_angle(&x, &y, &z, &angle, trackerBaseToHand[side].quat);
  trans->RotateWXYZ(angle * 180 / Q_PI, x, y, z);
  trans->Translate(trackerBaseToHand[side].xyz);
  trans->Concatenate(trackerBaseToRoom);
  trans->Concatenate(roomToWorld);
}

void TransformManager::getTrackerPosInWorldCoords(q_vec_type dest_vec,
                                                  SketchBioHandId::Type side)
{
  q_vec_type temp;
  trackerBaseToRoom->TransformPoint(trackerBaseToHand[side].xyz, temp);
  roomToWorld->TransformPoint(temp, dest_vec);
}

void TransformManager::getTrackerPosInRoomCoords(q_vec_type dest_vec,
                                                 SketchBioHandId::Type side)
{
  trackerBaseToRoom->TransformPoint(trackerBaseToHand[side].xyz, dest_vec);
}

void TransformManager::getOldTrackerPosInWorldCoords(q_vec_type dest_vec,
                                                     SketchBioHandId::Type side)
{
  q_vec_type temp;
  trackerBaseToRoom->TransformPoint(trackerBaseToHandOld[side].xyz, temp);
  roomToWorld->TransformPoint(temp, dest_vec);
}

void TransformManager::getTrackerOrientInWorldCoords(q_type dest,
                                                     SketchBioHandId::Type side)
{
  q_type vect;  // xyz w=0 gives a 3 space vector to apply transformations to
  double angle;
  q_to_axis_angle(&vect[0], &vect[1], &vect[2], &angle,
                  trackerBaseToHand[side].quat);
  vect[Q_W] = 0;
  roomToWorld->TransformVector(vect, vect);
  q_from_axis_angle(dest, vect[0], vect[1], vect[2], angle);
}

void TransformManager::getOldTrackerOrientInWorldCoords(
    q_type dest, SketchBioHandId::Type side)
{
  q_type vect;  // xyz w=0 gives a 3 space vector to apply transformations to
  double angle;
  q_to_axis_angle(&vect[0], &vect[1], &vect[2], &angle,
                  trackerBaseToHandOld[side].quat);
  vect[Q_W] = 0;
  roomToWorld->TransformVector(vect, vect);
  q_from_axis_angle(dest, vect[0], vect[1], vect[2], angle);
}

void TransformManager::getInterTrackerWorldVector(q_vec_type destVec,
                                                  SketchBioHandId::Type side)
{
  q_vec_type first, second;
  getTrackerPosInWorldCoords(first, side);
  getTrackerPosInWorldCoords(second, (side == SketchBioHandId::LEFT)
                                         ? SketchBioHandId::RIGHT
                                         : SketchBioHandId::LEFT);
  q_vec_subtract(destVec, second, first);
}

void TransformManager::getOldInterTrackerWorldVector(q_vec_type destVec,
                                                     SketchBioHandId::Type side)
{
  q_vec_type first, second;
  getOldTrackerPosInWorldCoords(first, side);
  getOldTrackerPosInWorldCoords(second, (side == SketchBioHandId::LEFT)
                                            ? SketchBioHandId::RIGHT
                                            : SketchBioHandId::LEFT);
  q_vec_subtract(destVec, second, first);
}

double TransformManager::getWorldDistanceBetweenHands()
{
  q_vec_type vec;
  getInterTrackerWorldVector(vec, SketchBioHandId::LEFT);
  return q_vec_magnitude(vec);
}

double TransformManager::getOldWorldDistanceBetweenHands()
{
  q_vec_type vec;
  getOldInterTrackerWorldVector(vec, SketchBioHandId::LEFT);
  return q_vec_magnitude(vec);
}

void TransformManager::setHandTransform(const q_xyz_quat_type *xyzQuat,
                                        SketchBioHandId::Type side)
{
  q_xyz_quat_copy(&trackerBaseToHand[side], xyzQuat);
}

void TransformManager::getHandLocation(q_xyz_quat_type *xyzQuat,
                                       SketchBioHandId::Type side)
{
  q_xyz_quat_copy(xyzQuat, &trackerBaseToHand[side]);
}

void TransformManager::copyCurrentHandTransformsToOld()
{
  q_xyz_quat_copy(&trackerBaseToHandOld[0], &trackerBaseToHand[0]);
  q_xyz_quat_copy(&trackerBaseToHandOld[1], &trackerBaseToHand[1]);
}

void TransformManager::scaleWorldRelativeToRoom(double amount)
{
  worldToRoom->Scale(amount, amount, amount);
  worldToRoomScale *= amount;
}

void TransformManager::scaleWithTrackerFixed(double amount,
                                             SketchBioHandId::Type side)
{
  q_vec_type pos;
  getTrackerPosInWorldCoords(pos, side);
  translateWorldRelativeToRoom(pos);
  scaleWorldRelativeToRoom(amount);
  q_vec_invert(pos, pos);
  translateWorldRelativeToRoom(pos);
}

void TransformManager::rotateWorldRelativeToRoom(const q_type quat)
{
  double xyz[3], angle;
  q_to_axis_angle(&xyz[0], &xyz[1], &xyz[2], &angle, quat);
  worldToRoom->RotateWXYZ(angle * 180 / Q_PI, xyz);
}

void TransformManager::translateWorldRelativeToRoom(const q_vec_type vect)
{
  worldToRoom->Translate(vect);
}

void TransformManager::translateWorldRelativeToRoom(double x, double y,
                                                    double z)
{
  worldToRoom->Translate(x, y, z);
}

void TransformManager::rotateWorldRelativeToRoomAboutTracker(
    const q_type quat, SketchBioHandId::Type side)
{
  q_vec_type pos;
  // get left tracker position in world space
  getTrackerPosInWorldCoords(pos, side);
  translateWorldRelativeToRoom(pos);
  rotateWorldRelativeToRoom(quat);
  q_vec_invert(pos, pos);
  translateWorldRelativeToRoom(pos);
}

vtkCamera *TransformManager::getGlobalCamera() { return globalCamera; }

// this updates the camera based on the current world to eye matrix using the
// algorithm described here:
// http://vtk.1045678.n5.nabble.com/Question-on-manual-configuration-of-VTK-camera-td5059478.html
void TransformManager::updateCameraForFrame()
{
  // get the inverse of the world to eye matrix
  vtkLinearTransform *inv = worldEyeTransform->GetLinearInverse();
  vtkMatrix4x4 *invMat = inv->GetMatrix();
  q_vec_type up, forward, pos, fPoint;
  double scale;
  // the first column is the 'right' vector ... unused
  // the second column of that matrix is the up vector
  up[0] = invMat->GetElement(0, 1);
  up[1] = invMat->GetElement(1, 1);
  up[2] = invMat->GetElement(2, 1);
  // note that the matrix may have a scale, take it into account
  scale = q_vec_magnitude(up);
  q_vec_normalize(up, up);
  // the third column is the front vector
  forward[0] = invMat->GetElement(0, 2);
  forward[1] = invMat->GetElement(1, 2);
  forward[2] = invMat->GetElement(2, 2);
  q_vec_normalize(forward, forward);
  // the fourth column of the matrix is the position of the camera as a point
  pos[0] = invMat->GetElement(0, 3);
  pos[1] = invMat->GetElement(1, 3);
  pos[2] = invMat->GetElement(2, 3);
  // compute the focal point from the position and the front vector
  q_vec_scale(fPoint, STARTING_CAMERA_POSITION * scale, forward);
  q_vec_add(fPoint, fPoint, pos);
  // set the camera parameters
  globalCamera->SetPosition(pos);
  globalCamera->SetFocalPoint(fPoint);
  globalCamera->SetViewUp(up);
  // to keep things from going outside the focal planes so much (may need
  // fine-tuning)
  globalCamera->SetClippingRange(scale * SCALE_DOWN_FACTOR * 100,
                                 (scale + 40) * STARTING_CAMERA_POSITION);
}
