/*
 * This file is a test for the vtkVRMLWriter class
 *
 */

#include "vtkVRMLWriter.h"
#include <vtkSmartPointer.h>
#include <vtkCubeSource.h>
#include <vtkArrayCalculator.h>


int writeVRML()
{
  vtkSmartPointer< vtkCubeSource > cube =
    vtkSmartPointer< vtkCubeSource >::New();
  cube->SetBounds(-1,1,-2,3,-.5,7);
  cube->Update();
  vtkSmartPointer< vtkArrayCalculator > calc =
    vtkSmartPointer< vtkArrayCalculator >::New();
  calc->SetInputConnection(cube->GetOutputPort());
  calc->AddCoordinateScalarVariable("x",0);
  calc->SetResultArrayName("X");
  calc->SetFunction("x");
  calc->Update();
  vtkSmartPointer< vtkVRMLWriter > writer =
    vtkSmartPointer< vtkVRMLWriter >::New();
  writer->SetInputConnection(calc->GetOutputPort());
  writer->SetFileName("test.wrl");
  writer->Write();
  return 0;
}

int main()
{
    return writeVRML();
}
