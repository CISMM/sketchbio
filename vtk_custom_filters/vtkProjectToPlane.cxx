#include "vtkProjectToPlane.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkDataObject.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

vtkStandardNewMacro(vtkProjectToPlane);

vtkProjectToPlane::vtkProjectToPlane()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->PointOnPlane[0] = 0.0;
  this->PointOnPlane[1] = 0.0;
  this->PointOnPlane[2] = 0.0;
  this->PlaneNormalVector[0] = 0.0;
  this->PlaneNormalVector[1] = 0.0;
  this->PlaneNormalVector[2] = 0.0;
}

vtkProjectToPlane::~vtkProjectToPlane()
{

}

int vtkProjectToPlane::RequestData(vtkInformation *vtkNotUsed(request),
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);


  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData *output = vtkPolyData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPoints = input->GetPoints();
  int numPts = inPoints->GetNumberOfPoints();
  vtkSmartPointer<vtkPoints> newPts =
      vtkSmartPointer<vtkPoints>::New();
  newPts->SetNumberOfPoints(numPts);

  double normalMagnitudeSquared =
    this->PlaneNormalVector[0] * this->PlaneNormalVector[0] +
    this->PlaneNormalVector[1] * this->PlaneNormalVector[1] +
    this->PlaneNormalVector[2] * this->PlaneNormalVector[2];
  // test if the normal vector has 0 magnitude... if so, this filter "fails"
  // to avoid division by 0
  if (normalMagnitudeSquared < 1e-50)
    {
    this->SetErrorCode(23); // copying the failed filter example... is 23 special?
    output->ShallowCopy(input);
    return 1;
    }

  for (vtkIdType i = 0; i < numPts; i++)
    {
    double inPoint[3], planeToPoint[3], projection[3],outPoint[3];
    inPoints->GetPoint(i);

    // compute the vector between the point on the plane and the point
    planeToPoint[0] = inPoint[0] - this->PointOnPlane[0];
    planeToPoint[1] = inPoint[1] - this->PointOnPlane[1];
    planeToPoint[2] = inPoint[2] - this->PointOnPlane[2];

    // project the computed vector to the normal vector of the plane
    double scalarProjection =
      (planeToPoint[0] * this->PlaneNormalVector[0] +
      planeToPoint[1] * this->PlaneNormalVector[1] +
      planeToPoint[2] * this->PlaneNormalVector[2]) /
      normalMagnitudeSquared;

    projection[0] = this->PlaneNormalVector[0] * scalarProjection;
    projection[1] = this->PlaneNormalVector[1] * scalarProjection;
    projection[2] = this->PlaneNormalVector[2] * scalarProjection;

    // subtract the projection from the original point
    outPoint[0] = inPoint[0] - projection[0];
    outPoint[1] = inPoint[1] - projection[1];
    outPoint[2] = inPoint[2] - projection[2];

    newPts->SetPoint(i,outPoint);

    }

  output->CopyStructure(input);
  output->SetPoints(newPts);

  return 1;
}

void vtkProjectToPlane::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Point on Plane: [ " << this->PointOnPlane[0] <<
    ", " << this->PointOnPlane[1] << ", " << this->PointOnPlane[2] << "]\n";
  os << indent << "Plane Normal Vector: [ " << this->PlaneNormalVector[0] <<
    ", " << this->PlaneNormalVector[1] << ", " << this->PlaneNormalVector[2] << "]\n";
}
