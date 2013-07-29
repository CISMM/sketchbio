#include "vtkGiftRibbonSource.h"

#include <cmath>

#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"

vtkStandardNewMacro(vtkGiftRibbonSource);

vtkGiftRibbonSource::vtkGiftRibbonSource(double xL, double yL, double zL)
{
    using std::abs;
    this->XLength = abs(xL);
    this->YLength = abs(yL);
    this->ZLength = abs(zL);

    this->Center[0] = 0.0;
    this->Center[1] = 0.0;
    this->Center[2] = 0.0;

    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
}

int vtkGiftRibbonSource::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    vtkPolyData *output = vtkPolyData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

    double x[3];
    int numLines = 12, numPts = 12;
    int i, j;
    vtkIdType pts[2];
    vtkSmartPointer< vtkPoints > newPoints =
      vtkSmartPointer< vtkPoints >::New();
    vtkSmartPointer< vtkCellArray > newLines =
      vtkSmartPointer< vtkCellArray >::New();

    newPoints->Allocate(numPts);
    newLines->Allocate(newLines->EstimateSize(numLines,2));

    for (x[0] = this->Center[0]-this->XLength*0.5, x[2] = this->Center[2], i = 0;
    i<2; i++, x[0]+=this->XLength)
      {
      for (x[1] = this->Center[1]-this->YLength*0.5, j = 0;
      j<2; j++, x[1]+= this->YLength)
        {
          newPoints->InsertNextPoint(x);
        }
      }
    pts[0] = 0; pts[1] = 1;
    newLines->InsertNextCell(2,pts);
    pts[1] = 2;
    newLines->InsertNextCell(2,pts);
    pts[0] = 2; pts[1] = 3;
    newLines->InsertNextCell(2,pts);
    pts[0] = 1;
    newLines->InsertNextCell(2,pts);

    for (x[1] = this->Center[1]-this->YLength*0.5, x[0] = this->Center[0], i = 0;
    i<2; i++, x[1]+=this->YLength)
      {
      for (x[2] = this->Center[2]-this->ZLength*0.5, j = 0;
      j<2; j++, x[2]+= this->ZLength)
        {
          newPoints->InsertNextPoint(x);
        }
      }
    pts[0] = 4; pts[1] = 5;
    newLines->InsertNextCell(2,pts);
    pts[1] = 6;
    newLines->InsertNextCell(2,pts);
    pts[0] = 6; pts[1] = 7;
    newLines->InsertNextCell(2,pts);
    pts[0] = 5;
    newLines->InsertNextCell(2,pts);

    for (x[0] = this->Center[0]-this->XLength*0.5, x[1] = this->Center[1], i = 0;
    i<2; i++, x[0]+=this->XLength)
      {
      for (x[2] = this->Center[2]-this->ZLength*0.5, j = 0;
      j<2; j++, x[2]+= this->ZLength)
        {
          newPoints->InsertNextPoint(x);
        }
      }
    pts[0] = 8; pts[1] = 9;
    newLines->InsertNextCell(2,pts);
    pts[1] = 10;
    newLines->InsertNextCell(2,pts);
    pts[0] = 10; pts[1] = 11;
    newLines->InsertNextCell(2,pts);
    pts[0] = 9;
    newLines->InsertNextCell(2,pts);

    output->SetPoints(newPoints);

    newLines->Squeeze();
    output->SetLines(newLines);

    return 1;
}

void vtkGiftRibbonSource::SetBounds(double xMin, double xMax,
                                    double yMin, double yMax,
                                    double zMin, double zMax)
{
  double bounds[6];
  bounds[0] = xMin;
  bounds[1] = xMax;
  bounds[2] = yMin;
  bounds[3] = yMax;
  bounds[4] = zMin;
  bounds[5] = zMax;
  this->SetBounds (bounds);
}

void vtkGiftRibbonSource::SetBounds(double bounds[])
{
  this->SetXLength(bounds[1] - bounds[0]);
  this->SetYLength(bounds[3] - bounds[2]);
  this->SetZLength(bounds[5] - bounds[4]);

  this->SetCenter((bounds[0] + bounds[1])*0.5,
                  (bounds[2] + bounds[3])*0.5,
                  (bounds[4] + bounds[5])*0.5);
}

void vtkGiftRibbonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "X Length: " << this->XLength << "\n";
  os << indent << "Y Length: " << this->YLength << "\n";
  os << indent << "Z Length: " << this->ZLength << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
               << this->Center[1] << ", " << this->Center[2] << ")\n";
}
