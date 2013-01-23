#include "sketchmodel.h"
#include <vtkExtractEdges.h>

SketchModel::SketchModel(QString source, vtkTransformPolyDataFilter *data, double iMass, double iMoment) :
    dataSource(source),
    modelData(data),
    collisionModel(new PQP_Model()),
    invMass(iMass),
    invMomentOfInertia(iMoment)
{
    vtkSmartPointer<vtkExtractEdges> wireFrame = vtkSmartPointer<vtkExtractEdges>::New();
    wireFrame->SetInputConnection(data->GetOutputPort());
    wireFrame->Update();
    normalMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    normalMapper->SetInputConnection(data->GetOutputPort());
    normalMapper->Update();
    wireFrameMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    wireFrameMapper->SetInputConnection(wireFrame->GetOutputPort());
    wireFrameMapper->Update();
}

SketchModel::~SketchModel()
{
    delete collisionModel;
}
