#include "sketchmodel.h"
#include <vtkExtractEdges.h>

SketchModel::SketchModel(vtkTransformPolyDataFilter *data, double iMass, double iMoment)
{
    modelData = data;
    collisionModel = new PQP_Model();
    vtkSmartPointer<vtkExtractEdges> wireFrame = vtkSmartPointer<vtkExtractEdges>::New();
    wireFrame->SetInputConnection(data->GetOutputPort());
    wireFrame->Update();
    normalMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    normalMapper->SetInputConnection(data->GetOutputPort());
    normalMapper->Update();
    wireFrameMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    wireFrameMapper->SetInputConnection(wireFrame->GetOutputPort());
    wireFrameMapper->Update();
    invMass = iMass;
    invMomentOfInertia = iMoment;
}

SketchModel::~SketchModel()
{
    delete collisionModel;
}
