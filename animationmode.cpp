#include "animationmode.h"
#include <sketchproject.h>
#include <sketchioconstants.h>

AnimationMode::AnimationMode(SketchProject *proj, const bool * const b, const double * const a) :
    HydraInputMode(proj,b,a),
    worldGrabbed(WORLD_NOT_GRABBED)
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
}

void AnimationMode::clearStatus()
{
    worldGrabbed = WORLD_NOT_GRABBED;
}
