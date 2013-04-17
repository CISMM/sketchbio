#ifndef PROJECTTOBLENDERANIMATION_H
#define PROJECTTOBLENDERANIMATION_H

#include <QHash>

class QFile;
class SketchProject;
class SketchModel;
class ModelManager;
class SketchObject;
class WorldManager;

/*
 * This class handles writing a python file to export the project to blender
 */
class ProjectToBlenderAnimation
{
public:
    // returns true for success, false for failure
    static bool writeProjectBlenderFile(QFile &file, const SketchProject *proj);
private:
    // Writes some python helper function definitions to the file
    static bool writeHelperFunctions(QFile &file);
    // Writes code to create a base object to copy for each model and assigns them to the list modelObjects
    // with indices that populate the given QHash so that a model's value in the map is its base object's
    // index in the list
    static bool writeCreateModels(QFile &file, QHash<SketchModel *, int> &modelIdxs, const ModelManager *models);
    // Writes code to create an object in Blender for each SketchObject (for now assumes no groups)
    // stores the index of each object in the myObjects list in the objectIdxs QHash
    static bool writeCreateObjects(QFile &file, QHash<SketchModel *, int> &modelIdxs,
                                   QHash<SketchObject *, int> &objectIdxs, const WorldManager *world);
};

#endif // PROJECTTOBLENDERANIMATION_H
