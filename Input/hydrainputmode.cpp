#include "hydrainputmode.h"

#include <cmath>

#include <QDebug>

#include <sketchproject.h>
#include <sketchioconstants.h>
#include <transformmanager.h>

#include "savedxmlundostate.h"

HydraInputMode::HydraInputMode(SketchBio::Project *proj, const bool *const b, const double *const a) :
    isButtonDown(b),
    analogStatus(a),
    project(proj),
	x_degrees(0),
	y_degrees(0)
{
}

HydraInputMode::~HydraInputMode()
{
}

void HydraInputMode::setProject(SketchBio::Project *proj)
{
    project = proj;
    this->clearStatus();
}

void HydraInputMode::useLeftJoystickToRotateViewPoint()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    if (std::abs(analogStatus[ANALOG_LEFT(UP_DOWN_ANALOG_IDX)]) > .3) {
		x_degrees += analogStatus[ANALOG_LEFT(UP_DOWN_ANALOG_IDX)];
    }
    if (std::abs(analogStatus[ANALOG_LEFT(LEFT_RIGHT_ANALOG_IDX)]) > .3) {
		y_degrees += analogStatus[ANALOG_LEFT(LEFT_RIGHT_ANALOG_IDX)];
    }
    project->getTransformManager().setRoomEyeOrientation(x_degrees, y_degrees);
}

void HydraInputMode::resetViewPoint()
{
	x_degrees = 0;
	y_degrees = 0;
    project->getTransformManager().setRoomEyeOrientation(x_degrees, y_degrees);
}

void HydraInputMode::useRightJoystickToChangeViewTime()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    double signal = analogStatus[ANALOG_RIGHT(LEFT_RIGHT_ANALOG_IDX)];
    double currentTime = project->getViewTime();
    double finalTime = currentTime;
    int sign = (signal >= 0) ? 1 : -1;
    if (Q_ABS(signal) > .8)
    {
        finalTime = currentTime + 1 * sign;
    }
    else if (Q_ABS(signal) > .4)
    {
        finalTime = currentTime + 0.1 * sign;
    }
    else if (Q_ABS(signal) > .2)
    {
        finalTime = (sign == 1) ? ceil(currentTime) : floor(currentTime) ;
    }
    if (finalTime < 0)
        finalTime = 0;
    if (finalTime != currentTime)
    {
        project->setViewTime(finalTime);
        emit viewTimeChanged(finalTime);
    }
}

void HydraInputMode::scaleWithLeftFixed()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    TransformManager &transforms = project->getTransformManager();
    double dist = transforms.getOldWorldDistanceBetweenHands();
    double delta = transforms.getWorldDistanceBetweenHands() / dist;
    transforms.scaleWithTrackerFixed(delta,SketchBioHandId::LEFT);
}

void HydraInputMode::scaleWithRightFixed()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    TransformManager &transforms = project->getTransformManager();
    double dist = transforms.getOldWorldDistanceBetweenHands();
    double delta = transforms.getWorldDistanceBetweenHands() / dist;
    transforms.scaleWithTrackerFixed(delta,SketchBioHandId::RIGHT);
}

void HydraInputMode::addXMLUndoState()
{
    UndoState *last = project->getLastUndoState();
    SavedXMLUndoState *lastXMLState = dynamic_cast< SavedXMLUndoState * >(last);
    SavedXMLUndoState *newXMLState = NULL;
    if (lastXMLState != NULL)
    {
        QSharedPointer< std::string > lastState =
                lastXMLState->getAfterState().toStrongRef();
        newXMLState = new SavedXMLUndoState(*project,lastState);
        if (lastXMLState->getBeforeState().isNull())
        {
            project->popUndoState();
        }
    }
    else
    {
        QSharedPointer< std::string > lastStr(NULL);
        newXMLState = new SavedXMLUndoState(*project,lastStr);
    }
    project->addUndoState(newXMLState);
}
