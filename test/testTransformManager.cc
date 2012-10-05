
#include <quat.h>
#include <iostream>
#include <transformmanager.h>


/*
 * Tests if the transformation in the transform manager's world-to-room matrix
 * is equivalent to the matrix passed in (assumed to be an old one from before
 * whatever operation was done and then undone).
 */
bool testIsUndone(TransformManager *mgr,qogl_matrix_type oringinalWRTransform) {
    qogl_matrix_type newWRTransform;
    mgr->getWorldToRoomMatrix(newWRTransform);
    for (int i = 0; i < 16; i++) {
        if (Q_ABS(newWRTransform[i] - oringinalWRTransform[i]) > Q_EPSILON) {
            return false;
        }
    }
    return true;
}

/*
 * Tests if the transform manager's world-to-room and room-to-world matrices are
 * still inverses of each other
 */
int testInverses(TransformManager *mgr,qogl_matrix_type afterTransform) {
    int failures = 0;
    // test inverse
    qogl_matrix_type inv;
    mgr->getRoomToWorldMatrix(inv);
    qogl_matrix_type result1, result2;
    qogl_matrix_mult(result1,afterTransform,inv);
    qogl_matrix_mult(result2,inv,afterTransform);
    q_matrix_type ident = Q_ID_MATRIX;
    for (int i = 0; i < 16; i++) {
        if (Q_ABS(result1[i] - ident[i>>2][i&3]) > Q_EPSILON) {
            failures++;
            std::cout << "Right inverse test failed!" << std::endl;
            qogl_print_matrix(result1);
        }
        if (Q_ABS(result2[i] - ident[i>>2][i&3]) > Q_EPSILON) {
            failures++;
            std::cout << "Left inverse test failed!" << std::endl;
            qogl_print_matrix(result2);
        }
        if (failures) {
            break;
        }
    }
    return failures;
}


int testTranslate(TransformManager *mgr) {
    int failures = 0;
    qogl_matrix_type before, after;
    mgr->getWorldToRoomMatrix(before);
    mgr->translateWorldRelativeToRoom(3,12,-7);
    mgr->getWorldToRoomMatrix(after);
    for (int i = 0; i < 12; i++) {
        if (before[i] != after[i]) {
            std::cout << "Translating changed rotation matrix!" << std::endl;
            failures++;
        }
    }
    double shouldBe[4] = {3,12,-7,0};
    qogl_matrix_vec_mult(shouldBe,before,shouldBe,false);
    for (int i = 0; i < 4; i++) {
        if (Q_ABS(after[12+i] - before[12+i] - shouldBe[i]) > Q_EPSILON) {
            failures++;
            std::cout << "Wrong translation!" << std::endl;
        }
    }
    if (failures != 0) {
        std::cout << "Before: " << std::endl;
        qogl_print_matrix(before);
        std::cout << "After: " << std::endl;
        qogl_print_matrix(after);
    }
    // test inverse
    failures += testInverses(mgr,after);

    mgr->translateWorldRelativeToRoom(-3,-12,7);
    if (!testIsUndone(mgr,before)) {
        std::cout << "Translation not undone!" << std::endl;
        failures++;
    }

    return failures;
}

int testRotate(TransformManager *mgr) {
    int failures = 0;
    qogl_matrix_type before, after;
    mgr->getWorldToRoomMatrix(before);

    q_vec_type a = {2,3,-1}, b = {-3,7,5};
    q_type q, qinv;
    q_from_two_vecs(q,a,b);
    q_from_two_vecs(qinv,b,a);

    mgr->rotateWorldRelativeToRoom(q);
    mgr->getWorldToRoomMatrix(after);

    failures += testInverses(mgr,after);

    mgr->rotateWorldRelativeToRoom(qinv);

    if (!testIsUndone(mgr,before)) {
        failures++;
        std::cout << "Room Rotation not undone!" << std::endl;
    }

    return failures;
}

