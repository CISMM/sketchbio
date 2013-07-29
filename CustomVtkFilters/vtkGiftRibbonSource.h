#ifndef VTKGIFTRIBBONSOURCE_H
#define VTKGIFTRIBBONSOURCE_H

#include "vtkPolyDataAlgorithm.h"

class vtkGiftRibbonSource : public vtkPolyDataAlgorithm
{
public:
  static vtkGiftRibbonSource *New();
  vtkTypeMacro(vtkGiftRibbonSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

  vtkSetClampMacro(XLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(XLength,double);

  vtkSetClampMacro(YLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(YLength,double);

  vtkSetClampMacro(ZLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(ZLength,double);

  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  void SetBounds(double xMin, double xMax,
                 double yMin, double yMax,
                 double zMin, double zMax);
  void SetBounds(double bounds[6]);

protected:
  vtkGiftRibbonSource(double xL=1.0, double yL=1.0, double zL=1.0);
  ~vtkGiftRibbonSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double XLength;
  double YLength;
  double ZLength;
  double Center[3];
private:
  vtkGiftRibbonSource(const vtkGiftRibbonSource&); // Not implemented
  void operator=(const vtkGiftRibbonSource&); // Not implemented
};

#endif // VTKGIFTRIBBONSOURCE_H
