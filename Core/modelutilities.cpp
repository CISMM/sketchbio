#include "modelutilities.h"

#include <QScopedPointer>
#include <QDir>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkOBJReader.h>
#include <vtkPointData.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>

#include <PQP.h>

namespace ModelUtilities
{
/*****************************************************************************
  *
  * This function initializes the PQP_Model from the vtkPolyData object passed
  * to it.
  *
  * m1 a reference parameter to an empty PQP_Model to initialize
  * polyData a reference parameter to a vtkPolyData to pull the model data from
  *
  ****************************************************************************/
void makePQP_Model(PQP_Model *m1, vtkPolyData *polyData)
{

    int numPoints = polyData->GetNumberOfPoints();

    // get points
    QScopedPointer< double, QScopedPointerArrayDeleter< double > >
            points(new double[3*numPoints]);
    for (int i = 0; i < numPoints; i++)
    {
        polyData->GetPoint(i,&points.data()[3*i]);
    }

    int numStrips = polyData->GetNumberOfStrips();
    int numPolygons = polyData->GetNumberOfPolys();
    if (numStrips > 0)
    {  // if we have triangle strips, great, use them!
        vtkCellArray *strips = polyData->GetStrips();

        m1->BeginModel();
        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numStrips; i++)
        {
            strips->GetCell( loc, nvertices, pvertices );
            // todo, refactor to getNextCell for better performance

            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++)
            {
                p3[0] = points.data()[3*pvertices[j]];
                p3[1] = points.data()[3*pvertices[j]+1];
                p3[2] = points.data()[3*pvertices[j]+2];
                // ensure counterclockwise points
                if (j % 2 == 0)
                    m1->AddTri(p1,p2,p3,triId++);
                else
                    m1->AddTri(p1,p3,p2,triId++);
                PQP_REAL *tmp = p1;
                p1 = p2;
                p2 = p3;
                p3 = tmp;
            }
            loc += nvertices+1;
        }
        m1->EndModel();
    }
    else if (numPolygons > 0)
    { // we may not have triangle strip data, try polygons
        vtkCellArray *polys = polyData->GetPolys();

        m1->BeginModel();
        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numPolygons; i++)
        {
            polys->GetCell(loc,nvertices,pvertices);
            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++)
            {
                p3[0] = points.data()[3*pvertices[j]];
                p3[1] = points.data()[3*pvertices[j]+1];
                p3[2] = points.data()[3*pvertices[j]+2];
                m1->AddTri(p1,p2,p3,triId++);
                PQP_REAL *tmp = p2;
                p2= p3;
                p3 = tmp;
            }
            loc += nvertices +1;
        }
        m1->EndModel();
    }
    for (int i = 0 ; i< m1->num_tris; i++)
        m1->tris[i].id = i;
}

#ifdef PQP_UPDATE_EPSILON
/****************************************************************************
  ***************************************************************************/
void updatePQP_Model(PQP_Model &model,vtkPolyData &polyData)
{

    int numPoints = polyData->GetNumberOfPoints();

    // get points
    QScopedPointer< double, QScopedPointerArrayDeleter< double > >
            points(new double[3*numPoints]);
    for (int i = 0; i < numPoints; i++)
    {
        polyData->GetPoint(i,&points.data()[3*i]);
    }

    int numStrips = polyData->GetNumberOfStrips();
    int numPolygons = polyData->GetNumberOfPolys();
    if (numStrips > 0)
    {  // if we have triangle strips, great, use them!
        vtkCellArray *strips = polyData->GetStrips();

        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numStrips; i++)
        {
            strips->GetCell( loc, nvertices, pvertices );
            // todo, refactor to getNextCell for better performance

            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++)
            {
                p3[0] = points.data()[3*pvertices[j]];
                p3[1] = points.data()[3*pvertices[j]+1];
                p3[2] = points.data()[3*pvertices[j]+2];
                // ensure counterclockwise points
                if (j % 2 == 0)
                    model.UpdateTri(p1,p2,p3,triId++);
                else
                    model.UpdateTri(p1,p3,p2,triId++);
                PQP_REAL *tmp = p1;
                p1 = p2;
                p2 = p3;
                p3 = tmp;
            }
            loc += nvertices+1;
        }
    }
    else if (numPolygons > 0)
    { // we may not have triangle strip data, try polygons
        vtkCellArray *polys = polyData->GetPolys();

        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numPolygons; i++)
        {
            polys->GetCell(loc,nvertices,pvertices);
            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++)
            {
                p3[0] = points.data()[3*pvertices[j]];
                p3[1] = points.data()[3*pvertices[j]+1];
                p3[2] = points.data()[3*pvertices[j]+2];
                model.UpdateTri(p1,p2,p3,triId++);
                PQP_REAL *tmp = p2;
                p2= p3;
                p3 = tmp;
            }
            loc += nvertices +1;
        }
    }
    model.UpdateModel();
    for (int i = 0 ; i< m1->num_tris; i++)
        m1->tris[i].id = i;

}
#endif


