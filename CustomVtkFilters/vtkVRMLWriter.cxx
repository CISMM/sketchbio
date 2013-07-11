#include "vtkVRMLWriter.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkDataObject.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkCellArray.h"
#include "vtkColorTransferFunction.h"
#include "vtkErrorCode.h"

#include <iostream>
using std::ostream;

vtkStandardNewMacro(vtkVRMLWriter);

vtkVRMLWriter::vtkVRMLWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
  this->ColorMap = NULL;
  this->FileName = NULL;
  this->ArrayToColorBy = NULL;
  this->Stream = NULL;
}

vtkVRMLWriter::~vtkVRMLWriter()
{
  if (this->ColorMap != NULL)
      ColorMap->UnRegister(this);
}

void vtkVRMLWriter::SetColorMap(vtkColorTransferFunction *colorMap)
{
  if (ColorMap != NULL)
    {
    ColorMap->UnRegister(this);
    }
  this->ColorMap = colorMap;
  this->ColorMap->Register(this);
}

bool vtkVRMLWriter::OpenStream()
{
  if ( !this->FileName )
  {
      vtkErrorMacro(<< "No FileName specified! Can't write!");
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      return false;
  }
  vtkDebugMacro(<<"Opening file for writing...");

  using std::ofstream;
  using std::ios;

  ofstream *fptr = new ofstream(this->FileName, ios::out);

  if (fptr->fail())
    {
    vtkErrorMacro(<< "Unable to open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    delete fptr;
    return false;
    }

  this->Stream = fptr;
  return true;
}

void vtkVRMLWriter::WriteData()
{
  vtkPolyData* pd = vtkPolyData::SafeDownCast(this->GetInput());
  if (pd)
    {
    this->WritePolyData(pd);
    }
  else
    {
    vtkErrorMacro(<< "VRML Writer can only write vtkPolyData.");
    }
}

void vtkVRMLWriter::WritePolyData(vtkPolyData *data)
{
  using std::endl;
  if ( !this->OpenStream() )
    {
    return;
    }
  (*this->Stream) << "#VRML V2.0 utf8" << endl;
  (*this->Stream) << "# VRML file written by Shawn Waldon's vtkVRMLWriter" << endl;
  (*this->Stream) << "Transform {" << endl;
  (*this->Stream) << "  translation 0 0 0" << endl;
  (*this->Stream) << "  rotation 0 0 1 0" << endl;
  (*this->Stream) << "  scale 1 1 1" << endl;
  (*this->Stream) << "  children [" << endl;
  (*this->Stream) << "    Shape {" << endl;
  (*this->Stream) << "      appearance Appearance {" << endl;
  (*this->Stream) << "        material Material {" << endl;
  (*this->Stream) << "          ambientIntensity 0" << endl;
  (*this->Stream) << "          diffuseColor 1 1 1" << endl;
  (*this->Stream) << "          specularColor 0 0 0" << endl;
  (*this->Stream) << "          shininess 0.78125" << endl;
  (*this->Stream) << "          transparency 0" << endl;
  (*this->Stream) << "        }" << endl;
  (*this->Stream) << "      }" << endl;
  (*this->Stream) << "      geometry IndexedFaceSet {" << endl;
  (*this->Stream) << "        solid FALSE" << endl;
  (*this->Stream) << "        coord DEF VTKcoordinates Coordinate {" << endl;
  (*this->Stream) << "          point [" << endl;
  // Export the points
  vtkPoints *points = data->GetPoints();
  for (int i = 0; i < points->GetNumberOfPoints(); i++)
    {
    double pt[3];
    points->GetPoint(i,pt);
    (*this->Stream) << "            " << pt[0] << " " << pt[1]
      << " " << pt[2] << "," << endl;
    }
  (*this->Stream) << "          ]" << endl;
  (*this->Stream) << "        }" << endl;
  vtkPointData *pointData = data->GetPointData();
  vtkDataArray *normals = pointData->GetNormals();
  if (normals != NULL)
    {
    (*this->Stream) << "        normal DEF VTKnormals Normal {" << endl;
    (*this->Stream) << "          vector [" << endl;
    // export the normals
    for (int i = 0; i < points->GetNumberOfPoints(); i++)
      {
      double norm[3];
      norm[0] = normals->GetComponent(i,0);
      norm[1] = normals->GetComponent(i,1);
      norm[2] = normals->GetComponent(i,2);
      (*this->Stream) << "            " << norm[0] << " " << norm[1]
        << " " << norm[2] << "," << endl;
      }
    (*this->Stream) << "          ]" << endl;
    (*this->Stream) << "        }" << endl;
    }
  else
    {
    vtkWarningMacro(<< "Dataset has no normals.");
    }
  // Export the point colors... if we have them
  if (this->ArrayToColorBy != NULL && this->ColorMap != NULL)
    {
    vtkDataArray *data = pointData->GetArray(this->ArrayToColorBy);
    if (data != NULL)
      {
      (*this->Stream) << "        color DEF VTKcolors Color {" << endl;
      (*this->Stream) << "          color [" << endl;
      for (int i = 0; i < points->GetNumberOfPoints(); i++)
        {
        double val = data->GetComponent(i,0);
        double color[3];
        this->ColorMap->GetColor(val,color);
        (*this->Stream) << "            " << color[0] << " " << color[1]
          << " " << color[2] << "," << endl;
        }
      (*this->Stream) << "          ]" << endl;
      (*this->Stream) << "        }" << endl;
      }
    else
      {
      vtkErrorMacro(<< "Expected array '" << this->ArrayToColorBy <<
        "' which does not exist.  No color data exported.");
      }
    }
  (*this->Stream) << "        coordIndex [" << endl;
  // Export the faces here
  int numPolygons = data->GetNumberOfPolys();
  vtkCellArray *polygons = data->GetPolys();
  vtkIdType nvertices, *pvertices, loc = 0;
  for (int i = 0; i < numPolygons; i++)
    {
    polygons->GetCell(loc,nvertices,pvertices);
    (*this->Stream) << "          ";
    for (int j = 0; j < nvertices; j++)
      {
      (*this->Stream) << pvertices[j] << ", ";
      }
    (*this->Stream) << "-1," << endl;
    loc += nvertices + 1;
    }
  (*this->Stream) << "        ]" << endl;
  (*this->Stream) << "      }" << endl;
  (*this->Stream) << "    }" << endl;
  (*this->Stream) << "  ]" << endl;
  (*this->Stream) << "}" << endl;
  delete this->Stream;
  this->Stream = NULL;
}

void vtkVRMLWriter::PrintSelf(ostream &os, vtkIndent indent)
{
  using std::endl;

  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << (this->FileName ?
    this->FileName : "none") << endl;
  os << indent << "ColorMap: " << this->ColorMap << endl;
}

int vtkVRMLWriter::FillInputPortInformation(int vtkNotUsed(port),
                                            vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
