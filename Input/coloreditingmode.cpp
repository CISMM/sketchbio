#include "coloreditingmode.h"

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>

ColorEditingMode::ColorEditingMode(SketchProject *proj, const bool * const b,
                                   const double * const a)
    : ObjectGrabMode(proj,b,a)
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
}

void ColorEditingMode::analogsUpdated()
{
    ObjectGrabMode::analogsUpdated();
    useLeftJoystickToRotateViewPoint();
}
