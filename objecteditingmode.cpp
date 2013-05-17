#include "objecteditingmode.h"
#include "sketchioconstants.h"
#include "sketchtests.h"
#include "sketchproject.h"
#include <limits>

#define NO_OPERATION                    0
#define DUPLICATE_OBJECT_PENDING        1
#define REPLICATE_OBJECT_PENDING        2
#define ADD_SPRING_PENDING              3
#define ADD_TRANSFORM_EQUALS_PENDING    4

ObjectEditingMode::ObjectEditingMode(SketchProject *proj, const bool *b, const double *a) :
    HydraInputMode(proj,b,a),
    grabbedWorld(WORLD_NOT_GRABBED),
    lDist(std::numeric_limits<double>::max()),
    rDist(std::numeric_limits<double>::max()),
    lObj(NULL),
    rObj(NULL),
    operationState(NO_OPERATION),
    objectsSelected(),
    positionsSelected()
{
}

ObjectEditingMode::~ObjectEditingMode()
{
}

void ObjectEditingMode::buttonPressed(int vrpn_ButtonNum)
{
    if (vrpn_ButtonNum == HydraButtonMapping::duplicate_object_button()) {
        operationState = DUPLICATE_OBJECT_PENDING;
        emit newDirectionsString("Move to the object you want to duplicate and release the button");
    } else if (vrpn_ButtonNum == HydraButtonMapping::replicate_object_button()) {
        SketchObject *obj = rObj;
        if ( rDist < DISTANCE_THRESHOLD ) { // object is selected
            q_vec_type pos;
            double bb[6];
            obj->getPosition(pos);
            obj->getAABoundingBox(bb);
            pos[Q_Y] += (bb[3] - bb[2]) * 1.5;
            SketchObject *nObj = obj->deepCopy();
            nObj->setPosition(pos);
            project->addObject(nObj);
            int nCopies = 1;
            project->addReplication(obj,nObj,nCopies);
            objectsSelected.append(obj);
            objectsSelected.append(nObj);
            operationState = REPLICATE_OBJECT_PENDING;
            emit newDirectionsString("Pull the trigger to set the number of replicas to add,\nthen release the button.");
        }
    } else if (vrpn_ButtonNum == HydraButtonMapping::spring_add_button_idx()) {
        operationState = ADD_SPRING_PENDING;
        SketchObject *obj = NULL;
        if ( rDist < DISTANCE_THRESHOLD ) {
            obj = rObj;
            objectsSelected.append(obj);
        }
        q_vec_type pos;
        TransformManager *transforms = project->getTransformManager();
        transforms->getRightTrackerPosInWorldCoords(pos);
        positionsSelected.append(pos[0]);
        positionsSelected.append(pos[1]);
        positionsSelected.append(pos[2]);
        emit newDirectionsString("Move the tracker to the object and position to attach to\nand release the button.");
    } else if (vrpn_ButtonNum == HydraButtonMapping::transform_equals_add_button_idx()) {
        SketchObject *obj = NULL;
        if (operationState == NO_OPERATION) {
            if ( rDist < DISTANCE_THRESHOLD ) {
                obj = rObj;
                objectsSelected.append(obj);
                operationState = ADD_TRANSFORM_EQUALS_PENDING;
                emit newDirectionsString("Move the tracker to the object that will be 'like'\nthis one and release the button.");
            }
        } else if (operationState == ADD_TRANSFORM_EQUALS_PENDING) {
            if (objectsSelected.size() > 2 && objectsSelected.contains(NULL)) {
                obj = rObj;
                if ( rDist < DISTANCE_THRESHOLD &&
                        ! objectsSelected.contains(obj) ) {
                    objectsSelected.append(obj);
                    emit newDirectionsString("Move to the object that will be paired with the second one\nyou selected and release the button.");
                } else {
                    objectsSelected.clear();
                    operationState = NO_OPERATION;
                    emit newDirectionsString(" ");
                }
            } else {
                objectsSelected.clear();
                operationState = NO_OPERATION;
                emit newDirectionsString(" ");
            }
        }
    }
}

void ObjectEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    if (vrpn_ButtonNum == HydraButtonMapping::replicate_object_button()
            && operationState == REPLICATE_OBJECT_PENDING ) {
        objectsSelected.clear();
        operationState = NO_OPERATION;
        emit newDirectionsString(" ");
    } else if (vrpn_ButtonNum == HydraButtonMapping::duplicate_object_button()
               && operationState == DUPLICATE_OBJECT_PENDING ) {
        SketchObject *obj = rObj;
        if ( rDist < DISTANCE_THRESHOLD ) { // object is selected
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
        emit newDirectionsString(" ");
    } else if (vrpn_ButtonNum == HydraButtonMapping::spring_add_button_idx()
               && operationState == ADD_SPRING_PENDING ) {
        SketchObject *obj1 = NULL, *obj2 = NULL;
        q_vec_type p1, p2;
        if (objectsSelected.size() > 0) {
            obj1 = objectsSelected[0];
        }
        if ( rDist < DISTANCE_THRESHOLD ) {
            obj2 = rObj;
        }
        if (positionsSelected.size() >= 3) {
            // get points... both in world coordinates
            p1[0] = positionsSelected[0];
            p1[1] = positionsSelected[1];
            p1[2] = positionsSelected[2];
            TransformManager *transforms = project->getTransformManager();
            transforms->getRightTrackerPosInWorldCoords(p2);
            double k = 2 * analogStatus[ ANALOG_LEFT(TRIGGER_ANALOG_IDX)];
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
        emit newDirectionsString(" ");
    } else if (vrpn_ButtonNum == HydraButtonMapping::transform_equals_add_button_idx()
               && operationState == ADD_TRANSFORM_EQUALS_PENDING) {
        SketchObject *obj = NULL;
        obj = rObj;
        if ( rDist < DISTANCE_THRESHOLD &&
                ! objectsSelected.contains(obj)) {
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
            emit newDirectionsString(" ");
        } else {
            objectsSelected.append(NULL);
            emit newDirectionsString("Move to the object that will be paired with the first one\nyou selected and press the button.");
        }
    }
}

void ObjectEditingMode::analogsUpdated()
{
    if (operationState == REPLICATE_OBJECT_PENDING) {
        double value =  analogStatus[ ANALOG_LEFT(TRIGGER_ANALOG_IDX) ];
        int nCopies =  min( floor( pow(2.0,value / .125)), 64);
        project->getReplicas()->back()->setNumShown(nCopies);
    }
    useLeftJoystickToRotateViewPoint();
}

void ObjectEditingMode::doUpdatesForFrame()
{
    if (grabbedWorld == LEFT_GRABBED_WORLD) {
        grabWorldWithLeft();
    } else if (grabbedWorld == RIGHT_GRABBED_WORLD) {
        grabWorldWithRight();
    }
    WorldManager *world = project->getWorldManager();
    SketchObject *leftHand = project->getLeftHandObject();
    SketchObject *rightHand = project->getRightHandObject();

    // we don't want to show bounding boxes during animation
    if (world->getNumberOfObjects() > 0) {
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
    updateTrackerObjectConnections();
}

void ObjectEditingMode::clearStatus()
{
    lDist = rDist = std::numeric_limits<double>::max();
    lObj = rObj = (SketchObject *) NULL;
    grabbedWorld = WORLD_NOT_GRABBED;
    objectsSelected.clear();
    positionsSelected.clear();
    operationState = NO_OPERATION;
}


/*
 * This method updates the springs connecting the trackers and the objects and the
 * status variable that determines if the world is grabbed
 */
void ObjectEditingMode::updateTrackerObjectConnections() {
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
        if (isButtonDown[buttonIdx]) {
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
