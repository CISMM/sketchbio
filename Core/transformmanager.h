#ifndef TRANSFORMMANAGER_H
#define TRANSFORMMANAGER_H

#include <quat.h>

#include <vtkSmartPointer.h>

#include "sketchioconstants.h"

class vtkTransform;
class vtkLinearTransform;
class vtkMatrix4x4;
class vtkCamera;
/*
 * This class handles transformations between world space, tracker space, and
 *camera space.
 *
 * There is an idea of having a vehicle or "room" that moves around relative to
 *the world
 * space.  This is that space that the view and trackers are located in.  This
 *class
 * contains transforms that define the relationships between tracker
 *coordinates,
 * room coordinates, view coordinates and world coordinates.
 *
 * This class also keeps track of the vrpn output of tracker
 *position/orientation
 * for the current step and the last step.
 */

class TransformManager
{
 public:
  TransformManager();

  /*
   * Gets the matrix describing the transformation from the world coordinates
   * to the camera coordinates
   */
  vtkTransform *getWorldToEyeTransform();

  /*
   * Gets the transform describing the transformation from world to room
   */
  vtkTransform *getWorldToRoomTransform();

  /*
   * Gets the transformation between world and room
   */
  const vtkMatrix4x4 *getWorldToRoomMatrix() const;

  /*
   * Gets the transformation between room and world
   */
  vtkLinearTransform *getRoomToWorldTransform();

  /*
   * Sets the transformation between world and room
   */
  void setWorldToRoomMatrix(vtkMatrix4x4 *matrix);

  /*
   * Sets the transformation between the tracker coordinates and the room
   * coordinates
   */
  void setTrackerToRoomMatrix(vtkMatrix4x4 *matrix);

  /*
   * Gets the transformation between room and eye
   */
  const vtkMatrix4x4 *getRoomToEyeMatrix() const;

  /*
   * Gets the transformation between room and eye
   */
  vtkTransform *getRoomToEyeTransform();

  /*
   * Sets the transformation between room and eye
   */
  void setRoomToEyeMatrix(vtkMatrix4x4 *matrix);

  /*
   * Sets the orientation of the eye relative to the room
   * x and y are rotations about x and y respectively and should be in degrees
   */
  void setRoomEyeOrientation(double x, double y);
  /*
   * Gets the scale factor between the world and room
   */
  double getWorldToRoomScale() const;

  /*
   * Sets the given transform to the transform needed to put the origin at
   * the tracker's eye coordinates and the orientation equal to the trackers
   */
  void getTrackerTransformInEyeCoords(vtkTransform *trans,
                                      SketchBioHandId::Type side);

  /*
   * Gets the position of the tracker in world coordinates
   */
  void getTrackerPosInWorldCoords(q_vec_type dest_vec,
                                  SketchBioHandId::Type side);

  /*
   * Gets the position of the tracker in room coordinates
   */
  void getTrackerPosInRoomCoords(q_vec_type dest_vec,
                                 SketchBioHandId::Type side);

  /*
   * Gets the former position of the tracker in world coordinates
   */
  void getOldTrackerPosInWorldCoords(q_vec_type dest_vec,
                                     SketchBioHandId::Type side);

  /*
   * Gets tracker orientation in world coordinates
   */
  void getTrackerOrientInWorldCoords(q_type dest_vec,
                                     SketchBioHandId::Type side);

  /*
   * Gets former tracker orientation in world coordinates
   */
  void getOldTrackerOrientInWorldCoords(q_type dest_vec,
                                        SketchBioHandId::Type side);

  /*
   * Gets the vector (in world coordinates) of the tracker given to
   * the other tracker
   */
  void getInterTrackerWorldVector(q_vec_type destVec,
                                  SketchBioHandId::Type side);

  /*
   * Gets the former vector (in world coordinates) of the tracker
   * given to the other tracker
   */
  void getOldInterTrackerWorldVector(q_vec_type destVec,
                                     SketchBioHandId::Type side);

  /*
   * Gets the distance (in room coordinates) of the left hand from the right
   * hand
   */
  double getWorldDistanceBetweenHands();

  /*
   * Gets the former distance (in room coordinates) of the left hand from the
   * right hand
   */
  double getOldWorldDistanceBetweenHands();

  /*
   * Sets the position and orientation of the hand tracker
   *
   */
  void setHandTransform(const q_xyz_quat_type *xyzQuat,
                        SketchBioHandId::Type side);

  /*
   * Gets the position/orientation of the tracker in relation to the base
   */
  void getHandLocation(q_xyz_quat_type *xyzQuat, SketchBioHandId::Type side);

  /*
   * This method should be called to set the current hand transforms as the
   * former hand transforms just before the new ones are set (in case multiple
   * sets occur per frame, this forces correct behavior)
   */
  void copyCurrentHandTransformsToOld();

  /*
   * Scales the room relative to the world
   */
  void scaleWorldRelativeToRoom(double amount);

  /*
   * Scales the room relative to the world while keeping the relative
   * position of the tracker fixed
   */
  void scaleWithTrackerFixed(double amount, SketchBioHandId::Type side);

  /*
   * Rotates the world relative to the room
   *
   */
  void rotateWorldRelativeToRoom(const q_type quat);

  /*
   * Translates world by vect (where vect is in room coordinates)
   */
  void translateWorldRelativeToRoom(const q_vec_type vect);

  /*
   * Translates the room relative to the world
   */
  void translateWorldRelativeToRoom(double x, double y, double z);

  /*
   * Rotates the world around the tracker
   */
  void rotateWorldRelativeToRoomAboutTracker(const q_type quat,
                                             SketchBioHandId::Type side);

  /*
   * Sets the position, focal point, and up vector of the global camera
   */
  void updateCameraForFrame();

  /*
   * Get the global camera.  This is a vtkCamera object whose coordinate
   * space is world coordinates but whose location and orientation are
   * at the origin of view coordinates.
   */
  vtkCamera *getGlobalCamera();

 private:
  vtkSmartPointer< vtkTransform > worldToRoom;
  vtkSmartPointer< vtkLinearTransform > roomToWorld;
  vtkSmartPointer< vtkTransform > roomToEyes;
  vtkSmartPointer< vtkTransform > worldEyeTransform;
  vtkSmartPointer< vtkTransform > roomToTrackerBase;
  vtkSmartPointer< vtkLinearTransform > trackerBaseToRoom;

  vtkSmartPointer< vtkCamera > globalCamera;

  double worldToRoomScale;

  q_xyz_quat_struct trackerBaseToHand[2];
  q_xyz_quat_struct trackerBaseToHandOld[2];
  
  private:
  TransformManager(const TransformManager& other);
  TransformManager &operator=(const TransformManager &other);
};


#endif  // TRANSFORMMANAGER_H
