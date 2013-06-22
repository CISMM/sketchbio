#include "sketchmodel.h"

#include <vtkCellArray.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkOBJReader.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>

#include <QDir>
#include <QString>

#include <PQP.h>

SketchModel::SketchModel(double iMass, double iMoment, QObject *parent) :
    QObject(parent),
    numConformations(0),
    resolutionLevelForConf(),
    modelDataForConf(),
    collisionModelForConf(),
    useCount(),
    source(),
    fileNames(),
    invMass(iMass),
    invMomentOfInertia(iMoment)
{
}

SketchModel::~SketchModel()
{
    qDeleteAll(collisionModelForConf);
    collisionModelForConf.clear();
}

int SketchModel::getNumberOfConformations() const
{
    return numConformations;
}

ModelResolution::ResolutionType SketchModel::getResolutionLevel(int conformationNum) const
{
    return resolutionLevelForConf[conformationNum];
}

vtkPolyDataAlgorithm *SketchModel::getVTKSource(int conformationNum)
{
    return modelDataForConf[conformationNum];
}

PQP_Model *SketchModel::getCollisionModel(int conformationNum)
{
    return collisionModelForConf[conformationNum];
}

int SketchModel::getNumberOfUses(int conformation) const
{
    return useCount[conformation];
}

bool SketchModel::hasFileNameFor(int conformation,
                                 ModelResolution::ResolutionType resolution) const
{
    return fileNames.contains(QPair< int,
                              ModelResolution::ResolutionType >(conformation,
                                                                resolution));
}

QString SketchModel::getFileNameFor(int conformation,
                                    ModelResolution::ResolutionType resolution) const
{
    return fileNames.value(QPair< int,
                           ModelResolution::ResolutionType >(conformation,resolution));
}

const QString &SketchModel::getSource(int conformation) const
{
    return source[conformation];
}

double SketchModel::getInverseMass() const
{
    return invMass;
}

double SketchModel::getInverseMomentOfInertia() const
{
    return invMomentOfInertia;
}

// helper function-- converts quaternion to a PQP rotation matrix
inline void pqpMatrixToQuat(q_type quat, const PQP_REAL mat[3][3]) {
    q_matrix_type colMat;
    for (int i = 0; i < 3; i++) {
        colMat[i][0] = mat[i][0];
        colMat[i][1] = mat[i][1];
        colMat[i][2] = mat[i][2];
    }
    q_from_col_matrix(quat,colMat);
}

int SketchModel::addConformation(const QString &src, const QString &fullResolutionFileName)
{
    useCount.append(0);
    source.append(src);
    fileNames.insert(
                QPair< int, ModelResolution::ResolutionType >(
                    numConformations,ModelResolution::FULL_RESOLUTION),
                fullResolutionFileName);
    resolutionLevelForConf.append(ModelResolution::FULL_RESOLUTION);
    vtkSmartPointer< vtkPolyDataAlgorithm > dataSource =
            ModelUtilities::read(fullResolutionFileName);
    double bb[6];
    dataSource->GetOutput()->GetBounds(bb);
    dataSource->Delete();
    vtkSmartPointer< vtkTransformPolyDataFilter > filter =
            vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    filter->SetInputConnection(dataSource->GetOutputPort());
    vtkSmartPointer< vtkTransform > transform =
            vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    filter->SetTransform(transform);
    filter->Update();
    modelDataForConf.append(filter);
    QScopedPointer<PQP_Model> collisionModel(new PQP_Model());
    // populate the PQP collision detection model
    ModelUtilities::makePQP_Model(collisionModel.data(), filter->GetOutput());
    // get the orientation of the model
    if (collisionModel.data()->num_tris < 5000)
    {
        fileNames.insert(
                    QPair< int, ModelResolution::ResolutionType >(
                        numConformations,ModelResolution::SIMPLIFIED_FULL_RESOLUTION),
                    fullResolutionFileName);
        fileNames.insert(
                    QPair< int, ModelResolution::ResolutionType >(
                        numConformations,ModelResolution::SIMPLIFIED_5000),
                    fullResolutionFileName);
    }
    if (collisionModel.data()->num_tris < 2000)
    {
        fileNames.insert(
                    QPair< int, ModelResolution::ResolutionType >(
                        numConformations,ModelResolution::SIMPLIFIED_2000),
                    fullResolutionFileName);
    }
    if (collisionModel.data()->num_tris < 1000)
    {
        fileNames.insert(
                    QPair< int, ModelResolution::ResolutionType >(
                        numConformations,ModelResolution::SIMPLIFIED_1000),
                    fullResolutionFileName);
    }
    collisionModelForConf.append(collisionModel.take());
    return numConformations++;
}

void SketchModel::incrementUses(int conformation)
{
    useCount[conformation]++;
    setResolutionLevelByUses(conformation);
}

void SketchModel::decrementUses(int conformation)
{
    useCount[conformation]--;
}

