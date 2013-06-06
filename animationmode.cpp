#include "animationmode.h"
#include <sketchproject.h>
#include <sketchioconstants.h>

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
    if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX) &&
            worldGrabbed == WORLD_NOT_GRABBED)
    {
        worldGrabbed = LEFT_GRABBED_WORLD;
    } else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX) &&
               worldGrabbed == WORLD_NOT_GRABBED)
    {
        worldGrabbed = RIGHT_GRABBED_WORLD;
    }
}

void AnimationMode::buttonReleased(int vrpn_ButtonNum)
{
    if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX) &&
            worldGrabbed == LEFT_GRABBED_WORLD)
        worldGrabbed = WORLD_NOT_GRABBED;
    else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX) &&
             worldGrabbed == RIGHT_GRABBED_WORLD)
        worldGrabbed = WORLD_NOT_GRABBED;
    else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD)
        {
            rObj->setIsVisible(!rObj->isVisible());
            project->getWorldManager()->changedVisibility(rObj);
        }
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        // TODO -- set active camera
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        WorldManager *world = project->getWorldManager();
        if (world->isShowingInvisible())
            world->hideInvisibleObjects();
        else
            world->showInvisibleObjects();
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        // play sample animation
        project->startAnimation();
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