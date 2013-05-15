#include "hydrainputmanager.h"
#include <QDebug>
#include <QSettings>

#define NO_OPERATION                    0
#define DUPLICATE_OBJECT_PENDING        1
#define REPLICATE_OBJECT_PENDING        2
#define ADD_SPRING_PENDING              3
#define ADD_TRANSFORM_EQUALS_PENDING    4


HydraInputManager::HydraInputManager(SketchProject *proj) :
    project(proj),
    tracker(VRPN_RAZER_HYDRA_DEVICE_STRING),
    buttons(VRPN_RAZER_HYDRA_DEVICE_STRING),
    analogRemote(VRPN_RAZER_HYDRA_DEVICE_STRING),
    grabbedWorld(WORLD_NOT_GRABBED),
    lDist(std::numeric_limits<double>::max()),
    rDist(std::numeric_limits<double>::max()),
    lObj(NULL),
    rObj(NULL),
    rightHandDominant(true),
    operationState(NO_OPERATION),
    objectsSelected(),
    positionsSelected()
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

    QSettings settings;
    rightHandDominant = settings.value(QString("handedness"),QString("right")).toString() == QString("right");
}

HydraInputManager::~HydraInputManager() {}

void HydraInputManager::setProject(SketchProject *proj) {
    project = proj;
    lDist = rDist = std::numeric_limits<double>::max();
    lObj = rObj = (SketchObject *) NULL;
    grabbedWorld = WORLD_NOT_GRABBED;
    objectsSelected.clear();
    positionsSelected.clear();
    operationState = NO_OPERATION;
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

    if (buttonsDown[HydraButtonMapping::scale_button_idx(rightHandDominant)]) {
        if (rightHandDominant) {
            transforms->scaleWithLeftTrackerFixed(delta);
        } else {
            transforms->scaleWithRightTrackerFixed(delta);
        }
    }


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
 * This method updates the springs connecting the trackers and the objects and the
 * status variable that determines if the world is grabbed
 */
void HydraInputManager::updateTrackerObjectConnections() {
    WorldManager *world = project->getWorldManager();
    if (world->getNumberOfObjects() == 0)
        return;
    for (int i = 0; i < 2; i++) {
        int buttonIdx;
        int worldGrabConstant;
        SketchObject *objectToGrab;
        SketchObject *tracker;
        QList<SpringConnection *> *springs;
        double dist;
        // select left or right
        if (i == 0) {
            buttonIdx = BUTTON_LEFT(BUMPER_BUTTON_IDX);
            worldGrabConstant = LEFT_GRABBED_WORLD;
            tracker = project->getLeftHandObject();
            springs = world->getLeftSprings();
            objectToGrab = lObj;
            dist = lDist;
        } else if (i == 1) {
            buttonIdx = BUTTON_RIGHT(BUMPER_BUTTON_IDX);
            worldGrabConstant = RIGHT_GRABBED_WORLD;
            tracker = project->getRightHandObject();
            springs = world->getRightSprings();
            objectToGrab = rObj;
            dist = rDist;
        }
        // if they are gripping the trigger
        if (buttonsDown[buttonIdx]) {
            // if we do not have springs yet add them
            if (springs->size() == 0 && grabbedWorld != worldGrabConstant) { // add springs
                if (dist > DISTANCE_THRESHOLD) {
                    if (grabbedWorld == WORLD_NOT_GRABBED) {
                        grabbedWorld = worldGrabConstant;
                    }
                    // allow grabbing world & moving something with other hand...
                    // discouraged, but allowed -> the results are not guaranteed.
                } else {
                    bool left = i == 0;
                    project->grabObject(left ? lObj : rObj, left);
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
    if (buttonPressed && operationState == NO_OPERATION) {
        // events on press
        if (buttonNum == HydraButtonMapping::spring_disable_button_idx(rightHandDominant)) {
            emit toggleWorldSpringsEnabled();
        } else if (buttonNum == HydraButtonMapping::collision_disable_button_idx(rightHandDominant)) {
            emit toggleWorldCollisionsEnabled();
        } else if (buttonNum == HydraButtonMapping::duplicate_object_button(rightHandDominant)) {
            operationState = DUPLICATE_OBJECT_PENDING;
        } else if (buttonNum == HydraButtonMapping::replicate_object_button(rightHandDominant)) {
            SketchObject *obj = rightHandDominant ? rObj : lObj;
            if (rightHandDominant ? rDist : lDist < DISTANCE_THRESHOLD ) { // object is selected
                q_vec_type pos;
                double bb[6];
                obj->getPosition(pos);
                obj->getAABoundingBox(bb);
                pos[Q_Y] += (bb[3] - bb[2]) * 1.5;
                SketchObject *nObj = obj->deepCopy();
                nObj->setPosition(pos);
                project->addObject(nObj);
                int nCopies = floor(100 * analogStatus[rightHandDominant ? ANALOG_LEFT(TRIGGER_ANALOG_IDX)
                                                                         : ANALOG_RIGHT(TRIGGER_ANALOG_IDX)]);
                project->addReplication(obj,nObj,nCopies);
                objectsSelected.append(obj);
                objectsSelected.append(nObj);
                operationState = REPLICATE_OBJECT_PENDING;
            }
        } else if (buttonNum == HydraButtonMapping::spring_add_button_idx(rightHandDominant)) {
            operationState = ADD_SPRING_PENDING;
            SketchObject *obj = NULL;
            if ((rightHandDominant ? rDist : lDist) < DISTANCE_THRESHOLD ) {
                obj = rightHandDominant ? rObj : lObj;
                objectsSelected.append(obj);
            }
            q_vec_type pos;
            TransformManager *transforms = project->getTransformManager();
            if (rightHandDominant) {
                transforms->getRightTrackerPosInWorldCoords(pos);
            } else {
                transforms->getLeftTrackerPosInWorldCoords(pos);
            }
            positionsSelected.append(pos[0]);
            positionsSelected.append(pos[1]);
            positionsSelected.append(pos[2]);
        } else if (buttonNum == HydraButtonMapping::transform_equals_add_button_idx(rightHandDominant)) {
            SketchObject *obj = NULL;
            if (operationState == NO_OPERATION) {
                if ((rightHandDominant ? rDist : lDist) < DISTANCE_THRESHOLD ) {
                    obj = rightHandDominant ? rObj : lObj;
                    objectsSelected.append(obj);
                    operationState = ADD_TRANSFORM_EQUALS_PENDING;
                }
            }
        }
    } else if (!buttonPressed) {
        if (buttonNum == HydraButtonMapping::replicate_object_button(rightHandDominant)
                && operationState == REPLICATE_OBJECT_PENDING ) {
            objectsSelected.clear();
            operationState = NO_OPERATION;
        } else if (buttonNum == HydraButtonMapping::duplicate_object_button(rightHandDominant)
                   && operationState == DUPLICATE_OBJECT_PENDING ) {
            SketchObject *obj = rightHandDominant ? rObj : lObj;
            if (rightHandDominant ? rDist : lDist < DISTANCE_THRESHOLD ) { // object is selected
                q_vec_type pos;
                double bb[6];
                obj->getPosition(pos);
                obj->getAABoundingBox(bb);
                pos[Q_Y] += (bb[3] - bb[2]) * 1.5;
                SketchObject *nObj = obj->deepCopy();
                nObj->setPosition(pos);
                project->addObject(nObj);
            }
            operationState = NO_OPERATION;
        } else if (buttonNum == HydraButtonMapping::spring_add_button_idx(rightHandDominant)
                   && operationState == ADD_SPRING_PENDING ) {
            SketchObject *obj1 = NULL, *obj2 = NULL;
            q_vec_type p1, p2;
            if (objectsSelected.size() > 0) {
                obj1 = objectsSelected[0];
            }
            if ((rightHandDominant ? rDist : lDist) < DISTANCE_THRESHOLD ) {
                obj2 = rightHandDominant ? rObj : lObj;
            }
            if (positionsSelected.size() >= 3) {
                // get points... both in world coordinates
                p1[0] = positionsSelected[0];
                p1[1] = positionsSelected[1];
                p1[2] = positionsSelected[2];
                TransformManager *transforms = project->getTransformManager();
                if (rightHandDominant) {
                    transforms->getRightTrackerPosInWorldCoords(p2);
                } else {
                    transforms->getLeftTrackerPosInWorldCoords(p2);
                }
                double k = 2 * analogStatus[rightHandDominant ? ANALOG_LEFT(TRIGGER_ANALOG_IDX)
                                                              : ANALOG_RIGHT(TRIGGER_ANALOG_IDX)];
                k = 2 - k;
                if (obj1 != NULL) {
                    if (obj2 != NULL && obj1 != obj2) {
                        project->getWorldManager()->addSpring(obj1,obj2,p1,p2,true,k,0);
                    } else {
                        obj1->getWorldSpacePointInModelCoordinates(p1,p1);
                        SpringConnection *conn = new ObjectPointSpring(obj1,0,0,k,p1,p2);
                        project->addSpring(conn);
                    }
                } else if (obj2 != NULL) {
                    obj2->getWorldSpacePointInModelCoordinates(p2,p2);
                    SpringConnection *conn = new ObjectPointSpring(obj2,0,0,k,p2,p1);
                    project->addSpring(conn);
                }
            }
            objectsSelected.clear();
            positionsSelected.clear();
            operationState = NO_OPERATION;
        } else if (buttonNum == HydraButtonMapping::transform_equals_add_button_idx(rightHandDominant)
                   && operationState == ADD_TRANSFORM_EQUALS_PENDING) {
            SketchObject *obj = NULL;
            if ((rightHandDominant ? rDist : lDist) < DISTANCE_THRESHOLD ) {
                obj = rightHandDominant ? rObj : lObj;
                objectsSelected.append(obj);
            }
            if (objectsSelected.contains(NULL)) {
                int idx = objectsSelected.indexOf(NULL) + 1;
                QSharedPointer<TransformEquals> equals(project->addTransformEquals(objectsSelected[0],
                                                                                     objectsSelected[idx+0]));
                if (!equals.isNull()) {
                    for (int i = 1; i < idx && idx + i < objectsSelected.size(); i++) {
                        equals->addPair(objectsSelected[i],objectsSelected[idx+i]);
                    }
                }
                objectsSelected.clear();
                operationState = NO_OPERATION;
            } else {
                objectsSelected.append(NULL);
            }
        }
    } else if (operationState == ADD_TRANSFORM_EQUALS_PENDING) {
        SketchObject *obj = NULL;
        if (objectsSelected.size() > 2 && objectsSelected.contains(NULL)) {
            if ((rightHandDominant ? rDist : lDist) < DISTANCE_THRESHOLD ) {
                obj = rightHandDominant ? rObj : lObj;
                objectsSelected.append(obj);
            } else {
                objectsSelected.clear();
                operationState = NO_OPERATION;
            }
        } else {
            objectsSelected.clear();
            operationState = NO_OPERATION;
        }
    }
    buttonsDown[buttonNum] = buttonPressed;
}

void HydraInputManager::setAnalogStates(const double state[]) {
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = state[i];
    }
    if (operationState == REPLICATE_OBJECT_PENDING) {
        int nCopies = floor(100 * analogStatus[rightHandDominant ? ANALOG_LEFT(TRIGGER_ANALOG_IDX)
                                                                 : ANALOG_RIGHT(TRIGGER_ANALOG_IDX)]);
        project->getReplicas()->back()->setNumShown(nCopies);
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
        qDebug() << "Wrong number of analogs!";
    }
    mgr->setAnalogStates(a.channel);
}

