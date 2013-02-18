#include "sketchmodel.h"
#include <vtkExtractEdges.h>
#include <vtkTransform.h>

SketchModel::SketchModel(QString source, vtkTransformPolyDataFilter *data, double iMass, double iMoment) :
    dataSource(source),
    modelData(data),
    collisionModel(new PQP_Model()),
#ifndef PQP_UPDATE_EPSILON
    triIdToTriIndex(),
#endif
    invMass(iMass),
    invMomentOfInertia(iMoment)
{}

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
