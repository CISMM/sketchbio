#include <iostream>

#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkGiftRibbonSource.h"
#include "vtkSmartPointer.h"
#include "vtkPolyDataWriter.h"

int main()
{
  vtkSmartPointer< vtkGiftRibbonSource > source =
    vtkSmartPointer< vtkGiftRibbonSource >::New();
  source->SetBounds(0.0,5.0,-4.5,2.7,-1.3333333,2.79);
  source->Update();
  vtkSmartPointer< vtkPolyDataWriter > writer =
    vtkSmartPointer< vtkPolyDataWriter >::New();
  writer->SetInputConnection(source->GetOutputPort());
  writer->SetFileName("testRibbon.vtk");
  writer->SetFileTypeToASCII();
  writer->Write();
  vtkPolyData *data = source->GetOutput();
  vtkPoints *points = data->GetPoints();
  if (points->GetNumberOfPoints() != 12)
  {
      std::cout << "Number of points is wrong." << std::endl;
      return 1;
  }
  if (data->GetNumberOfLines() != 12)
  {
      std::cout << "Number of lines is wrong." << std::endl;
      return 1;
  }
  // TODO: I'm not really sure what else to test other than
  // going through point by point and line by line to test
  // correct positions... and I don't want to do that.
  return 0;
}
