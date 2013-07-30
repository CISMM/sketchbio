#include "transformeditingmode.h"

#include <limits>

#include <vtkMatrix4x4.h>

#include <sketchioconstants.h>
#include <sketchtests.h>
#include <sketchproject.h>
#include <transformmanager.h>
#include <sketchobject.h>
#include <transformequals.h>
#include <worldmanager.h>
#include <structurereplicator.h>


inline int camera_add_button_idx() {
    return BUTTON_RIGHT(THREE_BUTTON_IDX);
}
inline int transform_equals_add_button_idx() {
    return BUTTON_RIGHT(TWO_BUTTON_IDX);
}
inline int replicate_object_button() {
    return BUTTON_RIGHT(ONE_BUTTON_IDX);
}
inline int duplicate_object_button() {
    return BUTTON_RIGHT(FOUR_BUTTON_IDX);
}


#define NO_OPERATION                    0
#define DUPLICATE_OBJECT_PENDING        1
#define REPLICATE_OBJECT_PENDING        2
#define ADD_SPRING_PENDING              3
#define ADD_TRANSFORM_EQUALS_PENDING    4

TransformEditingMode::TransformEditingMode(SketchProject *proj, const bool *b, const double *a) :
    ObjectGrabMode(proj,b,a),
    operationState(NO_OPERATION),
    objectsSelected(),
    positionsSelected()
{
}

TransformEditingMode::~TransformEditingMode()
{
}

void TransformEditingMode::buttonPressed(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonPressed(vrpn_ButtonNum);
    if (vrpn_ButtonNum == duplicate_object_button())
    {
        operationState = DUPLICATE_OBJECT_PENDING;
        emit newDirectionsString("Move to the object you want to duplicate and release the button");
    }
    else if (vrpn_ButtonNum == replicate_object_button())
    {
        SketchObject *obj = rObj;
        if ( rDist < DISTANCE_THRESHOLD ) { // object is selected
            q_vec_type pos;
            double bb[6];
            obj->getPosition(pos);
            obj->getBoundingBox(bb);
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
    }
    else if (vrpn_ButtonNum == camera_add_button_idx())
    {
        emit newDirectionsString("Release the button to add a camera.");
    }
    else if (vrpn_ButtonNum == transform_equals_add_button_idx())
    {
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

void TransformEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
    if (vrpn_ButtonNum == replicate_object_button()
            && operationState == REPLICATE_OBJECT_PENDING )
    {
        objectsSelected.clear();
        operationState = NO_OPERATION;
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == duplicate_object_button()
               && operationState == DUPLICATE_OBJECT_PENDING )
    {
        SketchObject *obj = rObj;
        if ( rDist < DISTANCE_THRESHOLD ) { // object is selected
            q_vec_type pos;
            double bb[6];
            obj->getPosition(pos);
            obj->getBoundingBox(bb);
            pos[Q_Y] += (bb[3] - bb[2]) * 1.5;
            SketchObject *nObj = obj->deepCopy();
            nObj->setPosition(pos);
            project->addObject(nObj);
        }
        operationState = NO_OPERATION;
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == camera_add_button_idx())
    {
        TransformManager *transforms = project->getTransformManager();
        q_vec_type pos;
        q_type orient;
        transforms->getRightTrackerPosInWorldCoords(pos);
        transforms->getRightTrackerOrientInWorldCoords(orient);
        project->addCamera(pos,orient);
        emit newDirectionsString(" ");
    } else if (vrpn_ButtonNum == transform_equals_add_button_idx()
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

void TransformEditingMode::analogsUpdated()
{
    if (operationState == REPLICATE_OBJECT_PENDING) {
        double value =  analogStatus[ ANALOG_LEFT(TRIGGER_ANALOG_IDX) ];
        int nCopies =  min( floor( pow(2.0,value / .125)), 64);
        project->getReplicas()->back()->setNumShown(nCopies);
    }
    useLeftJoystickToRotateViewPoint();
}

void TransformEditingMode::clearStatus()
{
    ObjectGrabMode::clearStatus();
    objectsSelected.clear();
    positionsSelected.clear();
    operationState = NO_OPERATION;
}

