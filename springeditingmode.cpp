#include "springeditingmode.h"
#include "sketchioconstants.h"
#include "sketchproject.h"
#include <limits>

SpringEditingMode::SpringEditingMode(SketchProject *proj, const bool *buttonState,
                                     const double *analogState) :
    HydraInputMode(proj,buttonState,analogState),
    grabbedWorld(WORLD_NOT_GRABBED),
    lSpring(NULL),
    rSpring(NULL),
    lDist(std::numeric_limits<double>::max()),
    rDist(std::numeric_limits<double>::max()),
    lAtEnd1(true),
    rAtEnd1(true),
    leftGrabbedSpring(false),
    rightGrabbedSpring(false)
{
}

SpringEditingMode::~SpringEditingMode()
{
}

void SpringEditingMode::buttonPressed(int vrpn_ButtonNum)
{
    // see if we should grab a spring or the world with each hand
    if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        if (rDist < SPRING_DISTANCE_THRESHOLD)
            rightGrabbedSpring = true;
        else if (grabbedWorld == WORLD_NOT_GRABBED)
            grabbedWorld = RIGHT_GRABBED_WORLD;
    }
    else if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        if (lDist < SPRING_DISTANCE_THRESHOLD)
            leftGrabbedSpring = true;
        else if (grabbedWorld == WORLD_NOT_GRABBED)
            grabbedWorld = LEFT_GRABBED_WORLD;
    }
}

void SpringEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        if (grabbedWorld == RIGHT_GRABBED_WORLD)
            grabbedWorld = WORLD_NOT_GRABBED;
        else if (rightGrabbedSpring)
            rightGrabbedSpring = false;
    }
    else if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        if (grabbedWorld == LEFT_GRABBED_WORLD)
            grabbedWorld = WORLD_NOT_GRABBED;
        else if (leftGrabbedSpring)
            leftGrabbedSpring = false;
    }
}

void SpringEditingMode::analogsUpdated()
{
    // use the default analogs action
    useLeftJoystickToRotateViewPoint();
}

void SpringEditingMode::doUpdatesForFrame()
{
    // if we are grabbing the world, update the world position for the frame
    if (grabbedWorld == LEFT_GRABBED_WORLD)
        grabWorldWithLeft();
    else if (grabbedWorld == RIGHT_GRABBED_WORLD)
        grabWorldWithRight();

    WorldManager *world = project->getWorldManager();

    if ( world->getNumberOfSprings() > 0 )
    {
        // get the tracker positions
        q_vec_type leftTrackerPos, rightTrackerPos;
        TransformManager *transformMgr = project->getTransformManager();
        transformMgr->getLeftTrackerPosInWorldCoords(leftTrackerPos);
        transformMgr->getRightTrackerPosInWorldCoords(rightTrackerPos);

        if ( ! leftGrabbedSpring )
        {
            // if we haven't grabbed a spring, display which spring is grabbable
            SpringConnection *closest;
            bool atEnd1;
            closest = world->getClosestSpring(leftTrackerPos,&lDist,&atEnd1);
            if (closest != lSpring || (atEnd1 != lAtEnd1))
            {
                project->setLeftOutlineSpring(closest,atEnd1);
                lSpring = closest;
                lAtEnd1 = atEnd1;
            }
            if (lDist < SPRING_DISTANCE_THRESHOLD)
            {
                if (!project->isLeftOutlinesVisible())
                {
                    project->setLeftOutlinesVisible(true);
                }
            }
            else if (project->isLeftOutlinesVisible())
            {
                project->setLeftOutlinesVisible(false);
            }
        }
        else
        {
            // if we have grabbed a spring, move that spring's end
            project->setLeftOutlineSpring(lSpring,lAtEnd1);
            if (lAtEnd1)
            {
                lSpring->setEnd1WorldPosition(leftTrackerPos);
            }
            else
            {
                lSpring->setEnd2WorldPosition(leftTrackerPos);
            }
        }

        if ( ! rightGrabbedSpring )
        {
            // if we haven't grabbed a spring, display which spring is grabbable
            SpringConnection *closest;
            bool atEnd1;
            closest = world->getClosestSpring(rightTrackerPos,&rDist,&atEnd1);
            if (closest != rSpring || (atEnd1 != rAtEnd1))
            {
                project->setRightOutlineSpring(closest,atEnd1);
                rSpring = closest;
                rAtEnd1 = atEnd1;
            }
            if (rDist < SPRING_DISTANCE_THRESHOLD)
            {
                if (!project->isRightOutlinesVisible())
                {
                    project->setRightOutlinesVisible(true);
                }
            }
            else if (project->isRightOutlinesVisible())
            {
                project->setRightOutlinesVisible(false);
            }
        }
        else
        {
            // if we have grabbed a spring, move that spring's end
            project->setRightOutlineSpring(rSpring,rAtEnd1);
            if (rAtEnd1)
            {
                rSpring->setEnd1WorldPosition(rightTrackerPos);
            }
            else
            {
                rSpring->setEnd2WorldPosition(rightTrackerPos);
            }
        }
    }
    else
    {
        if (project->isLeftOutlinesVisible())
            project->setLeftOutlinesVisible(false);
        if (project->isRightOutlinesVisible())
            project->setRightOutlinesVisible(false);
    }
}

void SpringEditingMode::clearStatus()
{
    grabbedWorld = WORLD_NOT_GRABBED;
    lSpring = rSpring = NULL;
    lDist = rDist = std::numeric_limits<double>::max();
    lAtEnd1 = rAtEnd1 = true;
    leftGrabbedSpring = rightGrabbedSpring = false;
}
