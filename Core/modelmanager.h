#ifndef MODELMANAGER_H
#define MODELMANAGER_H

/*
 *
 * This class was originally written for a homework assignment in Physically Based Modeling (COMP768)
 * Author: Shawn Waldon
 *
 * This class stores all the models that are currently read in/created in a hash so that they can be
 * reused easily without having the user keep track of indices.  The hash key is a QString which is
 * the filename for models read from a file and for vtk-source based models it gives some indication
 * of what the source is.
 *
 */


class QDir;
class QString;
#include <QVector>
#include <QHash>

class SketchModel;
class vtkPolyDataAlgorithm;



#define CAMERA_MODEL_KEY "CAMERA"


class ModelManager
{
public:
    ModelManager();
    virtual ~ModelManager();

    /*****************************************************************************
      *
      * This method returns the model that is currently being used for the camera
      * objects that the user can place and move.  This will probably be something
      * created from vtk sources, although that is not necessary.  This model is
      * guaranteed to have the key CAMERA_MODEL_KEY in the models hash.
      *
      * Notes: cameras are defined as being located at the object's origin and pointing
      * in the object's internal +z direction
      *
      ****************************************************************************/
    SketchModel *getCameraModel(QDir &projectDir);

    /*****************************************************************************
      *
      * This method creates a SketchModel from the given vtk source using the given
      * scale factor.  If there is already a model with the same source name, it simply
      * returns that one, otherwise it creates a new model (based on a vtk file in
      * the given directory)
      *
      * sourceName - the string name describing the source
      * source     - the VTK source that should be used to generate the geometry for
      *             the model
      * scale      - the scale at which the source should be interpreted
      * dir        - the directory in which to place the VTK file this creates for the model
      *
      ****************************************************************************/
    SketchModel *modelForVTKSource(const QString &sourceName,
                                   vtkPolyDataAlgorithm *source,
                                   double scale,
                                   QDir &dir);
    /*****************************************************************************
      *
      * This method creates a SketchModel from the given filename and source name,
      * unless there is already a model with the given source name, in which case it
      * returns that one.  The iMass and iMoment parameter set the inverse mass and
      * moment of inertia for the model.
      *
      ****************************************************************************/
    SketchModel *makeModel(const QString &source, const QString &filename,
                           double iMass, double iMoment);

    /*****************************************************************************
      *
      * Finds the model with the given source and adds to it as an alternate
      * conformation the given new source and data filename.
      *
      ****************************************************************************/
    SketchModel *addConformation(const QString &originalSource,
                                 const QString &newSource,
                                 const QString &newFilename);
    /*****************************************************************************
      *
      * Adds the given new source and data file to the given model as an alternate
      * molecule conformation
      *
      ****************************************************************************/
    SketchModel *addConformation(SketchModel *model,
                                 const QString &newSource,
                                 const QString &newFilename);
    /*****************************************************************************
      *
      * This method adds the model to the ModelManger, unless the model manager has
      * a model that has the exact same sources in the same order, in which case the
      * old one is used.
      *
      ****************************************************************************/
    SketchModel *addModel(SketchModel *model);
    /*****************************************************************************
      *
      * This method returns an iterator that may be used to examine each of the
      * models in the ModelManager.
      *
      ****************************************************************************/
    QVectorIterator<SketchModel *> getModelIterator() const;
    /*****************************************************************************
      *
      * Returns true if the model manager has a model with the given source
      *
      ****************************************************************************/
    bool hasModel(const QString &source) const;
    /*****************************************************************************
      *
      * Returns the model with the given source or NULL
      *
      ****************************************************************************/
    SketchModel *getModel(const QString &source) const;
    /*****************************************************************************
      *
      * This method returns the number of models in the model manager
      *
      ****************************************************************************/
    int getNumberOfModels() const;

private:
    // Disable copy constructor and assignment operator these are not implemented
    // and not supported
    ModelManager(const ModelManager &other);
    ModelManager &operator=(const ModelManager &other);

    // a hash of source to model
    QVector<SketchModel *> models;
    QHash<QString,int> modelSourceToIdx;
};


#endif // MODELMANAGER_H
