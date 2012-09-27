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
    void scaleRoom(double amount);
    /*
     * Rotates the room relative to the world
     *
     * Uses vrpn/quatlib quaternions for the rotation
     */
    void rotateRoom(double quat[4]);
    /*
     * Translates the room relative to the world
     */
    void translateRoom(double vect[3]);
    /*
     * Translates the room relative to the world
     */
    void translateRoom(double x, double y, double z);
    /*
     * Sets the position and orientation of the left hand tracker
     *
     * Uses vrpn/quatlib quaternions for the orientation
     */
    void setLeftHandTransform(double pos[3],double quat[4]);
    /*
     * Sets the position and orientation of the right hand tracker
     *
     * Uses vrpn/quatlib quaternions for the orientation
     */
    void setRightHandTransform(double pos[3],double quat[4]);

    /*
     * Gets the matrix describing the transformation from the camera coordinates
     * to the world coordinates
     */
    void getWorldToEyeMatrix(qogl_matrix_type destMatrix);

    /*
     * Gets the position of the left tracker transformed to the eye coordinate system
     * along with the orientation of the vector in that same coordinate system
     */
    void getLeftTrackerInEyeCoords(q_xyz_quat_type dest_xyz_quat);

    /*
     * Gets the position of the right tracker transformed to the eye coordinate system
     * along with the orientation of the vector in that same coordinate system
     */
    void getRightTrackerInEyeCoords(q_xyz_quat_type dest_xyz_quat);

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
