#include "coloreditingmode.h"

#include <sketchioconstants.h>
#include <sketchobject.h>
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
}

void ColorEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD) {
            SketchObject::ColorMapType::Type cmap = rObj->getColorMapType();
            switch (cmap)
            {
            case SketchObject::ColorMapType::SOLID_COLOR_RED:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_GREEN;
                break;
            case SketchObject::ColorMapType::SOLID_COLOR_GREEN:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_BLUE;
                break;
            case SketchObject::ColorMapType::SOLID_COLOR_BLUE:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_YELLOW;
                break;
            case SketchObject::ColorMapType::SOLID_COLOR_YELLOW:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_PURPLE;
                break;
            case SketchObject::ColorMapType::SOLID_COLOR_PURPLE:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_CYAN;
                break;
            case SketchObject::ColorMapType::SOLID_COLOR_CYAN:
                cmap = SketchObject::ColorMapType::BLUE_TO_RED;
                break;
            case SketchObject::ColorMapType::BLUE_TO_RED:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_RED;
                break;
            default:
                cmap = SketchObject::ColorMapType::SOLID_COLOR_RED;
                break;
            }
            rObj->setColorMapType(cmap);
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
                arr = QString("modelNum");
            }
            rObj->setArrayToColorBy(arr.toStdString().c_str());
        }
        emit newDirectionsString(" ");
    }
}

void ColorEditingMode::analogsUpdated()
{
    ObjectGrabMode::analogsUpdated();
    useLeftJoystickToRotateViewPoint();
}
