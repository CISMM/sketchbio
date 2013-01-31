#ifndef TRANSFORMMANAGER_H
#define TRANSFORMMANAGER_H

#include <quat.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkLinearTransform.h>

/*
 * This class handles transformations between world space, tracker space, and camera space.
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
     * Gets the transformation between room and eye
     */
    const vtkMatrix4x4 *getRoomToEyeMatrix() const;

    /*
     * Gets the scale factor between the world and room
     */
    double getWorldToRoomScale() const;

    /*
     * Sets the given transform to the transform needed to put the origin at
     * the right tracker's eye coordinates and the orientation equal to the trackers
     */
    void getLeftTrackerTransformInEyeCoords(vtkTransform *trans);

    /*
     * Gets the position of the right tracker in world coordinates
     */
    void getLeftTrackerPosInWorldCoords(q_vec_type dest_vec);

    /*
     * Gets the former position of the right tracker in world coordinates
     */
    void getOldLeftTrackerPosInWorldCoords(q_vec_type dest_vec);

    /*
     * Sets the given transform to the transform needed to put the origin at
     * the right tracker's eye coordinates and the orientation equal to the trackers
     */
    void getRightTrackerTransformInEyeCoords(vtkTransform *trans);

    /*
     * Gets the position of the left tracker in world coordinates
     */
    void getRightTrackerPosInWorldCoords(q_vec_type dest_vec);

    /*
     * Gets the former position of the left tracker in world coordinates
     */
    void getOldRightTrackerPosInWorldCoords(q_vec_type dest_vec);

    /*
     * Gets left tracker orientation in world coordinates
     */
    void getLeftTrackerOrientInWorldCoords(q_type dest_vec);

    /*
     * Gets former left tracker orientation in world coordinates
     */
    void getOldLeftTrackerOrientInWorldCoords(q_type dest_vec);

    /*
     * Gets right tracker orientation in world coordinates
     */
    void getRightTrackerOrientInWorldCoords(q_type dest_vec);

    /*
     * Gets former right tracker orientation in world coordinates
     */
    void getOldRightTrackerOrientInWorldCoords(q_type dest_vec);

    /*
     * Gets the vector (in world coordinates) of the left hand to the right hand
     */
    void getLeftToRightHandWorldVector(q_vec_type destVec);

    /*
     * Gets the former vector (in world coordinates) of the left hand to the right hand
     */
    void getOldLeftToRightHandWorldVector(q_vec_type destVec);

    /*
     * Gets the distance (in room coordinates) of the left hand from the right hand
     */
    double getWorldDistanceBetweenHands();

    /*
     * Gets the former distance (in room coordinates) of the left hand from the right hand
     */
    double getOldWorldDistanceBetweenHands();

    /*
     * Sets the position and orientation of the left hand tracker
     *
     */
    void setLeftHandTransform(const q_xyz_quat_type *xyzQuat);

    /*
     * Sets the position and orientation of the right hand tracker
     *
     */
    void setRightHandTransform(const q_xyz_quat_type *xyzQuat);

    /*
     * Gets the position/orientation of the left tracker in relation to the base
     */
    void getLeftHand(q_xyz_quat_type *xyzQuat);

    /*
     * Gets the position/orientation of the right tracker in relation to the base
     */
    void getRightHand(q_xyz_quat_type *xyzQuat);

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
     * position of the left tracker fixed
     */
    void scaleWithLeftTrackerFixed(double amount);

    /*
     * Scales the room relative to the world while keeping the relative
     * position of the left tracker fixed
     */
    void scaleWithRightTrackerFixed(double amount);

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
     * Rotates the world around the left tracker
     */
    void rotateWorldRelativeToRoomAboutLeftTracker(const q_type quat);

    /*
     * Rotates the world around the right tracker
     */
    void rotateWorldRelativeToRoomAboutRightTracker(const q_type quat);

private:

    vtkSmartPointer<vtkTransform> worldToRoom;
    vtkSmartPointer<vtkLinearTransform> roomToWorld;
    vtkSmartPointer<vtkTransform> roomToEyes;
    vtkSmartPointer<vtkTransform> worldEyeTransform;
    vtkSmartPointer<vtkTransform> roomToTrackerBase;
    vtkSmartPointer<vtkLinearTransform> trackerBaseToRoom;

    double worldToRoomScale;

    q_xyz_quat_struct trackerBaseToLeftHand;
    q_xyz_quat_struct trackerBaseToLeftHandOld;
    q_xyz_quat_struct trackerBaseToRightHand;
    q_xyz_quat_struct trackerBaseToRightHandOld;
};

inline void q_xyz_quat_copy(q_xyz_quat_struct *dest, const q_xyz_quat_struct *src) {
    q_copy(dest->quat,src->quat);
    q_vec_copy(dest->xyz,src->xyz);
}

#endif // TRANSFORMMANAGER_H
