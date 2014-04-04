#include "springeditingmode.h"

#include <limits>

#include <sketchioconstants.h>
#include <springconnection.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>

#include "controlFunctions.h"

SpringEditingMode::SpringEditingMode(SketchBio::Project* proj,
                                     const bool* buttonState,
                                     const double* analogState)
    : ObjectGrabMode(proj, buttonState, analogState), snapMode(false)
{
}

SpringEditingMode::~SpringEditingMode() {}

void SpringEditingMode::buttonPressed(int vrpn_ButtonNum)
{
  // see if we should grab a spring or the world with each hand
  if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabSpringOrWorld(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabSpringOrWorld(project, 0, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX)) {
    ControlFunctions::deleteSpring(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX)) {
    // TODO incomplete control function
    ControlFunctions::snapSpringToTerminus(project, 1, true);
    snapMode = true;
    emit newDirectionsString(
        "Move to a spring and choose which terminus to snap to.");
    bool rAtEnd1;
    double rSpringDist =
        project->getHand(SketchBioHandId::RIGHT).getNearestConnectorDistance();
    Connector* rSpring =
        project->getHand(SketchBioHandId::RIGHT).getNearestConnector(&rAtEnd1);
    if (rSpringDist < SPRING_DISTANCE_THRESHOLD) {
      double value = analogStatus[ANALOG_RIGHT(TRIGGER_ANALOG_IDX)];
      bool snap_to_n = (value < 0.5) ? true : false;
      rSpring->snapToTerminus(rAtEnd1, snap_to_n);
    }
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX)) {
    ControlFunctions::createSpring(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX)) {
    ControlFunctions::createTransparentConnector(project, 1, true);
  } else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX)) {
    ControlFunctions::resetViewPoint(project, 0, true);
  }
}

void SpringEditingMode::buttonReleased(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabSpringOrWorld(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX)) {
    ControlFunctions::grabSpringOrWorld(project, 0, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX)) {
    ControlFunctions::deleteSpring(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX)) {
    // TODO incomplete control function
    ControlFunctions::snapSpringToTerminus(project, 1, false);
    snapMode = false;
    emit newDirectionsString(" ");
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX)) {
    ControlFunctions::createSpring(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX)) {
    ControlFunctions::createTransparentConnector(project, 1, false);
  } else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX)) {
    ControlFunctions::resetViewPoint(project, 0, false);
  }
}

void SpringEditingMode::analogsUpdated()
{
  ObjectGrabMode::analogsUpdated();
  bool rAtEnd1;
  double rSpringDist =
      project->getHand(SketchBioHandId::RIGHT).getNearestConnectorDistance();
  Connector* rSpring =
      project->getHand(SketchBioHandId::RIGHT).getNearestConnector(&rAtEnd1);
  if (snapMode && (rSpringDist < SPRING_DISTANCE_THRESHOLD)) {
    double value = analogStatus[ANALOG_RIGHT(TRIGGER_ANALOG_IDX)];
    bool snap_to_n = (value < 0.5) ? true : false;
    rSpring->snapToTerminus(rAtEnd1, snap_to_n);
  }
}

void SpringEditingMode::doUpdatesForFrame()
{

  project->setOutlineType(SketchBio::Project::OUTLINE_CONNECTORS);

  useLeftJoystickToRotateViewPoint();
}

void SpringEditingMode::clearStatus()
{
  ObjectGrabMode::clearStatus();
  snapMode = false;
}
