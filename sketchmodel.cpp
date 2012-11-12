#include "sketchmodel.h"

SketchModel::SketchModel(vtkTransformPolyDataFilter *data, double iMass, double iMoment)
{
    modelData = data;
    collisionModel = new PQP_Model();
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(data->GetOutputPort());
    invMass = iMass;
    invMomentOfInertia = iMoment;
}

SketchModel::~SketchModel()
{
    delete collisionModel;
}
