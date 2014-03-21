#include "animationmode.h"

#include <limits>

#include <sketchioconstants.h>
#include <sketchtests.h>
#include <keyframe.h>
#include <sketchobject.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <sketchproject.h>

// initial port to controlFunctions (i.e. "generalized input")
#include <controlFunctions.h>

AnimationMode::AnimationMode(SketchProject *proj, const bool * const b, const double * const a) :
    ObjectGrabMode(proj,b,a)
{
}

AnimationMode::~AnimationMode()
{
}

void AnimationMode::buttonPressed(int vrpn_ButtonNum)
{
    
    
	//ObjectGrabMode::buttonPressed(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        ControlFunctions::grabObjectOrWorld(project, 0, true);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        ControlFunctions::grabObjectOrWorld(project, 1, true);
    }
	else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
		ControlFunctions::keyframeAll(project, 1, true); 
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
		ControlFunctions::toggleKeyframeObject(project, 1, true);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        ControlFunctions::addCamera(project, 1, true);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        ControlFunctions::showAnimationPreview(project, 1, true);
    }

	/*
    if (project->isShowingAnimation())
    {
        if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
        {
            emit newDirectionsString("Release to stop the animation preview.");
        }
        return;
    }
    ObjectGrabMode::buttonPressed(vrpn_ButtonNum);

    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        emit newDirectionsString("Release to keyframe all objects.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
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
	*/
}

void AnimationMode::buttonReleased(int vrpn_ButtonNum)
{
	//ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
	if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        ControlFunctions::grabObjectOrWorld(project, 0, false);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        ControlFunctions::grabObjectOrWorld(project, 1, false);
    }
	else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
		ControlFunctions::keyframeAll(project, 1, false); 
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
		ControlFunctions::toggleKeyframeObject(project, 1, false);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        ControlFunctions::addCamera(project, 1, false);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        ControlFunctions::showAnimationPreview(project, 1, false);
    }
	else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX))
    {
		ControlFunctions::resetViewPoint(project, 0, false);
    }
	/*
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
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        double time = project->getViewTime();
        QListIterator< SketchObject* > itr =
                project->getWorldManager()->getObjectIterator();
        while ( itr.hasNext() )
        {
            itr.next()->addKeyframeForCurrentLocation(time);
        }
        project->getWorldManager()->setAnimationTime(time);
        emit newDirectionsString(" ");
        addXMLUndoState();
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD && rightLevel == 0)
        {
            // add/update/remove keyframe
            double time = project->getViewTime();
            bool newFrame = rObj->hasChangedSinceKeyframe(time);
            if (newFrame)
            {
                rObj->addKeyframeForCurrentLocation(time);
            }
            else
            {
                rObj->removeKeyframeForTime(time);
            }
            // make sure the object doesn't move just because we removed
            // the keyframe... principle of least astonishment.
            // using this instead of setAnimationTime since it doesn't update
            // positions
            project->getWorldManager()->setKeyframeOutlinesForTime(time);
            addXMLUndoState();
            emit newDirectionsString(" ");
        }
        else if (rightLevel > 0)
        {
            emit newDirectionsString("Cannot keyframe objects in a group individually,\n"
                                     "keyframe the group and it will save the state of all of them.");
        }
        else
        {
            emit newDirectionsString(" ");
        }
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        q_vec_type pos;
        q_type orient;
        SketchBio::Hand& rhand = project->getHand(SketchBioHandId::RIGHT);
        rhand.getPosition(pos);
        rhand.getOrientation(orient);
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
	else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX))
    {
        resetViewPoint();
    }
	*/
}

void AnimationMode::doUpdatesForFrame()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    ObjectGrabMode::doUpdatesForFrame();
	useLeftJoystickToRotateViewPoint();
}

void AnimationMode::analogsUpdated()
{
    if (project->isShowingAnimation())
    {
        return;
    }
    ObjectGrabMode::analogsUpdated();
}

void AnimationMode::clearStatus()
{
    if (project->isShowingAnimation())
    {
        project->stopAnimation();
    }
    ObjectGrabMode::clearStatus();
}
