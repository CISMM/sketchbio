/*
 * This file is a test for the vtkVRMLWriter class
 *
 */

#include "vtkVRMLWriter.h"
#include <vtkSmartPointer.h>
#include <vtkCubeSource.h>
#include <vtkArrayCalculator.h>
#include <vtkColorTransferFunction.h>


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
  vtkSmartPointer< vtkColorTransferFunction > colors =
    vtkSmartPointer< vtkColorTransferFunction >::New();
  colors->AddRGBPoint(-1.0,1.0,0.0,0.0);
  colors->AddRGBPoint( 1.0,0.0,0.0,1.0);
  vtkSmartPointer< vtkVRMLWriter > writer =
    vtkSmartPointer< vtkVRMLWriter >::New();
  writer->SetInputConnection(calc->GetOutputPort());
  writer->SetFileName("test.wrl");
  writer->SetArrayToColorBy("X");
  writer->SetColorMap(colors);
  writer->Write();
  int err = (int) writer->GetErrorCode();
  // TODO - this currently tests if the file is written cleanly.  There is
  // no test to see if the data is correct, I can't figure out how to do it
  // with only VTK and standard stuff without writing an equivalent reader...
  return err;
}

int main()
{
    return writeVRML();
}
