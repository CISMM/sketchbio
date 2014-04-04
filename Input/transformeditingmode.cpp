#include "transformeditingmode.h"

#include <limits>

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataAlgorithm.h>

#include <sketchioconstants.h>
#include <sketchtests.h>
#include <sketchproject.h>
#include <transformmanager.h>
#include <sketchobject.h>
#include <transformequals.h>
#include <worldmanager.h>
#include <structurereplicator.h>

#include "TransformInputDialog.h"

// initial port to controlFunctions (i.e. "generalized input")
#include "controlFunctions.h"

inline int transform_equals_add_button_idx()
{
  return BUTTON_RIGHT(TWO_BUTTON_IDX);
}
inline int replicate_object_button() { return BUTTON_RIGHT(ONE_BUTTON_IDX); }
inline int delete_object_button() { return BUTTON_RIGHT(FOUR_BUTTON_IDX); }

#define NO_OPERATION 0
#define DELETE_OBJECT_PENDING 1
#define REPLICATE_OBJECT_PENDING 2
#define SET_TRANFORM_PENDING 3
#define ADD_TRANSFORM_EQUALS_PENDING 4

TransformEditingMode::TransformEditingMode(SketchBio::Project *proj, const bool *b,
                                           const double *a)
    : ObjectGrabMode(proj, b, a),
      operationState(NO_OPERATION),
      objectsSelected()
{
}

TransformEditingMode::~TransformEditingMode() {}

void TransformEditingMode::buttonPressed(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 0, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 1, true);
  } else if (vrpn_ButtonNum == delete_object_button()) {
    ControlFunctions::deleteObject(project, 1, true);
    
    /*operationState = DELETE_OBJECT_PENDING;
    emit newDirectionsString(
        "Move to the object you want to delete and"
        " release the button.\nWarning: this is a"
        " permanent delete!");*/
  } else if (vrpn_ButtonNum == replicate_object_button()) {
    ControlFunctions::replicateObject(project, 1, true);
    // TODO add control function
    //        SketchObject *obj = rObj;
    //        if ( rDist < DISTANCE_THRESHOLD ) { // object is selected
    //            q_vec_type pos;
    //            double bb[6];
    //            obj->getPosition(pos);
    //            obj->getOrientedBoundingBoxes()->GetOutput()->GetBounds(bb);
    //            double difference;
    //            difference = bb[3] - bb[2];
    //            pos[Q_Y] += difference;
    //            SketchObject *nObj = obj->getCopy();
    //            nObj->setPosition(pos);
    //            project->addObject(nObj);
    //            int nCopies = 1;
    //            project->addReplication(obj,nObj,nCopies);
    //            objectsSelected.append(obj);
    //            objectsSelected.append(nObj);
    //            operationState = REPLICATE_OBJECT_PENDING;
    //            emit newDirectionsString("Pull the trigger to set the number
    // of replicas to add,\nthen release the button.");
    //        }
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX)) {
    ControlFunctions::setTransforms(project, 1, true);
    
    /*if (operationState == NO_OPERATION) {
      operationState = SET_TRANFORM_PENDING;
      emit newDirectionsString(
          "Select the object to use as the base of the\n"
          "coordinate system and release the button.");
    } else if (operationState == SET_TRANFORM_PENDING) {
      emit newDirectionsString(
          "Select the object to define the transformation\n"
          "to and release the button.");
    }*/
  } else if (vrpn_ButtonNum == transform_equals_add_button_idx()) {
    // TODO add control function
    //        SketchObject *obj = NULL;
    //        if (operationState == NO_OPERATION) {
    //            if ( rDist < DISTANCE_THRESHOLD ) {
    //                obj = rObj;
    //                objectsSelected.append(obj);
    //                operationState = ADD_TRANSFORM_EQUALS_PENDING;
    //                emit newDirectionsString("Move the tracker to the object
    // that will be 'like'\nthis one and release the button.");
    //            }
    //        } else if (operationState == ADD_TRANSFORM_EQUALS_PENDING) {
    //            if (objectsSelected.size() > 2 &&
    // objectsSelected.contains(NULL)) {
    //                obj = rObj;
    //                if ( rDist < DISTANCE_THRESHOLD &&
    //                        ! objectsSelected.contains(obj) ) {
    //                    objectsSelected.append(obj);
    //                    emit newDirectionsString("Move to the object that will
    // be paired with the second one\nyou selected and release the button.");
    //                } else {
    //                    objectsSelected.clear();
    //                    operationState = NO_OPERATION;
    //                    emit newDirectionsString(" ");
    //                }
    //            } else {
    //                objectsSelected.clear();
    //                operationState = NO_OPERATION;
    //                emit newDirectionsString(" ");
    //            }
    //        }
  }
}

