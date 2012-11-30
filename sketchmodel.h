#ifndef SKETCHMODEL_H
#define SKETCHMODEL_H

#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <PQP.h>

/*
 * This class holds data about a general type of object such as a fibrin molecule.  The vtk
 * data that defines the object's shape as well as its collision detection model are here.
 * Multiple objects may share one instance of this class (for now, deformations may change this).
 *
 * Since all objects (molecules) of a given type will have the same mass, mass data is stored here
 * as well.
 */

class SketchModel
{
public:
    SketchModel(vtkTransformPolyDataFilter *data, double iMass, double iMoment);
    ~SketchModel();
    inline vtkTransformPolyDataFilter *getModelData() { return modelData; }
    inline vtkPolyDataMapper *getMapper() { return mapper; }
    inline PQP_Model *getCollisionModel() { return collisionModel; }
    inline double getInverseMass() const { return invMass;}
    inline double getInverseMomentOfInertia() const { return invMomentOfInertia; }
private:
    vtkSmartPointer<vtkTransformPolyDataFilter> modelData;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    PQP_Model *collisionModel;
    double invMass;
    double invMomentOfInertia;
};

#endif // SKETCHMODEL_H
