#ifndef SKETCHMODEL_H
#define SKETCHMODEL_H

#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <PQP.h>

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
