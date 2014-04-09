#include "objectgrabmode.h"

#include <limits>
#include <cmath>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>

#include "controlFunctions.h"

ObjectGrabMode::ObjectGrabMode(SketchBio::Project *proj, const bool *const b,
                               const double *const a)
    : HydraInputMode(proj, b, a), bumpLevels(true)
{
}

ObjectGrabMode::~ObjectGrabMode() {}

void ObjectGrabMode::buttonPressed(int vrpn_ButtonNum) {}

void ObjectGrabMode::buttonReleased(int vrpn_ButtonNum) {}

void ObjectGrabMode::doUpdatesForFrame()
{
    project->getHand(SketchBioHandId::RIGHT)
        .setSelectionType(SketchBio::OutlineType::OBJECTS);
    project->getHand(SketchBioHandId::LEFT)
        .setSelectionType(SketchBio::OutlineType::OBJECTS);
}

void ObjectGrabMode::clearStatus() { bumpLevels = true; }

void ObjectGrabMode::analogsUpdated()
{
    if (!bumpLevels &&
        std::fabs(analogStatus[ANALOG_RIGHT(UP_DOWN_ANALOG_IDX)]) < 0.25) {
        bumpLevels = true;
    } else if (bumpLevels &&
               analogStatus[ANALOG_RIGHT(UP_DOWN_ANALOG_IDX)] > 0.25) {
        bumpLevels = false;
        ControlFunctions::selectChildOfCurrent(project, 0, false);
        ControlFunctions::selectChildOfCurrent(project, 1, false);
    } else if (bumpLevels &&
               analogStatus[ANALOG_RIGHT(UP_DOWN_ANALOG_IDX)] < -0.25) {
        bumpLevels = false;
        ControlFunctions::selectParentOfCurrent(project, 0, false);
        ControlFunctions::selectParentOfCurrent(project, 1, false);
    }
}
