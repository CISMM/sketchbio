#include "hydrainputmode.h"
#include "sketchproject.h"
#include "sketchioconstants.h"

HydraInputMode::HydraInputMode(SketchProject *proj, const bool *const b, const double *const a) :
    isButtonDown(b),
    analogStatus(a),
    project(proj)
{
}

HydraInputMode::~HydraInputMode()
{
}

void HydraInputMode::setProject(SketchProject *proj)
{
    project = proj;
    this->clearStatus();
}

void HydraInputMode::useLeftJoystickToRotateViewPoint()
{
    // maybe disable this sometimes?
    double xdegrees, ydegrees;
    xdegrees = 90.0 * analogStatus[ANALOG_LEFT(UP_DOWN_ANALOG_IDX)];
    ydegrees = 180.0 * analogStatus[ANALOG_LEFT(LEFT_RIGHT_ANALOG_IDX)];
    project->getTransformManager()->setRoomEyeOrientation(xdegrees, ydegrees);
}

void HydraInputMode::scaleWithLeftFixed()
{
    TransformManager *transforms = project->getTransformManager();
    double dist = transforms->getOldWorldDistanceBetweenHands();
    double delta = transforms->getWorldDistanceBetweenHands() / dist;
    transforms->scaleWithLeftTrackerFixed(delta);
}

void HydraInputMode::scaleWithRightFixed()
{
    TransformManager *transforms = project->getTransformManager();
    double dist = transforms->getOldWorldDistanceBetweenHands();
    double delta = transforms->getWorldDistanceBetweenHands() / dist;
    transforms->scaleWithRightTrackerFixed(delta);
}

void HydraInputMode::grabWorldWithLeft()
{
    TransformManager *transforms = project->getTransformManager();
    q_vec_type beforePos, afterPos;
    q_type beforeOrient, afterOrient;
    transforms->getLeftTrackerPosInWorldCoords(afterPos);
    transforms->getOldLeftTrackerPosInWorldCoords(beforePos);
    transforms->getLeftTrackerOrientInWorldCoords(afterOrient);
    transforms->getOldLeftTrackerOrientInWorldCoords(beforeOrient);
    // translate
    q_vec_type delta;
    q_vec_subtract(delta,afterPos,beforePos);
    transforms->translateWorldRelativeToRoom(delta);
    // rotate
    q_type inv, rotation;
    q_invert(inv,beforeOrient);
    q_mult(rotation,afterOrient,inv);
    transforms->rotateWorldRelativeToRoomAboutLeftTracker(rotation);
}

void HydraInputMode::grabWorldWithRight()
{
    TransformManager *transforms = project->getTransformManager();
    q_vec_type beforePos, afterPos;
    q_type beforeOrient, afterOrient;
    transforms->getRightTrackerPosInWorldCoords(afterPos);
    transforms->getOldRightTrackerPosInWorldCoords(beforePos);
    transforms->getRightTrackerOrientInWorldCoords(afterOrient);
    transforms->getOldRightTrackerOrientInWorldCoords(beforeOrient);
    // translate
    q_vec_type delta;
    q_vec_subtract(delta,afterPos,beforePos);
    transforms->translateWorldRelativeToRoom(delta);
    // rotate
    q_type inv, rotation;
    q_invert(inv,beforeOrient);
    q_mult(rotation,afterOrient,inv);
    transforms->rotateWorldRelativeToRoomAboutRightTracker(rotation);
}
