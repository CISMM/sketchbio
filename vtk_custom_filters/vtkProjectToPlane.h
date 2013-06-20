#ifndef __vtkProjectToPlane_h
#define __vtkProjectToPlane_h

#include "vtkPolyDataAlgorithm.h"

class vtkProjectToPlane : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkProjectToPlane,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkProjectToPlane *New();

  vtkSetVector3Macro(PointOnPlane,double);
  vtkGetVector3Macro(PointOnPlane,double);
  vtkSetVector3Macro(PlaneNormalVector,double);
  vtkGetVector3Macro(PlaneNormalVector,double);

protected:
  vtkProjectToPlane();
  ~vtkProjectToPlane();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkProjectToPlane(const vtkProjectToPlane&);  // Not implemented.
  void operator=(const vtkProjectToPlane&);  // Not implemented.

  double PointOnPlane[3];
  double PlaneNormalVector[3];

};

#endif
