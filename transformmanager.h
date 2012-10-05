#ifndef TRANSFORMMANAGER_H
#define TRANSFORMMANAGER_H

#include <quat.h>

// my little addition to quatlib
/*
 * Performs the matrix * column vector multiplication between matrix and vector.  If the boolean flag is set,
 * it assumes the vector is a point and appends a 1, otherwise it appends a 0.
 *
 * It is safe to call this method with dest = vec
 */
void qogl_matrix_vec_mult(q_vec_type dest, const qogl_matrix_type matrix, const q_vec_type vec, bool point);

class TransformManager
{
public:

    TransformManager();

    /*
     * Scales the room relative to the world
     */
    void scaleWorldRelativeToRoom(double amount);
    /*
     * Rotates the room relative to the world
     *
     * Uses vrpn/quatlib quaternions for the rotation
     */
    void rotateWorldRelativeToRoom(const q_type quat);
    /*
     * Rotates the world around the left tracker
     */
    void rotateWorldRelativeToRoomAboutLeftTracker(const q_type quat);
    /*
     * Rotates the world around the right tracker
     */
    void rotateWorldRelativeToRoomAboutRightTracker(const q_type quat);
    /*
     * Translates the room relative to the world
     */
    void translateWorld(const q_vec_type vect);
    /*
     * Translates the room relative to the world
     */
    void translateWorld(double x, double y, double z);
    /*
     * Sets the position and orientation of the left hand tracker
     *
     * Uses vrpn/quatlib quaternions for the orientation
     */
    void setLeftHandTransform(const q_vec_type pos,const q_type quat);
    /*
     * Sets the position and orientation of the right hand tracker
     *
     * Uses vrpn/quatlib quaternions for the orientation
     */
    void setRightHandTransform(const q_vec_type pos,const q_type quat);

    /*
     * Gets the matrix describing the transformation from the camera coordinates
     * to the world coordinates
     */
    void getWorldToEyeMatrix(qogl_matrix_type destMatrix);

    /*
     * Gets the matrix describing the transformation from world to room
     */
    void getWorldToRoomMatrix(qogl_matrix_type destMatrix);

    /*
     * Gets the matrix describing the transformation from room to world
     */
    void getRoomToWorldMatrix(qogl_matrix_type destMatrix);

    /*
     * Gets the position of the left tracker transformed to the eye coordinate system
     * along with the orientation of the vector in that same coordinate system
     */
    void getLeftTrackerInEyeCoords(q_xyz_quat_type *dest_xyz_quat);

    /*
     * Gets the position of the right tracker in world coordinates
     */
    void getLeftTrackerPosInWorldCoords(q_vec_type dest_vec);

    /*
     * Gets the position of the right tracker transformed to the eye coordinate system
     * along with the orientation of the vector in that same coordinate system
     */
    void getRightTrackerInEyeCoords(q_xyz_quat_type *dest_xyz_quat);

    /*
     * Gets the position of the left tracker in world coordinates
     */
    void getRightTrackerPosInWorldCoords(q_vec_type dest_vec);

    /*
     * Gets the vector (in room coordinates) of the left hand to the right hand
     */
    void getLeftToRightHandVector(q_vec_type destVec);

    /*
     * Gets the distance (in room coordinates) of the left hand from the right hand
     */
    double getDistanceBetweenHands();

private:

    qogl_matrix_type worldRoomTransform; // world to room...
    qogl_matrix_type roomToWorldTransform; // room to world... no matrix inverse function
    q_vec_type roomToEyes;

    // Having these two separately is simpler than having them in a matrix
    // I define them to be applied in the order add the offset THEN multiply by the scale
    // Noone outside the internals of this class should care about this though
    double roomToTrackerBaseScale;
    q_vec_type roomToTrackerBaseOffset;

    q_xyz_quat_struct trackerBaseToLeftHand;
    q_xyz_quat_struct trackerBaseToRightHand;
};

#endif // TRANSFORMMANAGER_H
