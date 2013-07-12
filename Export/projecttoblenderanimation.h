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
private:
    // Writes some python helper function definitions to the file
    static bool writeHelperFunctions(QFile &file);
    // Writes code to create a base object to copy for the model and adds it to the list modelObjects
    // with indices that populate the given QHash so that a model's value in the map is its base object's
    // index in the list
    static void writeCreateModel(QFile &file,
                                 QHash< QPair< SketchModel *, int >, int> &modelIdx,
                                 SketchModel *model,
                                 int conformation);
    // Writes code to create an object in Blender for each SketchObject (for now assumes no groups)
    // stores the index of each object in the myObjects list in the objectIdxs QHash
    static bool writeCreateObjects(QFile &file, QHash< QPair< SketchModel *, int >, int> &modelIdxs,
                                   QHash<SketchObject *, int> &objectIdxs, SketchProject *proj);
    // Writes code to create a single object and recurse on groups (only objects with the an individual model
    // are keyframed, groups are ignored and each object within them is added)
    static void writeCreateObject(QFile &file, QHash< QPair< SketchModel *, int >, int> &modelIdxs,
                                  QHash<SketchObject *, int> &objectIdxs, int &objectsLen, SketchProject *proj,
                                  SketchObject *obj);
    // Writes a keyframe for each object at each frame with its position at that time from SketchBio
    static bool writeObjectKeyframes(QFile &file, QHash<SketchObject *, int> &objectIdxs, SketchProject *proj,
                                     unsigned frameRate = BLENDER_RENDERER_FRAMERATE);
    // Creates a VRML file for the given vtk file so that the data can be used in Blender
    // TODO - eventually this will take an array to color by and possibly a color map
    static QString generateVRMLFileFor(QString vtkFile, char const *arrayName = "modelNum",
                                       vtkColorTransferFunction *colorMap = NULL);
};

#endif // PROJECTTOBLENDERANIMATION_H
