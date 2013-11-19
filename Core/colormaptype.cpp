#include "colormaptype.h"

#include <QString>

#include <vtkNew.h>
#include <vtkColorTransferFunction.h>

namespace ColorMapType
{

const char *stringFromColorMap(
        Type cmap)
{
    switch (cmap)
    {
    case SOLID_COLOR_RED:
        return "solid_red";
    case SOLID_COLOR_GREEN:
        return "solid_green";
    case SOLID_COLOR_BLUE:
        return "solid_blue";
    case SOLID_COLOR_YELLOW:
        return "solid_yellow";
    case SOLID_COLOR_PURPLE:
        return "solid_purple";
    case SOLID_COLOR_CYAN:
        return "solid_cyan";
	case SOLID_COLOR_GRAY:
        return "solid_gray";
    case DIM_SOLID_COLOR_RED:
        return "dim_red";
    case DIM_SOLID_COLOR_GREEN:
        return "dim_green";
    case DIM_SOLID_COLOR_BLUE:
        return "dim_blue";
    case DIM_SOLID_COLOR_YELLOW:
        return "dim_yellow";
    case DIM_SOLID_COLOR_PURPLE:
        return "dim_purple";
    case DIM_SOLID_COLOR_CYAN:
        return "dim_cyan";
    case BLUE_TO_RED:
        return "blue_to_red";
    default:
        throw "No string for color map: " + QString::number(cmap);
    }
}

Type colorMapFromString(const char *str)
{
    QString s(str);
    if (s == "solid_red")
        return SOLID_COLOR_RED;
    if (s ==  "solid_green")
        return SOLID_COLOR_GREEN;
    if (s ==  "solid_blue")
        return SOLID_COLOR_BLUE;
    if (s ==  "solid_yellow")
        return SOLID_COLOR_YELLOW;
    if (s ==  "solid_purple")
        return SOLID_COLOR_PURPLE;
    if (s ==  "solid_cyan")
        return SOLID_COLOR_CYAN;
	if (s ==  "solid_gray")
        return SOLID_COLOR_GRAY;
    if (s ==  "dim_red")
        return DIM_SOLID_COLOR_RED;
    if (s ==  "dim_green")
        return DIM_SOLID_COLOR_GREEN;
    if (s ==  "dim_blue")
        return DIM_SOLID_COLOR_BLUE;
    if (s ==  "dim_yellow")
        return DIM_SOLID_COLOR_YELLOW;
    if (s ==  "dim_purple")
        return DIM_SOLID_COLOR_PURPLE;
    if (s ==  "dim_cyan")
        return DIM_SOLID_COLOR_CYAN;
    if (s ==  "blue_to_red")
        return BLUE_TO_RED;
    throw "Unknown color map.";
}

vtkColorTransferFunction *getColorMap(
        Type cmapType,
        double low,
        double high)
{
    vtkColorTransferFunction *ctf = vtkColorTransferFunction::New();
    ctf->SetScaleToLinear();
    switch (cmapType)
    {
    case SOLID_COLOR_RED:
        ctf->AddRGBPoint(low,1.0,0.7,0.7);
        break;
    case SOLID_COLOR_GREEN:
        ctf->AddRGBPoint(low,0.7,1.0,0.8);
        break;
    case SOLID_COLOR_BLUE:
        ctf->AddRGBPoint(low,0.7,0.7,1.0);
        break;
    case SOLID_COLOR_YELLOW:
        ctf->AddRGBPoint(low,1.0,1.0,0.7);
        break;
    case SOLID_COLOR_PURPLE:
        ctf->AddRGBPoint(low,1.0,0.7,1.0);
        break;
    case SOLID_COLOR_CYAN:
        ctf->AddRGBPoint(low,0.7,1.0,1.0);
        break;
	case SOLID_COLOR_GRAY:
        ctf->AddRGBPoint(low,0.8,0.8,0.8);
        break;
    case DIM_SOLID_COLOR_RED:
        ctf->AddRGBPoint(low,0.5,0.35,0.35);
        break;
    case DIM_SOLID_COLOR_GREEN:
        ctf->AddRGBPoint(low,0.35,0.5,0.4);
        break;
    case DIM_SOLID_COLOR_BLUE:
        ctf->AddRGBPoint(low,0.35,0.35,0.5);
        break;
    case DIM_SOLID_COLOR_YELLOW:
        ctf->AddRGBPoint(low,0.5,0.5,0.35);
        break;
    case DIM_SOLID_COLOR_PURPLE:
        ctf->AddRGBPoint(low,0.5,0.35,0.5);
        break;
    case DIM_SOLID_COLOR_CYAN:
        ctf->AddRGBPoint(low,0.35,0.5,0.5);
        break;
    case BLUE_TO_RED:
        ctf->SetColorSpaceToDiverging();
        ctf->AddRGBPoint(low,59.0/256.0,76.0/256.0,192.0/256.0);
        ctf->AddRGBPoint(high,180.0/256.0,4.0/256.0,38.0/256.0);
        break;
    }
    return ctf;
}

bool isSolidColor(ColorMapType::Type cMap, const QString& arrayName)
{
    // this array means solid color whatever the color map
    if (arrayName == "modelNum")
    {
        return true;
    }
    // test if the color map is solid color
    switch (cMap)
    {
    case SOLID_COLOR_RED:
    case SOLID_COLOR_GREEN:
    case SOLID_COLOR_BLUE:
    case SOLID_COLOR_YELLOW:
    case SOLID_COLOR_PURPLE:
    case SOLID_COLOR_CYAN:
	case SOLID_COLOR_GRAY:
    case DIM_SOLID_COLOR_RED:
    case DIM_SOLID_COLOR_GREEN:
    case DIM_SOLID_COLOR_BLUE:
    case DIM_SOLID_COLOR_YELLOW:
    case DIM_SOLID_COLOR_PURPLE:
    case DIM_SOLID_COLOR_CYAN:
        return true;
        break;
    case BLUE_TO_RED:
        return false;
        break;
    }
    return false;
}

}