vtkPolyDataAlgorithm *read(const QString &filename)
{
    using std::cout;
    using std::endl;
    vtkPolyDataAlgorithm *result = NULL;
    if (filename.endsWith(".vtk"))
    {
        vtkSmartPointer< vtkPolyDataReader > reader =
                vtkSmartPointer< vtkPolyDataReader >::New();
        reader->SetFileName(filename.toStdString().c_str());
        reader->Update();
        vtkTransformPolyDataFilter *identity = vtkTransformPolyDataFilter::New();
        identity->SetInputConnection(reader->GetOutputPort());
        vtkSmartPointer< vtkTransform > tfrm =
                vtkSmartPointer< vtkTransform >::New();
        tfrm->Identity();
        identity->SetTransform(tfrm);
        identity->Update();
        result = identity;
    }
    else if (filename.endsWith(".obj"))
    {
        vtkOBJReader *reader = vtkOBJReader::New();
        reader->SetFileName(filename.toStdString().c_str());
        reader->Update();
        result = reader;
    }
    else
    {
        throw "Unsupported file type in ModelUtilities::read()\n";
    }
    return result;
}

#define MODEL_NUM_ARRAY_NAME "modelNum"
#define SUFRACE_MODEL_NUM 1
#define ATOMS_MODEL_NUM 0

vtkPolyDataAlgorithm *modelSurfaceFrom(vtkPolyDataAlgorithm *rawModel)
{
    if (rawModel->GetOutput()->GetPointData()->HasArray(MODEL_NUM_ARRAY_NAME))
    {
        vtkSmartPointer< vtkThreshold > surface =
                vtkSmartPointer< vtkThreshold >::New();
        surface->SetInputConnection(rawModel->GetOutputPort());
        surface->AllScalarsOn();
        surface->SetInputArrayToProcess(
                    0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,MODEL_NUM_ARRAY_NAME);
        surface->ThresholdBetween(SUFRACE_MODEL_NUM,SUFRACE_MODEL_NUM);
        surface->Update();
        vtkGeometryFilter *geom = vtkGeometryFilter::New();
        geom->SetInputConnection(surface->GetOutputPort());
        geom->Update();
        return geom;
    }
    else
    {
        // increment reference count by 1 if returning same model
        rawModel->Register(NULL);
        return rawModel;
    }
}
vtkAlgorithm *modelAtomsFrom(vtkPolyDataAlgorithm *rawModel)
{
    if (rawModel->GetOutput()->GetPointData()->HasArray(MODEL_NUM_ARRAY_NAME))
    {

        vtkThreshold *atoms = vtkThreshold::New();
        atoms->SetInputConnection(rawModel->GetOutputPort());
        atoms->AllScalarsOn();
        atoms->SetInputArrayToProcess(
                    0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,MODEL_NUM_ARRAY_NAME);
        atoms->ThresholdBetween(ATOMS_MODEL_NUM,ATOMS_MODEL_NUM);
        atoms->Update();
        return atoms;
    }
    else
    {
        return NULL;
    }
}

QString createFileFromVTKSource(vtkPolyDataAlgorithm *algorithm, const QString &descr)
{
    return createFileFromVTKSource(algorithm,descr,QDir::current());
}

QString createFileFromVTKSource(vtkPolyDataAlgorithm *algorithm, const QString &descr,
                                const QDir &dir)
{
    vtkSmartPointer< vtkPolyDataWriter > writer =
            vtkSmartPointer< vtkPolyDataWriter >::New();
    writer->SetInputConnection(algorithm->GetOutputPort());
    writer->SetFileName(dir.absoluteFilePath(descr + ".vtk").toStdString().c_str());
    writer->SetFileTypeToBinary();
    writer->Update();
    writer->Write();
    return dir.absoluteFilePath(descr + ".vtk");
}

QString createSourceNameFor(const QString &pdbId, const QString &chainsLeftOut)
{
    QString src = "PDB-" + pdbId;
    if (!chainsLeftOut.trimmed().isEmpty())
        src = src + "-" + chainsLeftOut;
    return src;
}

void vtkConvertAsciiToBinary(const QString &filename)
{
    if (! filename.endsWith(".vtk") )
    {
        return;
    }
    else
    {
        vtkSmartPointer< vtkPolyDataReader > reader =
                vtkSmartPointer< vtkPolyDataReader >::New();
        reader->SetFileName(filename.toStdString().c_str());
        reader->Update();
        vtkSmartPointer< vtkPolyDataWriter > writer =
                vtkSmartPointer< vtkPolyDataWriter >::New();
        writer->SetFileName(filename.toStdString().c_str());
        writer->SetInputConnection(reader->GetOutputPort());
        writer->SetFileTypeToBinary();
        writer->Update();
        writer->Write();
    }
}

}
