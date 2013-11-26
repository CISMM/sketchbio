#include "coloreditingmode.h"

#include <connector.h>
#include <sketchioconstants.h>
#include <sketchobject.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <sketchproject.h>

ColorEditingMode::ColorEditingMode(SketchProject *proj, const bool * const b,
                                   const double * const a)
    : ObjectGrabMode(proj,b,a),
    springDist(std::numeric_limits<double>::max()),
    rSpring(NULL)
{
}

ColorEditingMode::~ColorEditingMode()
{
}

void ColorEditingMode::buttonPressed(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonPressed(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        emit newDirectionsString("Move to the object and release to change\n"
                                 " the color map of the object");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        emit newDirectionsString("Move to the object and release to change\n"
                                 " the variable the object is colored by.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        emit newDirectionsString("Move to an object and release to toggle "
                                 "object visibility.");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        emit newDirectionsString("Release to toggle whether invisible objects are shown.");
    }
}

void ColorEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD && !rObj->getArrayToColorBy().isEmpty())
        {
            ColorMapType::Type cmap = rObj->getColorMapType();
            using namespace ColorMapType;
            switch (cmap)
            {
            case SOLID_COLOR_RED:
                cmap = SOLID_COLOR_GREEN;
                break;
            case SOLID_COLOR_GREEN:
                cmap = SOLID_COLOR_BLUE;
                break;
            case SOLID_COLOR_BLUE:
                cmap = SOLID_COLOR_YELLOW;
                break;
            case SOLID_COLOR_YELLOW:
                cmap = SOLID_COLOR_PURPLE;
                break;
            case SOLID_COLOR_PURPLE:
                cmap = SOLID_COLOR_CYAN;
                break;
            case SOLID_COLOR_CYAN:
                cmap = BLUE_TO_RED;
                break;
            case BLUE_TO_RED:
                cmap = SOLID_COLOR_RED;
                break;
            default:
                cmap = SOLID_COLOR_RED;
                break;
            }
            rObj->setColorMapType(cmap);
            addXMLUndoState();
        }
		else if (springDist < SPRING_DISTANCE_THRESHOLD) {
			ColorMapType::Type cmap = rSpring->getColorMapType();
            using namespace ColorMapType;
            switch (cmap)
            {
            case SOLID_COLOR_RED:
                cmap = SOLID_COLOR_GREEN;
                break;
            case SOLID_COLOR_GREEN:
                cmap = SOLID_COLOR_BLUE;
                break;
            case SOLID_COLOR_BLUE:
                cmap = SOLID_COLOR_YELLOW;
                break;
            case SOLID_COLOR_YELLOW:
                cmap = SOLID_COLOR_PURPLE;
                break;
            case SOLID_COLOR_PURPLE:
                cmap = SOLID_COLOR_CYAN;
                break;
            case SOLID_COLOR_CYAN:
                cmap = SOLID_COLOR_GRAY;
                break;
            case SOLID_COLOR_GRAY:
                cmap = SOLID_COLOR_RED;
                break;
            default:
                cmap = SOLID_COLOR_GRAY;
                break;
            }
            rSpring->setColorMapType(cmap);
            addXMLUndoState();
		}
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD) {
            QString arr(rObj->getArrayToColorBy());
            if (arr == QString("modelNum"))
            {
                arr = QString("chainPosition");
            }
            else if (arr == QString("chainPosition"))
            {
                arr = QString("charge");
            }
            else if (arr == QString("charge"))
            {
                arr = QString("modelNum");
            }
            rObj->setArrayToColorBy(arr);
            addXMLUndoState();
        }
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(THREE_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD)
        {
            // toggle object visible
            SketchObject::setIsVisibleRecursive(rObj,!rObj->isVisible());
            project->getWorldManager()->changedVisibility(rObj);
            addXMLUndoState();
        }
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        // toggle invisible objects shown
        WorldManager *world = project->getWorldManager();
        if (world->isShowingInvisible())
            world->hideInvisibleObjects();
        else
            world->showInvisibleObjects();
        emit newDirectionsString(" ");
    }
	else if (vrpn_ButtonNum == BUTTON_LEFT(THUMBSTICK_CLICK_IDX))
    {
        resetViewPoint();
    }
}

void ColorEditingMode::doUpdatesForFrame() {
	if(rDist < DISTANCE_THRESHOLD) {
		project->setOutlineObject(RIGHT_SIDE_OUTLINE,rObj);
	}
	else if(springDist < SPRING_DISTANCE_THRESHOLD) {
		project->setOutlineSpring(RIGHT_SIDE_OUTLINE,rSpring,true);
	}
	ObjectGrabMode::doUpdatesForFrame();

	//from spring editing mode, now that we want to color springs
	WorldManager* world = project->getWorldManager();

	if(rDist > DISTANCE_THRESHOLD) {
		if ( world->getNumberOfConnectors() > 0 )
		{
			// get the tracker positions
			q_vec_type rightTrackerPos;
			TransformManager* transformMgr = project->getTransformManager();
			transformMgr->getRightTrackerPosInWorldCoords(rightTrackerPos);

			Connector* closest;
			bool newAtEnd1;
			closest = world->getClosestConnector(rightTrackerPos,&springDist,&newAtEnd1);
			if (closest != rSpring)
			{
				project->setOutlineSpring(RIGHT_SIDE_OUTLINE,closest,newAtEnd1);
				rSpring = closest;
			}
			if (springDist < SPRING_DISTANCE_THRESHOLD)
			{
				if (!project->isOutlineVisible(RIGHT_SIDE_OUTLINE))
				{
					project->setOutlineVisible(RIGHT_SIDE_OUTLINE,true);
				}
			}
			else if (project->isOutlineVisible(RIGHT_SIDE_OUTLINE))
			{
				project->setOutlineVisible(RIGHT_SIDE_OUTLINE,false);
			}
		}
		else
		{
			if (project->isOutlineVisible(LEFT_SIDE_OUTLINE))
				project->setOutlineVisible(LEFT_SIDE_OUTLINE,false);
			if (project->isOutlineVisible(RIGHT_SIDE_OUTLINE))
				project->setOutlineVisible(RIGHT_SIDE_OUTLINE,false);
		}
	}
	useLeftJoystickToRotateViewPoint();
}

void ColorEditingMode::analogsUpdated()
{
    ObjectGrabMode::analogsUpdated();
    useLeftJoystickToRotateViewPoint();
}
