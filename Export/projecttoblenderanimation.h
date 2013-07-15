#ifndef PROJECTTOBLENDERANIMATION_H
#define PROJECTTOBLENDERANIMATION_H

#include <QHash>
class QFile;

class SketchProject;
class SketchModel;
class ModelManager;
class SketchObject;
class WorldManager;

class vtkColorTransferFunction;

#define BLENDER_RENDERER_FRAMERATE 30

/*
 * This class handles writing a python file to export the project to blender
 */
class ProjectToBlenderAnimation
{
public:
    // returns true for success, false for failure
    static bool writeProjectBlenderFile(QFile &file, SketchProject *proj);
    // converts time in seconds to a blender frame number (what Blender puts keyframes on)
    // using the frame rate.  The default for frame rate is Blender's default frame rate.
    static unsigned timeToBlenderFrameNum(double time, unsigned frameRate = BLENDER_RENDERER_FRAMERATE);
    // Creates a VRML file for the given vtk file so that the data can be used in Blender
    // the data is written with colors from the given color map according to the given array
    static QString generateVRMLFileFor(QString vtkFile, char const *arrayName = "modelNum",
                                       vtkColorTransferFunction *colorMap = NULL);
private:
    // Writes some python helper function definitions to the file
    static bool writeHelperFunctions(QFile &file);
    // Writes code to create an object in Blender for each SketchObject (for now assumes no groups)
    // stores the index of each object in the myObjects list in the objectIdxs QHash
    static bool writeCreateObjects(QFile &file,
                                   QHash<SketchObject *, int> &objectIdxs, SketchProject *proj);
    // Writes a keyframe for each object at each frame with its position at that time from SketchBio
    static bool writeObjectKeyframes(QFile &file, QHash<SketchObject *, int> &objectIdxs, SketchProject *proj,
                                     unsigned frameRate = BLENDER_RENDERER_FRAMERATE);
};

#endif // PROJECTTOBLENDERANIMATION_H
