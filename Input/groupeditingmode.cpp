#include "groupeditingmode.h"

#include <string>
#include <sstream>

#include <QApplication>
#include <QClipboard>

#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

#include <sketchioconstants.h>
#include <transformmanager.h>
#include <sketchobject.h>
#include <objectgroup.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <projecttoxml.h>

// initial port to controlFunctions (i.e. "generalized input")
#include <controlFunctions.h>

GroupEditingMode::GroupEditingMode(SketchProject *proj, bool const *const b,
                                   double const *const a)
    : ObjectGrabMode(proj, b, a)
{
}

GroupEditingMode::~GroupEditingMode() {}

void GroupEditingMode::buttonPressed(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 0, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX)) {
    ControlFunctions::toggleGroupMembership(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX)) {
    ControlFunctions::copyObject(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX)) {
    ControlFunctions::pasteObject(project, 1, true);
  }
}

void GroupEditingMode::buttonReleased(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 0, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX)) {
    ControlFunctions::toggleGroupMembership(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX)) {
    ControlFunctions::copyObject(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX)) {
    ControlFunctions::pasteObject(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX)) {
    ControlFunctions::resetViewPoint(project, 0, false);
  }
}

void GroupEditingMode::analogsUpdated() { ObjectGrabMode::analogsUpdated(); }

void GroupEditingMode::doUpdatesForFrame()
{
  ObjectGrabMode::doUpdatesForFrame();
  bool givenNewDirections = false;
  //    if (lDist < DISTANCE_THRESHOLD && rDist < DISTANCE_THRESHOLD &&
  //            isButtonDown[BUTTON_RIGHT(ONE_BUTTON_IDX)])
  //    {
  //        ObjectGroup *lGrp = dynamic_cast< ObjectGroup * >(lObj);
  //        ObjectGroup *rGrp = dynamic_cast< ObjectGroup * >(rObj);
  //        if (lObj == rObj && lGrp != NULL)
  //        {
  //            SketchObject *obj = WorldManager::getClosestObject(
  //                        *rGrp->getSubObjects(),
  //                        project->getRightHandObject(),rDist);
  //            if (rDist < DISTANCE_THRESHOLD)
  //            {
  //                emit newDirectionsString("Release to remove object from
  // group");
  //                givenNewDirections = true;
  //                project->setOutlineObject(RIGHT_SIDE_OUTLINE,obj);
  //            }
  //        }
  //        else if (lObj != rObj)
  //        {
  //            if (lGrp != NULL && rGrp != NULL)
  //            {
  //                emit newDirectionsString("Release to join groups into one");
  //                givenNewDirections = true;
  //            }
  //            else
  //            {
  //                emit newDirectionsString("Release to join objects into
  // group");
  //                givenNewDirections = true;
  //            }
  //        }
  //    }
  //    if ( isButtonDown[BUTTON_RIGHT(ONE_BUTTON_IDX)] && !givenNewDirections )
  //    {
  //        emit newDirectionsString("Select the group with the left hand\n"
  //                                 "and the object to add/remove with the
  // right hand");
  //    }
  useLeftJoystickToRotateViewPoint();
}
