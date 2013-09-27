#include "animationmode.h"

#include <limits>

#include <sketchioconstants.h>
#include <sketchtests.h>
#include <keyframe.h>
#include <sketchobject.h>
#include <transformmanager.h>
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
    if (project->isShowingAnimation())
    {
        if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
        {
            emit newDirectionsString("Release to stop the animation preview.");
        }
        return;
    }
    ObjectGrabMode::buttonPressed(vrpn_ButtonNum);

    if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        emit newDirectionsString("Move to an object and release to add or remove a keyframe\nfor the current location and time.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        emit newDirectionsString("Release the button to add a camera.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        emit newDirectionsString("Release to preview the animation.");
    }
}

void AnimationMode::buttonReleased(int vrpn_ButtonNum)
{
    if (project->isShowingAnimation())
    {
        if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
        {
            project->stopAnimation();
            emit newDirectionsString(" ");
        }
        return;
    }
    ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD)
        {
            // add keyframe
            double time = project->getViewTime();
            bool newFrame = true;
            q_vec_type pos, fpos;
            q_type orient, forient;
            rObj->getPosition(pos);
            rObj->getOrientation(orient);
            if (rObj->hasKeyframes() && rObj->getKeyframes()->contains(time))
            {
                Keyframe frame = rObj->getKeyframes()->value(time);
                frame.getPosition(fpos);
                frame.getOrientation(forient);
                ColorMapType::Type fcmap, cmap;
                cmap = rObj->getColorMapType();
                fcmap = frame.getColorMapType();
                const QString & farray = frame.getArrayToColorBy(),
                              & array = rObj->getArrayToColorBy();
                if (q_vec_equals(pos,fpos) && q_equals(forient,orient)
                        && (cmap == fcmap) &&
                        (( ColorMapType::isSolidColor(cmap,array) &&
                          ColorMapType::isSolidColor(fcmap,farray)) ||
                         (farray == array)))
                {
                    newFrame = false;
                }
            }
            if (newFrame)
            {
                rObj->addKeyframeForCurrentLocation(time);
            }
            else
            {
                rObj->removeKeyframeForTime(time);
                // make sure the object doesn't move just because we removed
                // the keyframe... principle of least astonishment.
                rObj->setPosAndOrient(pos,orient);
            }
            project->getWorldManager()->setAnimationTime(time);
            addXMLUndoState();
        }
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        TransformManager *transforms = project->getTransformManager();
        q_vec_type pos;
        q_type orient;
        transforms->getRightTrackerPosInWorldCoords(pos);
        transforms->getRightTrackerOrientInWorldCoords(orient);
        project->addCamera(pos,orient);
        addXMLUndoState();
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        // play sample animation
        project->startAnimation();
        emit newDirectionsString(" ");
    }
}

void AnimationMode::doUpdatesForFrame()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    ObjectGrabMode::doUpdatesForFrame();
}

void AnimationMode::analogsUpdated()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    ObjectGrabMode::analogsUpdated();
    useLeftJoystickToRotateViewPoint();
}

void AnimationMode::clearStatus()
{
    if (project->isShowingAnimation())
    {
        project->stopAnimation();
    }
    ObjectGrabMode::clearStatus();
}
