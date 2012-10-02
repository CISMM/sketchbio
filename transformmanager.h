#ifndef TRANSFORMMANAGER_H
#define TRANSFORMMANAGER_H

#include <quat.h>

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
    void rotateRoom(q_type quat);
    /*
     * Translates the room relative to the world
     */
    void translateRoom(q_vec_type vect);
    /*
     * Translates the room relative to the world
     */
    void translateRoom(double x, double y, double z);
    /*
     * Sets the position and orientation of the left hand tracker
     *
     * Uses vrpn/quatlib quaternions for the orientation
     */
    void setLeftHandTransform(q_vec_type pos,q_type quat);
    /*
     * Sets the position and orientation of the right hand tracker
     *
     * Uses vrpn/quatlib quaternions for the orientation
     */
    void setRightHandTransform(q_vec_type pos,q_type quat);

    /*
     * Gets the matrix describing the transformation from the camera coordinates
     * to the world coordinates
     */
    void getWorldToEyeMatrix(qogl_matrix_type destMatrix);

    /*
     * Gets the position of the left tracker transformed to the eye coordinate system
     * along with the orientation of the vector in that same coordinate system
     */
    void getLeftTrackerInEyeCoords(q_xyz_quat_type *dest_xyz_quat);

    /*
     * Gets the position of the right tracker transformed to the eye coordinate system
     * along with the orientation of the vector in that same coordinate system
     */
    void getRightTrackerInEyeCoords(q_xyz_quat_type *dest_xyz_quat);

    /*
     * Gets the vector (in room coordinates) of the left hand to the right hand
     */
    void getLeftToRightHandVector(q_vec_type destVec);

    /*
     * Gets the distance (in room coordinates) of the left hand from the right hand
     */
    double getDistanceBetweenHands();

private:

    qogl_matrix_type worldRoomTransform; // world to room... maybe store inverses?
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
