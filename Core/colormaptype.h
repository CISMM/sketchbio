#ifndef COLORMAPTYPE_H_
#define COLORMAPTYPE_H_

class QString;

class vtkColorTransferFunction;
/*
 * This namespace contains an enum that holds the color map types that are available,
 * and some helper functions that convert them to/from a string and to a
 * vtkColorTransferFunction
 *
 */
namespace ColorMapType {
    enum Type {
        SOLID_COLOR_RED,
        SOLID_COLOR_GREEN,
        SOLID_COLOR_BLUE,
        SOLID_COLOR_YELLOW,
        SOLID_COLOR_PURPLE,
        SOLID_COLOR_CYAN,
		SOLID_COLOR_GRAY,
        DIM_SOLID_COLOR_RED,
        DIM_SOLID_COLOR_GREEN,
        DIM_SOLID_COLOR_BLUE,
        DIM_SOLID_COLOR_YELLOW,
        DIM_SOLID_COLOR_PURPLE,
        DIM_SOLID_COLOR_CYAN,
        BLUE_TO_RED
    };
    // gets the color map corresponding to one of the strings
    // returned by stringFromColorMap
    Type colorMapFromString(const char *str);
    // gets a string representation of the color map
    const char *stringFromColorMap(Type cmap);
    // This function takes a color map type and constructs the color map as
    // a vtkColorTransferFunction.  The returned vtkColorTransferFunction should
    // recieved with vtkSmartPointer<...>::Take()
    // low and high are the lowest and highest values in the interval that the
    // color map should map over.
    vtkColorTransferFunction *getColorMap(ColorMapType::Type cmapType,
                                             double low, double high);

    // Returns true if the color map listed has a solid color for the entire object
    // and false if it requires per-vertex coloring
    bool isSolidColor(ColorMapType::Type cmapType, const QString& arrayName);

}


#endif