int testScale(TransformManager *mgr) {
    int failures = 0;
    qogl_matrix_type before, after;
    double scale = 5;
    mgr->getWorldToRoomMatrix(before);
    mgr->scaleWorldRelativeToRoom(scale);
    mgr->getWorldToRoomMatrix(after);

    for (int i = 0; i < 12  ; i++) {
        if (Q_ABS(before[i] * scale - after[i]) > Q_EPSILON) {
            failures++;
            std::cout << "Scale matrix wrong!" << std::endl;
            std::cout << "Before: " << std::endl;
            qogl_print_matrix(before);
            std::cout << "After: " << std::endl;
            qogl_print_matrix(after);
            break;
        }
    }

    failures += testInverses(mgr,after);

    mgr->scaleWorldRelativeToRoom(1/scale);
    if (!testIsUndone(mgr,before)) {
        failures++;
        std::cout << "Scaling not undone!" << std::endl;
    }

    return failures;
}

/*
 * This method tests the getLeftTrackerPosition and methods
 * in combination with translation, rotation, and scaling
 *
 * This is the most likely test to find errors in any of the methods since it
 * has a definite way to measure them
 */
int testLeftTrackerPosition() {
    int failures = 0;
    TransformManager mgr = TransformManager();
    q_vec_type vec;
    mgr.getLeftTrackerPosInWorldCoords(vec);
    // the default tracker base offset is <0,0,10> (all other default translations
    // are identity)
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 10) {
        failures++;
        std::cout << "Initial tracker position wrong" << std::endl;
        q_vec_print(vec);
    }
    // this should put the tracker at the origin
    mgr.translateWorldRelativeToRoom(0,0,10);
    mgr.getLeftTrackerPosInWorldCoords(vec);
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 0) {
        failures++;
        std::cout << "Tracker position after translation wrong" << std::endl;
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(5,2,0);
    mgr.getLeftTrackerPosInWorldCoords(vec);
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
    mgr.getLeftTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]*vec[2] - 29) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after rotation wrong";
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(vec);
    mgr.rotateWorldRelativeToRoom(qInv);
    mgr.getLeftTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after third translation wrong" << std::endl;
        q_vec_print(vec);
    }
    q_type ident = Q_ID_QUAT;
    q_vec_type tracker;
    q_vec_set(tracker,0,1,0);
    mgr.setLeftHandTransform(tracker,ident);
    mgr.getLeftTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]-8*tracker[0]) > Q_EPSILON
            || Q_ABS(vec[1]-8*tracker[1]) > Q_EPSILON
            || Q_ABS(vec[2]-8*tracker[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    // test in combination with scaling
    mgr.scaleWorldRelativeToRoom(2);
    mgr.getLeftTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]-4*tracker[0]) > Q_EPSILON
            || Q_ABS(vec[1]-4*tracker[1]) > Q_EPSILON
            || Q_ABS(vec[2]-4*tracker[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    return failures;
}

int testRightTrackerPosition() {
    int failures = 0;
    TransformManager mgr = TransformManager();
    q_vec_type vec;
    mgr.getRightTrackerPosInWorldCoords(vec);
    // the default tracker base offset is <0,0,10> (all other default translations
    // are identity)
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 10) {
        failures++;
        std::cout << "Initial tracker position wrong" << std::endl;
        q_vec_print(vec);
    }
    // this should put the tracker at the origin
    mgr.translateWorldRelativeToRoom(0,0,10);
    mgr.getRightTrackerPosInWorldCoords(vec);
    if (vec[0] != vec[1] || vec[1] != 0 || vec[2] != 0) {
        failures++;
        std::cout << "Tracker position after translation wrong" << std::endl;
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(5,2,0);
    mgr.getRightTrackerPosInWorldCoords(vec);
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
    mgr.getRightTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]*vec[2] - 29) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after rotation wrong";
        q_vec_print(vec);
    }
    mgr.translateWorldRelativeToRoom(vec);
    mgr.rotateWorldRelativeToRoom(qInv);
    mgr.getRightTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]) > Q_EPSILON || Q_ABS(vec[1]) > Q_EPSILON
            || Q_ABS(vec[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after third translation wrong" << std::endl;
        q_vec_print(vec);
    }
    q_type ident = Q_ID_QUAT;
    q_vec_type tracker;
    q_vec_set(tracker,0,1,0);
    mgr.setRightHandTransform(tracker,ident);
    mgr.getRightTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]-8*tracker[0]) > Q_EPSILON
            || Q_ABS(vec[1]-8*tracker[1]) > Q_EPSILON
            || Q_ABS(vec[2]-8*tracker[2]) > Q_EPSILON) {
        failures++;
        std::cout << "Tracker position after moving tracker is wrong" << std::endl;
        q_vec_print(vec);
    }
    // test in combination with scaling
    mgr.scaleWorldRelativeToRoom(2);
    mgr.getRightTrackerPosInWorldCoords(vec);
    if (Q_ABS(vec[0]-4*tracker[0]) > Q_EPSILON
            || Q_ABS(vec[1]-4*tracker[1]) > Q_EPSILON
            || Q_ABS(vec[2]-4*tracker[2]) > Q_EPSILON) {
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
    q_vec_print(a);
    q_vec_print(b);

    // we need to set up for this test
    // difference between the trackers should initially be the a vector
    // final difference should be the b vector

    mgr.translateWorldRelativeToRoom(0,0,10); // world origin = tracker origin
    mgr.scaleWorldRelativeToRoom(8); // world units = tracker units
    // set tracker positions
    mgr.setLeftHandTransform(pos,ident);
    mgr.setRightHandTransform(pos2,ident);


    q_vec_type lpos1, rpos1, difference1;
    mgr.getLeftTrackerPosInWorldCoords(lpos1);
    mgr.getRightTrackerPosInWorldCoords(rpos1);

    q_vec_subtract(difference1,rpos1,lpos1);

    for (int i = 0; i < 3; i++) {
        if (Q_ABS(difference1[i] -a[i]) > Q_EPSILON) {
            failures++;
            std::cout << "initial locaitons wrong" << std::endl;
        }
    }

    // get the initial matrix to compare to
    qogl_matrix_type before, after;
    mgr.getWorldToRoomMatrix(before);

    // set up quaternions to rotate a to b and vice versa
    q_type q, qinv;
    q_from_two_vecs(q,a,b); // q rotates a to b
    q_from_two_vecs(qinv,b,a); // qinv rotates b to a

    // note that rotating the world by qinv gives the right tracker (whose initial difference
    // from the left tracker was a) a difference of b from the other
    mgr.rotateWorldRelativeToRoomAboutLeftTracker(qinv);
    mgr.getWorldToRoomMatrix(after);

    failures += testInverses(&mgr,after);

    q_vec_type lpos2, rpos2, difference2;
    mgr.getLeftTrackerPosInWorldCoords(lpos2);
    mgr.getRightTrackerPosInWorldCoords(rpos2);

    q_vec_subtract(difference2,rpos2,lpos2);

    // test the resulting difference in positions of left & right trackers
    for (int i = 0; i < 3; i++) {
        if (Q_ABS(difference2[i] -b[i]) > Q_EPSILON) {
            failures++;
            std::cout << "rotation wrong" << std::endl;
            q_vec_print(difference1);
            q_vec_print(difference2);
        }
    }

    // make sure the left tracker didn't move
    for (int i = 0; i < 3; i++) {
        if (Q_ABS(lpos1[i]-lpos2[i]) > Q_EPSILON) {
            failures++;
            std::cout << "Tracker we rotated around moved!";
        }
    }

    // undo the rotation
    mgr.rotateWorldRelativeToRoomAboutLeftTracker(q);
    // make sure it is undone
    if (!testIsUndone(&mgr,before)) {
        failures++;
        std::cout << "Point Rotation not undone!" << std::endl;
        qogl_print_matrix(before);
        qogl_print_matrix(after);
        qogl_matrix_type after2;
        mgr.getWorldToRoomMatrix(after2);
        qogl_print_matrix(after2);
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
    q_vec_print(a);
    q_vec_print(b);

    // we need to set up for this test
    // difference between the trackers should initially be the a vector
    // final difference should be the b vector

    mgr.translateWorldRelativeToRoom(0,0,10); // world origin = tracker origin
    mgr.scaleWorldRelativeToRoom(8); // world units = tracker units
    // set tracker positions
    mgr.setRightHandTransform(pos,ident);
    mgr.setLeftHandTransform(pos2,ident);


    q_vec_type lpos1, rpos1, difference1;
    mgr.getLeftTrackerPosInWorldCoords(lpos1);
    mgr.getRightTrackerPosInWorldCoords(rpos1);

    q_vec_subtract(difference1,lpos1,rpos1);

    for (int i = 0; i < 3; i++) {
        if (Q_ABS(difference1[i] -a[i]) > Q_EPSILON) {
            failures++;
            std::cout << "initial locaitons wrong" << std::endl;
        }
    }

    // get the initial matrix to compare to
    qogl_matrix_type before, after;
    mgr.getWorldToRoomMatrix(before);

    // set up quaternions to rotate a to b and vice versa
    q_type q, qinv;
    q_from_two_vecs(q,a,b); // q rotates a to b
    q_from_two_vecs(qinv,b,a); // qinv rotates b to a

    // note that rotating the world by qinv gives the left tracker (whose initial difference
    // from the right tracker was a) a difference of b from the other
    mgr.rotateWorldRelativeToRoomAboutRightTracker(qinv);
    mgr.getWorldToRoomMatrix(after);

    failures += testInverses(&mgr,after);

    q_vec_type lpos2, rpos2, difference2;
    mgr.getLeftTrackerPosInWorldCoords(lpos2);
    mgr.getRightTrackerPosInWorldCoords(rpos2);

    q_vec_subtract(difference2,lpos2,rpos2);

    // test the resulting difference in positions of left & right trackers
    for (int i = 0; i < 3; i++) {
        if (Q_ABS(difference2[i] -b[i]) > Q_EPSILON) {
            failures++;
            std::cout << "rotation wrong" << std::endl;
            q_vec_print(difference1);
            q_vec_print(difference2);
        }
    }

    // make sure the right tracker didn't move
    for (int i = 0; i < 3; i++) {
        if (Q_ABS(rpos1[i]-rpos2[i]) > Q_EPSILON) {
            failures++;
            std::cout << "Tracker we rotated around moved!";
        }
    }

    // undo the rotation
    mgr.rotateWorldRelativeToRoomAboutRightTracker(q);
    // make sure it is undone
    if (!testIsUndone(&mgr,before)) {
        failures++;
        std::cout << "Point Rotation not undone!" << std::endl;
        qogl_print_matrix(before);
        qogl_print_matrix(after);
        qogl_matrix_type after2;
        mgr.getWorldToRoomMatrix(after2);
        qogl_print_matrix(after2);
    }

    return failures;
}

int testGettingEyeTransforms() {
    return 0;
}

int main(int argc, char *argv[]) {
    int test = 0;
    if (argc > 1) {
        test = (int) (argv[1][0] - '0');
    }
    int status = 0;
    TransformManager mgr = TransformManager();
    switch (test) {
    case 1:
        std::cout << "Testing translations" << std::endl;
        status += testTranslate(&mgr);
        break;
    case 2:
        std::cout << "Testing rotations" << std::endl;
        status += testRotate(&mgr);
        break;
    case 3:
        std::cout << "Testing scaling" << std::endl;
        status += testScale(&mgr);
        break;
    case 4:
        std::cout << "Testing rotation about left tracker" << std::endl;
        status += testRotateAboutLeftTracker();
        std::cout << "Testing rotation about right tracker" << std::endl;
        status += testRotateAboutRightTracker();
        break;
    case 5:
        std::cout << "Testing getting eye transformations" << std::endl;
        status += testGettingEyeTransforms();
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
        q_type q;
        q_from_euler(q,45,45,45);
        mgr.translateWorldRelativeToRoom(4,-7,12);
        mgr.rotateWorldRelativeToRoom(q);
        mgr.translateWorldRelativeToRoom(3,2,-8);
        mgr.scaleWorldRelativeToRoom(.3);
        std::cout << "Testing compositions of operations..." << std::endl;
        std::cout << "... translation ..." << std::endl;
        status += testTranslate(&mgr);
        std::cout << "... rotation ..." << std::endl;
        status += testRotate(&mgr);
        std::cout << "... scaling ..." << std::endl;
        status += testScale(&mgr);
        status += testGettingEyeTransforms();
        break;
    }
    return status;
}
