#include "hydrainputmanager.h"
#include <QDebug>

HydraInputManager::HydraInputManager(SketchProject *proj) :
    project(proj),
    tracker(VRPN_RAZER_HYDRA_DEVICE_STRING),
    buttons(VRPN_RAZER_HYDRA_DEVICE_STRING),
    analogRemote(VRPN_RAZER_HYDRA_DEVICE_STRING),
    grabbedWorld(WORLD_NOT_GRABBED),
    lDist(std::numeric_limits<double>::max()),
    rDist(std::numeric_limits<double>::max()),
    lObj(NULL),
    rObj(NULL)
{
    for (int i = 0; i < NUM_HYDRA_BUTTONS; i++) {
        buttonsDown[i] = false;
    }
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = 0.0;
    }

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
    buttons.register_change_handler((void *) this, handle_button);
    analogRemote.register_change_handler((void *) this, handle_analogs);
}

HydraInputManager::~HydraInputManager() {}

void HydraInputManager::setProject(SketchProject *proj) {
    project = proj;
    lDist = rDist = std::numeric_limits<double>::max();
    lObj = rObj = (SketchObject *) NULL;
    grabbedWorld = WORLD_NOT_GRABBED;
}

void HydraInputManager::handleCurrentInput() {
    // get new data from hydra
    tracker.mainloop();
    buttons.mainloop();
    analogRemote.mainloop();

    // use the updated data

    q_vec_type beforeDVect, afterDVect, beforeLPos, beforeRPos, afterLPos, afterRPos;
    q_type beforeLOr, afterLOr, beforeROr, afterROr;

    TransformManager *transforms = project->getTransformManager();

    // get former measurements
    double dist = transforms->getOldWorldDistanceBetweenHands();
    transforms->getOldLeftTrackerPosInWorldCoords(beforeLPos);
    transforms->getOldRightTrackerPosInWorldCoords(beforeRPos);
    transforms->getOldLeftTrackerOrientInWorldCoords(beforeLOr);
    transforms->getOldRightTrackerOrientInWorldCoords(beforeROr);
    transforms->getOldLeftToRightHandWorldVector(beforeDVect);

    // get current measurements
    double delta = transforms->getWorldDistanceBetweenHands() / dist;
    transforms->getLeftToRightHandWorldVector(afterDVect);
    transforms->getLeftTrackerPosInWorldCoords(afterLPos);
    transforms->getRightTrackerPosInWorldCoords(afterRPos);
    transforms->getLeftTrackerOrientInWorldCoords(afterLOr);
    transforms->getRightTrackerOrientInWorldCoords(afterROr);

    /* These are not part of the new input scheme.
    // possibly scale the world
    if (buttonsDown[SCALE_BUTTON]) {
        transforms->scaleWithLeftTrackerFixed(delta);
    }
    // possibly rotate the world
    if (buttonsDown[ROTATE_BUTTON]) {
        q_type rotation;
        q_normalize(afterDVect,afterDVect);
        q_normalize(beforeDVect,beforeDVect);
        q_from_two_vecs(rotation,beforeDVect,afterDVect);
        transforms->rotateWorldRelativeToRoomAboutLeftTracker(rotation);
    }*/
    // if the world is grabbed, translate/rotate it
    if (grabbedWorld == LEFT_GRABBED_WORLD) {
        // translate
        q_vec_type delta;
        q_vec_subtract(delta,afterLPos,beforeLPos);
        transforms->translateWorldRelativeToRoom(delta);
        // rotate
        q_type inv, rotation;
        q_invert(inv,beforeLOr);
        q_mult(rotation,afterLOr,inv);
        transforms->rotateWorldRelativeToRoomAboutLeftTracker(rotation);
    } else if (grabbedWorld == RIGHT_GRABBED_WORLD) {
        // translate
        q_vec_type delta;
        q_vec_subtract(delta,afterRPos,beforeRPos);
        transforms->translateWorldRelativeToRoom(delta);
        // rotate
        q_type inv, rotation;
        q_invert(inv,beforeROr);
        q_mult(rotation,afterROr,inv);
        transforms->rotateWorldRelativeToRoomAboutRightTracker(rotation);
    }
    // move fibers
    updateTrackerObjectConnections();

    // update the main camera
    transforms->updateCameraForFrame();

    WorldManager *world = project->getWorldManager();
    SketchObject *leftHand = project->getLeftHandObject();
    SketchObject *rightHand = project->getRightHandObject();

    // we don't want to show bounding boxes during animation
    if (world->getNumberOfObjects() > 0) {// && !isDoingAnimation) { TODO - fix this
        bool oldShown = false;

        SketchObject *closest = NULL;

        if (world->getLeftSprings()->size() == 0 ) {
            oldShown = project->isLeftOutlinesVisible();
            closest = world->getClosestObject(leftHand,&lDist);

            if (lObj != closest) {
                project->setLeftOutlineObject(closest);
                lObj = closest;
            }
            if (lDist < DISTANCE_THRESHOLD) {
                if (!oldShown) {
                    project->setLeftOutlinesVisible(true);
                }
            } else if (oldShown) {
                project->setLeftOutlinesVisible(false);
            }
        }

        if (world->getRightSprings()->size() == 0 ) {
            oldShown = project->isRightOutlinesVisible();
            closest = world->getClosestObject(rightHand,&rDist);

            if (rObj != closest) {
                project->setRightOutlineObject(closest);
                rObj = closest;
            }
            if (rDist < DISTANCE_THRESHOLD) {
                if (!oldShown) {
                    project->setRightOutlinesVisible(true);
                }
            } else if (oldShown) {
                project->setRightOutlinesVisible(false);
            }
        }
    }

    // set tracker locations
    project->updateTrackerPositions();
    // animation button
    if (buttonsDown[0]) {
        project->startAnimation();
    }
}

