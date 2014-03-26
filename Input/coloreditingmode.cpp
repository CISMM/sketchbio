#include "coloreditingmode.h"

#include <limits>

#include <connector.h>
#include <sketchioconstants.h>
#include <sketchobject.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <sketchproject.h>

// initial port to controlFunctions (i.e. "generalized input")
#include <controlFunctions.h>

ColorEditingMode::ColorEditingMode(SketchProject* proj, const bool* const b,
                                   const double* const a)
    : ObjectGrabMode(proj, b, a)
{
}

ColorEditingMode::~ColorEditingMode() {}

void ColorEditingMode::buttonPressed(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 0, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX)) {
    ControlFunctions::changeObjectColor(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX)) {
    ControlFunctions::changeObjectColorVariable(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX)) {
    ControlFunctions::toggleObjectVisibility(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX)) {
    ControlFunctions::toggleShowInvisibleObjects(project, 1, true);
  }
}

void ColorEditingMode::buttonReleased(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 0, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabObjectOrWorld(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX)) {
    ControlFunctions::changeObjectColor(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX)) {
    ControlFunctions::changeObjectColorVariable(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX)) {
    ControlFunctions::toggleObjectVisibility(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX)) {
    ControlFunctions::toggleShowInvisibleObjects(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX)) {
    ControlFunctions::resetViewPoint(project, 0, false);
  }
}

void ColorEditingMode::doUpdatesForFrame()
{
  ObjectGrabMode::doUpdatesForFrame();
  SketchBio::Hand& rhand = project->getHand(SketchBioHandId::RIGHT);
  if (rhand.getNearestObjectDistance() < DISTANCE_THRESHOLD) {
    project->setOutlineType(SketchProject::OUTLINE_OBJECTS);
  } else if (rhand.getNearestConnectorDistance() < SPRING_DISTANCE_THRESHOLD) {
    project->setOutlineType(SketchProject::OUTLINE_CONNECTORS);
  }

  useLeftJoystickToRotateViewPoint();
}

void ColorEditingMode::analogsUpdated() { ObjectGrabMode::analogsUpdated(); }
