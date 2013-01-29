#include "sketchmodel.h"
#include <vtkExtractEdges.h>
#include <vtkTransform.h>

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

double SketchModel::getScale() const {
    vtkSmartPointer<vtkTransform> tf = ((vtkTransform*)modelData->GetTransform());
    return tf->GetScale()[0];
}

void SketchModel::getTranslate(double out[]) const {
    ((vtkTransform*)modelData->GetTransform())->GetPosition(out);
}
