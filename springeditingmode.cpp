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
    lSpringDist(std::numeric_limits<double>::max()),
    rSpringDist(std::numeric_limits<double>::max()),
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
        if (rSpringDist < SPRING_DISTANCE_THRESHOLD)
            rightGrabbedSpring = true;
        else if (grabbedWorld == WORLD_NOT_GRABBED)
            grabbedWorld = RIGHT_GRABBED_WORLD;
    }
    else if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        if (lSpringDist < SPRING_DISTANCE_THRESHOLD)
            leftGrabbedSpring = true;
        else if (grabbedWorld == WORLD_NOT_GRABBED)
            grabbedWorld = LEFT_GRABBED_WORLD;
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        emit newDirectionsString("Release at location of new spring.");
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
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        q_vec_type pos1, pos2 = {0, 1, 0};
        project->getTransformManager()->getRightTrackerPosInWorldCoords(pos1);
        q_vec_add(pos2,pos1,pos2);
        double stiffness;
        stiffness = 2 *( 1 - analogStatus[ANALOG_LEFT(TRIGGER_ANALOG_IDX)]);
        SpringConnection *spring = SpringConnection::makeSpring(
                    NULL,
                    NULL,
                    pos1,
                    pos2,
                    true,
                    stiffness,
                    0);
        project->addSpring(spring);
        emit newDirectionsString(" ");
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
    SketchObject *leftHand = project->getLeftHandObject();
    SketchObject *rightHand = project->getRightHandObject();

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
            closest = world->getClosestSpring(leftTrackerPos,&lSpringDist,&atEnd1);
            if (closest != lSpring || (atEnd1 != lAtEnd1))
            {
                project->setLeftOutlineSpring(closest,atEnd1);
                lSpring = closest;
                lAtEnd1 = atEnd1;
            }
            if (lSpringDist < SPRING_DISTANCE_THRESHOLD)
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
            double objectDist = 0;
            SketchObject *closestObject = world->getClosestObject(leftHand,&objectDist);
            if (objectDist > DISTANCE_THRESHOLD)
            {
                closestObject = NULL;
                project->setLeftOutlineSpring(lSpring,lAtEnd1);
            }
            else
            {
                project->setLeftOutlineObject(closestObject);
            }
            if (lAtEnd1)
            {
                std::cout << ((closestObject == NULL) ? "NULL" : "OBJECT") << endl;
                if (closestObject != lSpring->getObject1())
                    lSpring->setObject1(closestObject);
                lSpring->setEnd1WorldPosition(leftTrackerPos);
            }
            else
            {
                if (closestObject != lSpring->getObject2())
                    lSpring->setObject2(closestObject);
                lSpring->setEnd2WorldPosition(leftTrackerPos);
            }
        }

        if ( ! rightGrabbedSpring )
        {
            // if we haven't grabbed a spring, display which spring is grabbable
            SpringConnection *closest;
            bool atEnd1;
            closest = world->getClosestSpring(rightTrackerPos,&rSpringDist,&atEnd1);
            if (closest != rSpring || (atEnd1 != rAtEnd1))
            {
                project->setRightOutlineSpring(closest,atEnd1);
                rSpring = closest;
                rAtEnd1 = atEnd1;
            }
            if (rSpringDist < SPRING_DISTANCE_THRESHOLD)
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
            double objectDist = 0;
            SketchObject *closestObject = world->getClosestObject(rightHand,&objectDist);
            if (objectDist > DISTANCE_THRESHOLD)
            {
                closestObject = NULL;
                project->setRightOutlineSpring(rSpring,rAtEnd1);
            }
            else
            {
                project->setRightOutlineObject(closestObject);
            }
            if (rAtEnd1)
            {
                if (closestObject != rSpring->getObject1())
                    rSpring->setObject1(closestObject);
                rSpring->setEnd1WorldPosition(rightTrackerPos);
            }
            else
            {
                if (closestObject != rSpring->getObject2())
                    rSpring->setObject2(closestObject);
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
    lSpringDist = rSpringDist = std::numeric_limits<double>::max();
    lAtEnd1 = rAtEnd1 = true;
    leftGrabbedSpring = rightGrabbedSpring = false;
}
