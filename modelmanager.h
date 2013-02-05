#ifndef MODELMANAGER_H
#define MODELMANAGER_H

/*
 *
 * This class was originally written for a homework assignment in Physically Based Modeling (COMP768)
 * Author: Shawn Waldon
 *
 * PolyMananger is a class to deal with keeping models in memory at any given time.  Each model is
 * stored as an index into an internal collection and the model data can be gotten out via this index.
 * The collision detection model for each model can also be accessed by this same index.
 *
 */


#include <vtkSmartPointer.h>
#include <vtkCollection.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkAlgorithmOutput.h>
#include "sketchmodel.h"
#include <PQP.h>
#include <QString>
#include <QHash>


#define INVERSEMASS 1.0
#define INVERSEMOMENT (1.0/25000)

class ModelManager
{
public:
    ModelManager();
    ~ModelManager();

    /*****************************************************************************
      *
      * This method creates a SketchModel from the given vtk source using the given
      * scale factor.  If there is already a model using a vtk source of the same class name, this
      * method simply returns the old one (ignores scale for now).
      *
      * source  - the VTK source that should be used to generate the geometry for
      *             the model
      * scale   - the scale at which the source should be interpreted
      *
      ****************************************************************************/
    SketchModel *modelForVTKSource(vtkPolyDataAlgorithm *source, double scale = 1.0);
    /*****************************************************************************
      *
      * This method creates a SketchModel from the given obj file using the given
      * scale factor.  If there is already a model using the given OBJ file, this
      * method simply returns the old one (ignores scale for now).
      *
      * objFile - the filename of the obj file (should be absolute path, but in
      *             the project folder
      * scale   - the scale at which the obj should be interpreted
      * iMass   - 1/mass where mass is the mass of an object of this type
      * iMoment - 1/moment where moment is the (approximate) moment of inertia (pick an axis)
      *             for an object of this type
      *
      ****************************************************************************/
    SketchModel *modelForOBJSource(QString objFile, double iMass = INVERSEMASS,
                                   double iMoment = INVERSEMOMENT, double scale = 1.0);
    /*****************************************************************************
      *
      * This method returns an iterator that may be used to examine each of the
      * models in the ModelManager.
      *
      ****************************************************************************/
    QHashIterator<QString,SketchModel *> getModelIterator() const;
    /*****************************************************************************
      *
      * This method returns the number of models in the model manager
      *
      ****************************************************************************/
    int getNumberOfModels() const;

private:
    // a hash of source to model
    QHash<QString,SketchModel *> models;
};
void makePQP_Model(PQP_Model &m1, vtkPolyData &polyData);

#ifdef PQP_UPDATE_EPSILON
void updatePQP_Model(PQP_Model &model,vtkPolyData &polyData);
#endif

#endif // MODELMANAGER_H
