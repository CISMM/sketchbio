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
#include <vector>


class ModelManager
{
public:
    ModelManager();
    ~ModelManager();
    /*****************************************************************************
      *
      * Adds a new Algorithm to use as a source in vtk.
      * Returns an index to this source to use with addObjectType.
      *
      * Note: addObjectType initializes a transformation the model with scaling
      * and must be called before it is available from the getPolyDataOutput and
      * getOutputPort methods
      *
      * dataSource - the new source to use
      *
      ****************************************************************************/
    int addObjectSource(vtkPolyDataAlgorithm *dataSource);
    /*****************************************************************************
      *
      * Adds an instance of the identified model at scale factor scale.
      * Returns the index of the scaled model in the usable models list
      *
      * srcIndex - the model ID, returned by addObjectSource
      * scale    - the scale factor to scale the object by
      *
      ****************************************************************************/
    int addObjectType(int srcIndex, double scale);
    /*****************************************************************************
      *
      * This method gets the vtkPolyData model which is scaled and recentered
      * and identified by transformIdx
      *
      * modelIdx is assumed to be an index returned by addObjectType
      *
      ****************************************************************************/
    vtkPolyData *getPolyDataOutput(int modelIdx);
    /*****************************************************************************
      *
      * This method gets the output port of the scaled and recentered model
      * which has the index transformIdx.
      *
      * modelIdx is assumed to be an index returned by addObjectType
      *
      ****************************************************************************/
    vtkAlgorithmOutput *getOutputPort(int modelIdx);
    /*****************************************************************************
      *
      * This method gets the entire stored data about the model at the given index
      * this includes the vtkTransformPolyDataFilter, mass, moment of inertia
      * and (eventually) collision detection information
      *
      * modelIdx is assumed to be an index returned by addObjectType
      *
      ****************************************************************************/
    SketchModel *getModelFor(int modelIndex);

private:
    // no source collection... so fun with casting
    // a collection of vtkPolyDataAlgorithm indexed by the order they are added
    // they should not be removed.
    vtkSmartPointer<vtkCollection> sources;

    std::vector<SketchModel *> models;
};

#endif // MODELMANAGER_H