void SketchModel::addSurfaceFileForResolution(
        int conformation,
        ModelResolution::ResolutionType resolution,
        const QString &filename)
{
    fileNames.insert(
                QPair< int, ModelResolution::ResolutionType >(
                    conformation, resolution),
                filename);
    setResolutionLevelByUses(conformation);
}

void SketchModel::setReslutionForConformation(
        int conformation, ModelResolution::ResolutionType resolution)
{
    QPair< int, ModelResolution::ResolutionType > key(conformation, resolution);
    if (fileNames.contains(key) && resolutionLevelForConf[conformation] != resolution)
    {
        vtkSmartPointer< vtkPolyDataAlgorithm > dataSource =
                ModelUtilities::read(fileNames.value(key));
        dataSource->Delete();
        modelDataForConf[conformation]->SetInputConnection(dataSource->GetOutputPort());
        modelDataForConf[conformation]->Update();
        resolutionLevelForConf[conformation] = resolution;
        ModelUtilities::makePQP_Model(collisionModelForConf[conformation],
                                      modelDataForConf[conformation]->GetOutput());
    }
}

void SketchModel::setResolutionLevelByUses(int conformation)
{
    int uses = useCount[conformation];
    int res;
    switch (resolutionLevelForConf[conformation])
    {
    case ModelResolution::SIMPLIFIED_1000:
        res = 1000;
        break;
    case ModelResolution::SIMPLIFIED_2000:
        res = 2000;
        break;
    case ModelResolution::SIMPLIFIED_5000:
        res = 5000;
        break;
    case ModelResolution::SIMPLIFIED_FULL_RESOLUTION:
        res = 50000;
        break;
    default:
        res = 100000;
        break;
    }
    if (uses > 15 && res > 1000)
        setReslutionForConformation(conformation,ModelResolution::SIMPLIFIED_1000);
    else if (uses > 10 && res > 2000)
        setReslutionForConformation(conformation,ModelResolution::SIMPLIFIED_2000);
    else if (uses > 5 && res > 5000)
        setReslutionForConformation(conformation,ModelResolution::SIMPLIFIED_5000);
    else if (uses != 0 && res > 50000)
        setReslutionForConformation(conformation,ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
}

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
void makePQP_Model(PQP_Model *m1, vtkPolyData *polyData) {

    int numPoints = polyData->GetNumberOfPoints();

    // get points
    QScopedPointer< double, QScopedPointerArrayDeleter< double > >
            points(new double[3*numPoints]);
    for (int i = 0; i < numPoints; i++) {
        polyData->GetPoint(i,&points.data()[3*i]);
    }

    int numStrips = polyData->GetNumberOfStrips();
    int numPolygons = polyData->GetNumberOfPolys();
    if (numStrips > 0) {  // if we have triangle strips, great, use them!
        vtkCellArray *strips = polyData->GetStrips();

        m1->BeginModel();
        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numStrips; i++) {
            strips->GetCell( loc, nvertices, pvertices );
            // todo, refactor to getNextCell for better performance

            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
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
    } else if (numPolygons > 0) { // we may not have triangle strip data, try polygons
        vtkCellArray *polys = polyData->GetPolys();

        m1->BeginModel();
        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numPolygons; i++) {
            polys->GetCell(loc,nvertices,pvertices);
            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
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
void updatePQP_Model(PQP_Model &model,vtkPolyData &polyData) {

    int numPoints = polyData->GetNumberOfPoints();

    // get points
    QScopedPointer< double, QScopedPointerArrayDeleter< double > >
            points(new double[3*numPoints]);
    for (int i = 0; i < numPoints; i++) {
        polyData->GetPoint(i,&points.data()[3*i]);
    }

    int numStrips = polyData->GetNumberOfStrips();
    int numPolygons = polyData->GetNumberOfPolys();
    if (numStrips > 0) {  // if we have triangle strips, great, use them!
        vtkCellArray *strips = polyData->GetStrips();

        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numStrips; i++) {
            strips->GetCell( loc, nvertices, pvertices );
            // todo, refactor to getNextCell for better performance

            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
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
    } else if (numPolygons > 0) { // we may not have triangle strip data, try polygons
        vtkCellArray *polys = polyData->GetPolys();

        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numPolygons; i++) {
            polys->GetCell(loc,nvertices,pvertices);
            p1[0] = points.data()[3*pvertices[0]];
            p1[1] = points.data()[3*pvertices[0]+1];
            p1[2] = points.data()[3*pvertices[0]+2];
            p2[0] = points.data()[3*pvertices[1]];
            p2[1] = points.data()[3*pvertices[1]+1];
            p2[2] = points.data()[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
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
    writer->SetFileTypeToASCII();
    writer->Update();
    writer->Write();
    return dir.absoluteFilePath(descr + ".vtk");
}

QString createSourceNameFor(const QString &pdbId, const QString &chainsLeftOut)
{
    QString src = "PDB:" + pdbId;
    if (!chainsLeftOut.trimmed().isEmpty())
        src = src + "-" + chainsLeftOut;
    return src;
}

}
