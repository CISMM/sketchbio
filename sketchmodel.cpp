#include "sketchmodel.h"

SketchModel::SketchModel(vtkTransformPolyDataFilter *data, double iMass, double iMoment)
{
    modelData = data;
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(data->GetOutputPort());
    invMass = iMass;
    invMomentOfInertia = iMoment;
}

SketchModel::~SketchModel()
{
    // nothing yet, but once collision model is added, who knows
}
