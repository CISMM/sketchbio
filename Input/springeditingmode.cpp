#include "springeditingmode.h"

#include <limits>

#include <sketchioconstants.h>
#include <springconnection.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <sketchproject.h>

SpringEditingMode::SpringEditingMode(SketchProject* proj, const bool* buttonState,
                                     const double* analogState) :
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
    else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        emit newDirectionsString("Move to a spring and release to delete the spring.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        emit newDirectionsString("Release at location of new spring.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        emit newDirectionsString("Release to create a transparent connector at the current location.");
    }
}

void SpringEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
    {
        if (grabbedWorld == RIGHT_GRABBED_WORLD)
        {
            grabbedWorld = WORLD_NOT_GRABBED;
        }
        else if (rightGrabbedSpring)
        {
            rightGrabbedSpring = false;
            project->setOutlineSpring(RIGHT_SIDE_OUTLINE,rSpring,rAtEnd1);
            addXMLUndoState();
        }
    }
    else if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
    {
        if (grabbedWorld == LEFT_GRABBED_WORLD)
        {
            grabbedWorld = WORLD_NOT_GRABBED;
        }
        else if (leftGrabbedSpring)
        {
            leftGrabbedSpring = false;
            project->setOutlineSpring(LEFT_SIDE_OUTLINE,lSpring,lAtEnd1);
            addXMLUndoState();
        }
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        if (rSpringDist < SPRING_DISTANCE_THRESHOLD)
        {
            project->getWorldManager()->removeSpring(rSpring);
            if (rSpring == lSpring)
            {
                lSpring = NULL;
            }
            rSpring = NULL;
        }
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        q_vec_type pos1, pos2 = {0, 1, 0};
        project->getTransformManager()->getRightTrackerPosInWorldCoords(pos1);
        q_vec_add(pos2,pos1,pos2);
        double stiffness;
        stiffness = ( 1 - analogStatus[ANALOG_LEFT(TRIGGER_ANALOG_IDX)]);
        SpringConnection* spring = SpringConnection::makeSpring(
                    NULL,
                    NULL,
                    pos1,
                    pos2,
                    true,
                    stiffness,
                    0);
        project->addConnector(spring);
        addXMLUndoState();
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        q_vec_type pos1, pos2 = {0, 1, 0};
        project->getTransformManager()->getRightTrackerPosInWorldCoords(pos1);
        q_vec_add(pos2,pos1,pos2);
        Connector* conn = new Connector(NULL,NULL,pos1,pos2,0.4,10);
        project->addConnector(conn);
        addXMLUndoState();
        emit newDirectionsString(" ");
    }
}

void SpringEditingMode::analogsUpdated()
{
    // use the default analogs action
    useLeftJoystickToRotateViewPoint();
}

static inline void processFrameForSide(SketchProject* project,
                                Connector* & spring, // reference to pointer
                                double& springDist,
                                bool& atEnd1,
                                bool grabbedSpring,
                                q_vec_type trackerPos,
                                SketchObject* trackerObj,
                                int side)
{
    WorldManager* world = project->getWorldManager();
    if ( ! grabbedSpring )
    {
        // if we haven't grabbed a spring, display which spring is grabbable
        Connector* closest;
        bool newAtEnd1;
        closest = world->getClosestConnector(trackerPos,&springDist,&newAtEnd1);
        if (closest != spring || (newAtEnd1 != atEnd1))
        {
            project->setOutlineSpring(side,closest,newAtEnd1);
            spring = closest;
            atEnd1 = newAtEnd1;
        }
        if (springDist < SPRING_DISTANCE_THRESHOLD)
        {
            if (!project->isOutlineVisible(side))
            {
                project->setOutlineVisible(side,true);
            }
        }
        else if (project->isOutlineVisible(side))
        {
            project->setOutlineVisible(side,false);
        }
    }
    else
    {
        // if we have grabbed a spring, move that spring's end
        double objectDist = 0;
        SketchObject* closestObject = world->getClosestObject(trackerObj,objectDist);
        if (objectDist > DISTANCE_THRESHOLD)
        {
            closestObject = NULL;
            project->setOutlineSpring(side,spring,atEnd1);
        }
        else
        {
            project->setOutlineObject(side,closestObject);
        }
        if (atEnd1)
        {
            if (closestObject != spring->getObject1())
                spring->setObject1(closestObject);
            spring->setEnd1WorldPosition(trackerPos);
        }
        else
        {
            if (closestObject != spring->getObject2())
                spring->setObject2(closestObject);
            spring->setEnd2WorldPosition(trackerPos);
        }
    }
}

void SpringEditingMode::doUpdatesForFrame()
{
    // if we are grabbing the world, update the world position for the frame
    if (grabbedWorld == LEFT_GRABBED_WORLD)
        grabWorldWithLeft();
    else if (grabbedWorld == RIGHT_GRABBED_WORLD)
        grabWorldWithRight();

    WorldManager* world = project->getWorldManager();
    SketchObject* leftHand = project->getLeftHandObject();
    SketchObject* rightHand = project->getRightHandObject();

    if ( world->getNumberOfConnectors() > 0 )
    {
        // get the tracker positions
        q_vec_type leftTrackerPos, rightTrackerPos;
        TransformManager* transformMgr = project->getTransformManager();
        transformMgr->getLeftTrackerPosInWorldCoords(leftTrackerPos);
        transformMgr->getRightTrackerPosInWorldCoords(rightTrackerPos);

        processFrameForSide(project,lSpring,lSpringDist,lAtEnd1,leftGrabbedSpring,
                            leftTrackerPos,leftHand,LEFT_SIDE_OUTLINE);
        processFrameForSide(project,rSpring,rSpringDist,rAtEnd1,rightGrabbedSpring,
                            rightTrackerPos,rightHand,RIGHT_SIDE_OUTLINE);
    }
    else
    {
        if (project->isOutlineVisible(LEFT_SIDE_OUTLINE))
            project->setOutlineVisible(LEFT_SIDE_OUTLINE,false);
        if (project->isOutlineVisible(RIGHT_SIDE_OUTLINE))
            project->setOutlineVisible(RIGHT_SIDE_OUTLINE,false);
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
