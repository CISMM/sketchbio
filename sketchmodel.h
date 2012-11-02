#ifndef SKETCHMODEL_H
#define SKETCHMODEL_H

#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>

class SketchModel
{
public:
    SketchModel(vtkTransformPolyDataFilter *data, double iMass, double iMoment);
    ~SketchModel();
    inline vtkTransformPolyDataFilter *getModelData() { return modelData; }
    inline vtkPolyDataMapper *getMapper() { return mapper; }
    inline double getInverseMass() const { return invMass;}
    inline double getInverseMomentOfInertia() const { return invMomentOfInertia; }
private:
    vtkSmartPointer<vtkTransformPolyDataFilter> modelData;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    // collision model here
    double invMass;
    double invMomentOfInertia;
};

#endif // SKETCHMODEL_H
