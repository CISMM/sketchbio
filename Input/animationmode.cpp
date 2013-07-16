#include "animationmode.h"

#include <limits>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>

AnimationMode::AnimationMode(SketchProject *proj, const bool * const b, const double * const a) :
    ObjectGrabMode(proj,b,a)
{
}

AnimationMode::~AnimationMode()
{
}

void AnimationMode::buttonPressed(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonPressed(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        emit newDirectionsString("Move to an object and release to toggle object visibility.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        emit newDirectionsString("Move to an object and release to add a keyframe\nfor the current location and time.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        emit newDirectionsString("Release to toggle whether invisible objects are shown.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        emit newDirectionsString("Release to preview the animation.");
    }
}

void AnimationMode::buttonReleased(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD)
        {
            // toggle object visible
            rObj->setIsVisible(!rObj->isVisible());
            project->getWorldManager()->changedVisibility(rObj);
        }
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD)
        {
            // add keyframe
            rObj->addKeyframeForCurrentLocation(project->getViewTime());
        }
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        // toggle invisible objects shown
        WorldManager *world = project->getWorldManager();
        if (world->isShowingInvisible())
            world->hideInvisibleObjects();
        else
            world->showInvisibleObjects();
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        // play sample animation
        project->startAnimation();
        emit newDirectionsString(" ");
    }
}

void AnimationMode::analogsUpdated()
{
}
