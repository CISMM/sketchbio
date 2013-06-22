#include "animationmode.h"

#include <limits>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>

AnimationMode::AnimationMode(SketchProject *proj, const bool * const b, const double * const a) :
    HydraInputMode(proj,b,a),
    worldGrabbed(WORLD_NOT_GRABBED),
    lDist(std::numeric_limits<double>::max()),
    rDist(std::numeric_limits<double>::max()),
    lObj(NULL),
    rObj(NULL)
{
}

AnimationMode::~AnimationMode()
{
}

void AnimationMode::buttonPressed(int vrpn_ButtonNum)
{
    if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        if (lDist > DISTANCE_THRESHOLD)
        {
            if (worldGrabbed == WORLD_NOT_GRABBED)
                worldGrabbed = LEFT_GRABBED_WORLD;
        }
        else
            project->grabObject(lObj,true);
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        if (rDist > DISTANCE_THRESHOLD)
        {
            if (worldGrabbed == WORLD_NOT_GRABBED)
                worldGrabbed = RIGHT_GRABBED_WORLD;
        }
        else
            project->grabObject(rObj,false);

    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
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
    if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        if (worldGrabbed == LEFT_GRABBED_WORLD)
            worldGrabbed = WORLD_NOT_GRABBED;
        else if (!project->getWorldManager()->getLeftSprings()->empty())
            project->getWorldManager()->clearLeftHandSprings();
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        if (worldGrabbed == RIGHT_GRABBED_WORLD)
            worldGrabbed = WORLD_NOT_GRABBED;
        else if (!project->getWorldManager()->getRightSprings()->empty())
            project->getWorldManager()->clearRightHandSprings();
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
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

void AnimationMode::doUpdatesForFrame()
{
    if (worldGrabbed == LEFT_GRABBED_WORLD)
        grabWorldWithLeft();
    else if (worldGrabbed == RIGHT_GRABBED_WORLD)
        grabWorldWithRight();
    WorldManager *world = project->getWorldManager();
    SketchObject *leftHand = project->getLeftHandObject();
    SketchObject *rightHand = project->getRightHandObject();

    // we don't want to show bounding boxes during animation
    if (world->getNumberOfObjects() > 0) {
        bool oldShown = false;

        SketchObject *closest = NULL;

        if (world->getLeftSprings()->size() == 0 ) {
            oldShown = project->isLeftOutlinesVisible();
            closest = world->getClosestObject(leftHand,&lDist);

            if (lObj != closest) {
                project->setLeftOutlineObject(closest);
                lObj = closest;
            }
            if (lDist < DISTANCE_THRESHOLD) {
                if (!oldShown) {
                    project->setLeftOutlinesVisible(true);
                }
            } else if (oldShown) {
                project->setLeftOutlinesVisible(false);
            }
        }

        if (world->getRightSprings()->size() == 0 ) {
            oldShown = project->isRightOutlinesVisible();
            closest = world->getClosestObject(rightHand,&rDist);

            if (rObj != closest) {
                project->setRightOutlineObject(closest);
                rObj = closest;
            }
            if (rDist < DISTANCE_THRESHOLD) {
                if (!oldShown) {
                    project->setRightOutlinesVisible(true);
                }
            } else if (oldShown) {
                project->setRightOutlinesVisible(false);
            }
        }
    }
}

void AnimationMode::clearStatus()
{
    worldGrabbed = WORLD_NOT_GRABBED;
    lDist = rDist = std::numeric_limits<double>::max();
    lObj = rObj = NULL;
}
