
#include <quat.h>
#include <iostream>
#include <transformmanager.h>
#include <sketchioconstants.h>
#include <sketchtests.h>

#include <test/TestCoreHelpers.h>

/*
 * This method tests the get/setLeftTrackerPosition and methods
 * in combination with translation, rotation, and scaling
 *
 */
int testLeftTrackerPosition() {
    int failures = 0;
    TransformManager mgr = TransformManager();
    q_vec_type vec;
    mgr.translateWorldRelativeToRoom(0,0,-10);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    // the default tracker base offset is <0,0,10> (all other default translations
    // are identity)
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 10) {
        failures++;
        std::cout << "Initial tracker position wrong" << std::endl;
        q_vec_print(vec);
    }
    // this should put the tracker at the origin
    mgr.translateWorldRelativeToRoom(0,0,10);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 0) {
        failures++;
        std::cout << "Tracker position after translation wrong" << std::endl;
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(5,2,0);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    if (vec[0] != -5 || vec[1] != -2 || vec[2] != 0) {
        failures++;
        std::cout << "Tracker position after second translation wrong";
        q_vec_print(vec);
    }
    q_vec_type z_axis = {0.0,0.0,1.0};
    q_type quat, qInv;
    q_from_two_vecs(quat,z_axis,vec);
    q_from_two_vecs(qInv,vec,z_axis);
    mgr.rotateWorldRelativeToRoom(quat);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]*vec[2] - 29) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after rotation wrong";
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(vec);
    mgr.rotateWorldRelativeToRoom(qInv);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after third translation wrong" << std::endl;
        q_vec_print(vec);
    }
    q_type ident = Q_ID_QUAT;
    q_xyz_quat_type tracker;
    q_copy(tracker.quat,ident);
    q_vec_set(tracker.xyz,0,1,0);
    mgr.setHandTransform(&tracker,SketchBioHandId::LEFT);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    if (Q_ABS(vec[0]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*tracker.xyz[0]) > Q_EPSILON
            || Q_ABS(vec[1]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*tracker.xyz[1]) > Q_EPSILON
            || Q_ABS(vec[2]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*tracker.xyz[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    // test in combination with scaling
    mgr.scaleWorldRelativeToRoom(2);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::LEFT);
    if (Q_ABS(vec[0]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE/2*tracker.xyz[0]) > Q_EPSILON
            || Q_ABS(vec[1]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE/2*tracker.xyz[1]) > Q_EPSILON
            || Q_ABS(vec[2]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE/2*tracker.xyz[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    return failures;
}

/*
 * This method tests the get/setRightTrackerPosition and methods
 * in combination with translation, rotation, and scaling
 *
 */
int testRightTrackerPosition() {
    int failures = 0;
    TransformManager mgr = TransformManager();
    q_vec_type vec;
    mgr.translateWorldRelativeToRoom(0,0,-10);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    // the default tracker base offset is <0,0,10> (all other default translations
    // are identity)
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 10) {
        failures++;
        std::cout << "Initial tracker position wrong" << std::endl;
        q_vec_print(vec);
    }
    // this should put the tracker at the origin
    mgr.translateWorldRelativeToRoom(0,0,10);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 0) {
        failures++;
        std::cout << "Tracker position after translation wrong" << std::endl;
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(5,2,0);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    if (vec[0] != -5 || vec[1] != -2 || vec[2] != 0) {
        failures++;
        std::cout << "Tracker position after second translation wrong";
        q_vec_print(vec);
    }
    q_vec_type z_axis = {0.0,0.0,1.0};
    q_type quat, qInv;
    q_from_two_vecs(quat,vec,z_axis);
    q_from_two_vecs(qInv,z_axis,vec);
    mgr.rotateWorldRelativeToRoom(quat);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]*vec[2] - 29) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after rotation wrong";
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(vec);
    mgr.rotateWorldRelativeToRoom(qInv);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after third translation wrong" << std::endl;
        q_vec_print(vec);
    }
    q_type ident = Q_ID_QUAT;
    q_xyz_quat_type tracker;
    q_copy(tracker.quat,ident);
    q_vec_set(tracker.xyz,0,1,0);
    mgr.setHandTransform(&tracker,SketchBioHandId::RIGHT);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    if (Q_ABS(vec[0]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*tracker.xyz[0]) > Q_EPSILON
            || Q_ABS(vec[1]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*tracker.xyz[1]) > Q_EPSILON
            || Q_ABS(vec[2]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*tracker.xyz[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    // test in combination with scaling
    mgr.scaleWorldRelativeToRoom(2);
    mgr.getTrackerPosInWorldCoords(vec,SketchBioHandId::RIGHT);
    if (Q_ABS(vec[0]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE/2*tracker.xyz[0]) > Q_EPSILON
            || Q_ABS(vec[1]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE/2*tracker.xyz[1]) > Q_EPSILON
            || Q_ABS(vec[2]-1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE/2*tracker.xyz[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    return failures;
}

int testRotateAboutLeftTracker() {
    int failures = 0;
    // I can't figure out how to do it if transform manager is in arbitrary
    // configuration and still get a valid test
    TransformManager mgr = TransformManager();

    q_vec_type a = {2,3,-1}, b = {-3,7,5}, pos = {5,-2,7}, pos2;
    q_type ident = Q_ID_QUAT;
    q_vec_normalize(a,a);
    q_vec_normalize(b,b);
    q_vec_add(pos2,pos,a);
//    q_vec_print(a);
//    q_vec_print(b);

    // we need to set up for this test
    // difference between the trackers should initially be the a vector
    // final difference should be the b vector

    mgr.translateWorldRelativeToRoom(0,0,0); // world origin = tracker origin
    mgr.scaleWorldRelativeToRoom(1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE); // world units = tracker units

    // set tracker positions
    q_xyz_quat_type xyz1, xyz2;
    q_vec_copy(xyz1.xyz,pos);
    q_vec_copy(xyz2.xyz,pos2);
    q_copy(xyz1.quat,ident);
    q_copy(xyz2.quat,ident);
    mgr.setHandTransform(&xyz1,SketchBioHandId::LEFT);
    mgr.setHandTransform(&xyz2,SketchBioHandId::RIGHT);


    q_vec_type lpos1, rpos1, difference1;
    mgr.getTrackerPosInWorldCoords(lpos1,SketchBioHandId::LEFT);
    mgr.getTrackerPosInWorldCoords(rpos1,SketchBioHandId::RIGHT);

    q_vec_subtract(difference1,rpos1,lpos1);

    for (int i = 0; i < 3; i++) {
        if (Q_ABS(difference1[i] -a[i]) > Q_EPSILON) {
            failures++;
            std::cout << "initial locaitons wrong" << std::endl;
        }
    }
    if (failures) {
        q_vec_print(lpos1);
        q_vec_print(rpos1);
    }

    // set up quaternions to rotate a to b and vice versa
    q_type q, qinv;
    q_from_two_vecs(q,a,b); // q rotates a to b
    q_from_two_vecs(qinv,b,a); // qinv rotates b to a

    mgr.rotateWorldRelativeToRoomAboutTracker(qinv,SketchBioHandId::LEFT);

    q_vec_type lpos2, rpos2, difference2;
    mgr.getTrackerPosInWorldCoords(lpos2,SketchBioHandId::LEFT);
    mgr.getTrackerPosInWorldCoords(rpos2,SketchBioHandId::RIGHT);

    q_vec_subtract(difference2,rpos2,lpos2);

    // test the resulting difference in positions of left & right trackers
    if (Q_ABS(difference2[0] -b[0]) > Q_EPSILON || Q_ABS(difference2[1] -b[1]) > Q_EPSILON
            || Q_ABS(difference2[2] -b[2]) > Q_EPSILON) {
        failures++;
        std::cout << "rotation wrong" << std::endl;
        q_vec_print(difference1);
        q_vec_print(difference2);
    }

    // make sure the left tracker didn't move
    if (Q_ABS(lpos1[0]-lpos2[0]) > Q_EPSILON || Q_ABS(lpos1[1]-lpos2[1]) > Q_EPSILON ||
            Q_ABS(lpos1[2]-lpos2[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker we rotated around moved!" << std::endl;
        q_vec_print(lpos1);
        q_vec_print(lpos2);
    }

    return failures;
}

int testRotateAboutRightTracker() {
    int failures = 0;
    // I can't figure out how to do it if transform manager is in arbitrary
    // configuration and still get a valid test
    TransformManager mgr = TransformManager();

    q_vec_type a = {2,3,-1}, b = {-3,7,5}, pos = {5,-2,7}, pos2;
    q_type ident = Q_ID_QUAT;
    q_vec_normalize(a,a);
    q_vec_normalize(b,b);
    q_vec_add(pos2,pos,a);
//    q_vec_print(a);
//    q_vec_print(b);

    // we need to set up for this test
    // difference between the trackers should initially be the a vector
    // final difference should be the b vector

    mgr.translateWorldRelativeToRoom(0,0,0); // world origin = tracker origin
    mgr.scaleWorldRelativeToRoom(1.0/TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE); // world units = tracker units

    // set tracker positions
    q_xyz_quat_type xyz1, xyz2;
    q_vec_copy(xyz1.xyz,pos);
    q_vec_copy(xyz2.xyz,pos2);
    q_copy(xyz1.quat,ident);
    q_copy(xyz2.quat,ident);
    mgr.setHandTransform(&xyz1,SketchBioHandId::RIGHT);
    mgr.setHandTransform(&xyz2,SketchBioHandId::LEFT);


    q_vec_type lpos1, rpos1, difference1;
    mgr.getTrackerPosInWorldCoords(lpos1,SketchBioHandId::LEFT);
    mgr.getTrackerPosInWorldCoords(rpos1,SketchBioHandId::RIGHT);

    q_vec_subtract(difference1,lpos1,rpos1);

    for (int i = 0; i < 3; i++) {
        if (Q_ABS(difference1[i] -a[i]) > Q_EPSILON) {
            failures++;
            std::cout << "initial locaitons wrong" << std::endl;
        }
    }

    // set up quaternions to rotate a to b and vice versa
    q_type q, qinv;
    q_from_two_vecs(q,a,b); // q rotates a to b
    q_from_two_vecs(qinv,b,a); // qinv rotates b to a

    // note that rotating the world by qinv gives the left tracker (whose initial difference
    // from the right tracker was a) a difference of b from the other
    mgr.rotateWorldRelativeToRoomAboutTracker(qinv,SketchBioHandId::RIGHT);

    q_vec_type lpos2, rpos2, difference2;
    mgr.getTrackerPosInWorldCoords(lpos2,SketchBioHandId::LEFT);
    mgr.getTrackerPosInWorldCoords(rpos2,SketchBioHandId::RIGHT);

    q_vec_subtract(difference2,lpos2,rpos2);

    // test the resulting difference in positions of left & right trackers
    if (Q_ABS(difference2[0] -b[0]) > Q_EPSILON || Q_ABS(difference2[1] -b[1]) > Q_EPSILON
            || Q_ABS(difference2[2] -b[2]) > Q_EPSILON) {
        failures++;
        std::cout << "rotation wrong" << std::endl;
        q_vec_print(difference1);
        q_vec_print(difference2);
    }

    // make sure the right tracker didn't move
    if (Q_ABS(rpos1[0]-rpos2[0]) > Q_EPSILON || Q_ABS(rpos1[1]-rpos2[1]) > Q_EPSILON ||
            Q_ABS(rpos1[2]-rpos2[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker we rotated around moved!" << std::endl;
        q_vec_print(rpos1);
        q_vec_print(rpos2);
    }
    return failures;
}

int testSetTrackerPosition()
{
    TransformManager mgr = TransformManager();
    q_vec_type pos = {3, Q_PI, 4.23748293748372};
    TestCoreHelpers::setTrackerWorldPosition(mgr,SketchBioHandId::LEFT,pos);
    q_vec_type result;
    mgr.getTrackerPosInWorldCoords(result,SketchBioHandId::LEFT);
    if (! q_vec_equals(pos,result) ) {
        std::cout << "Left hand set position failed." << std::endl;
        return 1;
    }
    pos[0] = 3.48943548946;
    pos[1] = 5.9941314894;
    pos[2] = -6.549451684;
    TestCoreHelpers::setTrackerWorldPosition(mgr,SketchBioHandId::RIGHT,pos);
    mgr.getTrackerPosInWorldCoords(result,SketchBioHandId::RIGHT);
    if (! q_vec_equals(pos,result) ) {
        std::cout << "Right hand set position failed." << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int test = 0;
    if (argc > 1) {
        test = (int) (argv[1][0] - '0');
    }
    int status = 0;
    switch (test) { // originally there were more tests, but then I changed to using vtkTransform
    // and most of them became unnecessary
    case 4:
        std::cout << "Testing rotation about left tracker" << std::endl;
        status += testRotateAboutLeftTracker();
        std::cout << "Testing rotation about right tracker" << std::endl;
        status += testRotateAboutRightTracker();
        break;
    case 6:
        std::cout << "Testing left tracker position" << std::endl;
        status += testLeftTrackerPosition();
        break;
    case 7:
        std::cout << "Testing right tracker position" << std::endl;
        status += testRightTrackerPosition();
        break;
    default:
        std::cout << "Testing tracker position setting for other tests" << std::endl;
        status += testSetTrackerPosition();
        break;
    }
    return status;
}