void TransformEditingMode::buttonReleased(int vrpn_ButtonNum)
{
  // ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 0, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 1, false);
  } else if (vrpn_ButtonNum == replicate_object_button()) {
    ControlFunctions::replicateObject(project, 1, false);
    
    /*objectsSelected.clear();
    operationState = NO_OPERATION;
    addXMLUndoState();
    emit newDirectionsString(" ");*/
  } else if (vrpn_ButtonNum == delete_object_button()) {
    ControlFunctions::deleteObject(project, 1, false);
    
    // TODO add control function
    //        if ( rDist < DISTANCE_THRESHOLD ) { // object is selected
    //            project->getWorldManager()->deleteObject(rObj);
    //            project->setOutlineVisible(RIGHT_SIDE_OUTLINE,false);
    //        }
    //        operationState = NO_OPERATION;
    //        addXMLUndoState();
    //        emit newDirectionsString(" ");
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX)) {
    ControlFunctions::setTransforms(project, 1, false);
    // TODO add control function
    //        if (operationState == SET_TRANFORM_PENDING)
    //        {
    //            if (objectsSelected.empty())
    //            {
    //                if (rDist < DISTANCE_THRESHOLD)
    //                {
    //                    objectsSelected.append(rObj);
    //                    emit newDirectionsString("Press to select the object
    // to define relative\n"
    //                                             "to the selected object.");
    //                }
    //                else
    //                {
    //                    emit newDirectionsString(" ");
    //                    operationState = NO_OPERATION;
    //                }
    //            }
    //            else
    //            {
    //                if (rDist < DISTANCE_THRESHOLD)
    //                {
    //                    objectsSelected.append(rObj);
    //                    vtkSmartPointer< vtkTransform > transform =
    //                            vtkSmartPointer< vtkTransform >::New();
    //                    transform->Identity();
    //                    transform->PreMultiply();
    //                    transform->Concatenate(objectsSelected[1]->getLocalTransform());
    //                    transform->Concatenate(objectsSelected[0]->getInverseLocalTransform());
    //                    transform->Update();
    //                    TransformInputDialog *dialog = new
    // TransformInputDialog(
    //                                "Set Transformation from First to
    // Second");
    //                    dialog->setTranslation(transform->GetPosition());
    //                    dialog->setRotation(transform->GetOrientation());
    //                    connect(dialog,SIGNAL(
    //                                transformAquired(double,double,double,
    //                                                 double,double,double)),
    //                            this,SLOT(
    //                                setTransformBetweenActiveObjects(
    //                                    double,double,double,double,double,double)));
    //                    connect(dialog,SIGNAL(rejected()),this,SLOT(cancelSetTransforms()));
    //                    dialog->exec();
    //                    delete dialog;
    //                    // TODO - actually set the transforms
    //                }
    //                objectsSelected.clear();
    //                emit newDirectionsString(" ");
    //                operationState = NO_OPERATION;
    //            }
    //        }
  } else if (vrpn_ButtonNum == transform_equals_add_button_idx() &&
             operationState == ADD_TRANSFORM_EQUALS_PENDING) {
    // TODO add control function
    //        SketchObject *obj = NULL;
    //        obj = rObj;
    //        if ( rDist < DISTANCE_THRESHOLD &&
    //                ! objectsSelected.contains(obj)) {
    //            objectsSelected.append(obj);
    //        }
    //        if (objectsSelected.contains(NULL)) {
    //            int idx = objectsSelected.indexOf(NULL) + 1;
    //            QSharedPointer<TransformEquals>
    // equals(project->addTransformEquals(objectsSelected[0],
    //                                                                               objectsSelected[idx+0]));
    //            if (!equals.isNull()) {
    //                for (int i = 1; i < idx && idx + i <
    // objectsSelected.size(); i++) {
    //                    equals->addPair(objectsSelected[i],objectsSelected[idx+i]);
    //                }
    //            }
    //            objectsSelected.clear();
    //            addXMLUndoState();
    //            operationState = NO_OPERATION;
    //            emit newDirectionsString(" ");
    //        } else {
    //            objectsSelected.append(NULL);
    //            emit newDirectionsString("Move to the object that will be
    // paired with the first one\nyou selected and press the button.");
    //        }
  } else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX)) {
    resetViewPoint();
  }
}

void TransformEditingMode::analogsUpdated()
{
  ObjectGrabMode::analogsUpdated();
  if (operationState == REPLICATE_OBJECT_PENDING) {
    double value = analogStatus[ANALOG_LEFT(TRIGGER_ANALOG_IDX)];
    int nCopies = min(floor(pow(2.0, value / .125)), 64);
    project->getCrystalByExamples().back()->setNumShown(nCopies);
  }
}

void TransformEditingMode::clearStatus()
{
  ObjectGrabMode::clearStatus();
  objectsSelected.clear();
  operationState = NO_OPERATION;
}

void TransformEditingMode::doUpdatesForFrame()
{
  ObjectGrabMode::doUpdatesForFrame();
  useLeftJoystickToRotateViewPoint();
}

void TransformEditingMode::setTransformBetweenActiveObjects(
    double translateX, double translateY, double translateZ, double rotateX,
    double rotateY, double rotateZ)
{
  if (operationState != SET_TRANFORM_PENDING || objectsSelected.size() != 2) {
    return;
  }
  vtkSmartPointer< vtkTransform > transform =
      vtkSmartPointer< vtkTransform >::New();
  transform->Identity();
  transform->Concatenate(objectsSelected[0]->getLocalTransform());
  transform->RotateY(rotateY);
  transform->RotateX(rotateX);
  transform->RotateZ(rotateZ);
  transform->Translate(translateX, translateY, translateZ);
  q_vec_type pos;
  q_type orient;
  SketchObject::getPositionAndOrientationFromTransform(transform, pos, orient);
  if (objectsSelected[1]->getParent() != NULL) {
    SketchObject::setParentRelativePositionForAbsolutePosition(
        objectsSelected[1], objectsSelected[1]->getParent(), pos, orient);
  } else {
    objectsSelected[1]->setPosAndOrient(pos, orient);
  }
}

void TransformEditingMode::cancelSetTransforms() {}