/*
 * This function takes a q_vec_type and returns the index of
 * the component with the minimum magnitude.
 */
inline int getMinIdx(const q_vec_type vec) {
    if (Q_ABS(vec[Q_X]) < Q_ABS(vec[Q_Y])) {
        if (Q_ABS(vec[Q_X]) < Q_ABS(vec[Q_Z])) {
            return Q_X;
        } else {
            return Q_Z;
        }
    } else {
        if (Q_ABS(vec[Q_Y]) < Q_ABS(vec[Q_Z])) {
            return Q_Y;
        } else {
            return Q_Z;
        }
    }
}

/*
 * This method updates the springs connecting the trackers and the objects in the world->..
 */
void HydraInputManager::updateTrackerObjectConnections() {
    WorldManager *world = project->getWorldManager();
    if (world->getNumberOfObjects() == 0)
        return;
    for (int i = 0; i < 2; i++) {
        int analogIdx;
        int worldGrabConstant;
        SketchObject *objectToGrab;
        SketchObject *tracker;
        QList<SpringConnection *> *springs;
        double dist;
        // select left or right
        if (i == 0) {
            analogIdx = HYDRA_LEFT_TRIGGER;
            worldGrabConstant = LEFT_GRABBED_WORLD;
            tracker = project->getLeftHandObject();
            springs = world->getLeftSprings();
            objectToGrab = lObj;
            dist = lDist;
        } else if (i == 1) {
            analogIdx = HYDRA_RIGHT_TRIGGER;
            worldGrabConstant = RIGHT_GRABBED_WORLD;
            tracker = project->getRightHandObject();
            springs = world->getRightSprings();
            objectToGrab = rObj;
            dist = rDist;
        }
        // if they are gripping the trigger
        if (analogStatus[analogIdx] > .1) {
            // if we do not have springs yet add them
            if (springs->size() == 0 && grabbedWorld != worldGrabConstant) { // add springs
                if (dist > DISTANCE_THRESHOLD) {
                    if (grabbedWorld == WORLD_NOT_GRABBED) {
                        grabbedWorld = worldGrabConstant;
                    }
                    // allow grabbing world & moving something with other hand...
                    // discouraged, but allowed -> the results are not guaranteed.
                } else {
                    q_vec_type oPos, tPos, vec;
                    objectToGrab->getPosition(oPos);
                    tracker->getPosition(tPos);
                    q_vec_subtract(vec,tPos,oPos);
                    q_vec_normalize(vec,vec);
                    q_vec_type axis = Q_NULL_VECTOR;
                    axis[getMinIdx(vec)] = 1; // this gives an axis that is guaranteed not to be
                                        // parallel to vec
                    q_vec_type per1, per2; // create two perpendicular unit vectors
                    q_vec_cross_product(per1,axis,vec);
                    q_vec_normalize(per1,per1);
                    q_vec_cross_product(per2,per1,vec); // should already be length 1
                    // create scaled perpendicular vectors
                    q_vec_type tPer1, tPer2;
                    q_vec_scale(tPer1,TRACKER_SIDE_LEN,per1);
                    q_vec_scale(tPer2,TRACKER_SIDE_LEN,per2);
                    q_vec_type /*wPos1,*/ wPos2;
                    // create springs and add them
                    // first spring --defined along the "x" axis (per1)
                    SpringConnection *spring;
                    q_vec_add(wPos2,tPos,tPer1);
                    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                                           analogStatus[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    if (i == 0) {
                        world->addLeftHandSpring(spring);
                    } else if (i == 1) {
                        world->addRightHandSpring(spring);
                    }
                    // second spring --defined as rotated 120 degrees about "z" axis.
                    // coordinates in terms of x and y: (-1/2x, sqrt(3)/2y)
                    q_vec_scale(tPer1,-.5,tPer1);
                    q_vec_scale(tPer2,sqrt(3.0)/2,tPer2);
                    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                    q_vec_add(wPos2,wPos2,tPer2); // + sqrt(3)/2 y
                    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                                           analogStatus[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    if (i == 0) {
                        world->addLeftHandSpring(spring);
                    } else if (i == 1) {
                        world->addRightHandSpring(spring);
                    }
                    // third spring --defined as rotated 240 degrees about "z" axis.
                    // coordinates in terms of x and y: (-1/2x, -sqrt(3)/2y)
                    q_vec_invert(tPer2,tPer2);
                    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                    q_vec_add(wPos2,wPos2,tPer2); // - sqrt(3)/2 y
                    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                                           analogStatus[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    if (i == 0) {
                        world->addLeftHandSpring(spring);
                    } else if (i == 1) {
                        world->addRightHandSpring(spring);
                    }
                }
            } else { // update springs stiffness if they are already there
                for (QListIterator<SpringConnection *> it(*springs); it.hasNext();) {
                    it.next()->setStiffness(analogStatus[analogIdx]);
                }
            }
        } else {
            if (!springs->empty()) { // if we have springs and they are no longer gripping the trigger
                // remove springs between model & tracker, set grabbed objects solid
                if (i == 0) {
                    world->clearLeftHandSprings();
                } else if (i == 1) {
                    world->clearRightHandSprings();
                }
            }
            if (grabbedWorld == worldGrabConstant) {
                grabbedWorld = WORLD_NOT_GRABBED;
            }
        }
    }
}


void HydraInputManager::setLeftPos(q_xyz_quat_type *newPos) {
    project->setLeftHandPos(newPos);
}

void HydraInputManager::setRightPos(q_xyz_quat_type *newPos) {
    project->setRightHandPos(newPos);
}

void HydraInputManager::setButtonState(int buttonNum, bool buttonPressed) {
    buttonsDown[buttonNum] = buttonPressed;
}

void HydraInputManager::setAnalogStates(const double state[]) {
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = state[i];
    }
}

//####################################################################################
// VRPN callback functions
//####################################################################################


void VRPN_CALLBACK HydraInputManager::handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    q_xyz_quat_type data;
    // changes coordinates to OpenGL coords, switching y & z and negating y
    data.xyz[Q_X] = t.pos[Q_X];
    data.xyz[Q_Y] = -t.pos[Q_Z];
    data.xyz[Q_Z] = t.pos[Q_Y];
    data.quat[Q_X] = t.quat[Q_X];
    data.quat[Q_Y] = -t.quat[Q_Z];
    data.quat[Q_Z] = t.quat[Q_Y];
    data.quat[Q_W] = t.quat[Q_W];
    // set the correct hand's position
    if (t.sensor == 0) {
        mgr->setLeftPos(&data);
    } else {
        mgr->setRightPos(&data);
    }
}

void VRPN_CALLBACK HydraInputManager::handle_button(void *userdata, const vrpn_BUTTONCB b) {
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    mgr->setButtonState(b.button,b.state);
}

void VRPN_CALLBACK HydraInputManager::handle_analogs(void *userdata, const vrpn_ANALOGCB a) {
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    if (a.num_channel != NUM_HYDRA_ANALOGS) {
        qDebug() << "We have problems!";
    }
    mgr->setAnalogStates(a.channel);
}

