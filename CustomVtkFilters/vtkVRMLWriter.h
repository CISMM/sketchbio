#ifndef VTKVRMLWRITER_H
#define VTKVRMLWRITER_H

#include "vtkWriter.h"
class vtkColorTransferFunction;
class vtkPolyData;
class ostream;

class vtkVRMLWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkVRMLWriter, vtkWriter);
  void PrintSelf(ostream &os, vtkIndent indent);

  static vtkVRMLWriter *New();

  virtual void SetColorMap(vtkColorTransferFunction*);
  vtkGetMacro(ColorMap,vtkColorTransferFunction *);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetStringMacro(ArrayToColorBy);
  vtkGetStringMacro(ArrayToColorBy);

protected:
  vtkVRMLWriter();
  virtual ~vtkVRMLWriter();

  bool OpenStream();

  virtual void WriteData();
  virtual void WritePolyData(vtkPolyData *data);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkColorTransferFunction *ColorMap;
  char *FileName;
  char *ArrayToColorBy;
  ostream *Stream;

private:
  vtkVRMLWriter(const vtkVRMLWriter &); // Not implemented
  void operator =(const vtkVRMLWriter &); // Not implemented
};

#endif // VTKVRMLWRITER_H
