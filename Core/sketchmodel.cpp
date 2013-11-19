#include "sketchmodel.h"

#include <iostream>

#include <vtkPolyDataAlgorithm.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <QString>

#include <PQP.h>

#include "modelutilities.h"

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

vtkPolyDataAlgorithm *SketchModel::getVTKSurface(int conformationNum)
{
    return surfaceDataForConf[conformationNum];
}

vtkPolyDataAlgorithm *SketchModel::getAtomData(int conformation)
{
    return atomDataForConf[conformation];
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
inline void pqpMatrixToQuat(q_type quat, const PQP_REAL mat[3][3])
{
    q_matrix_type colMat;
    for (int i = 0; i < 3; i++)
    {
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
            vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                ModelUtilities::read(fullResolutionFileName));
    double bb[6];
    dataSource->GetOutput()->GetBounds(bb);
    vtkSmartPointer< vtkTransformPolyDataFilter > filter =
            vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    filter->SetInputConnection(dataSource->GetOutputPort());
    vtkSmartPointer< vtkTransform > transform =
            vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    filter->SetTransform(transform);
    filter->Update();
    modelDataForConf.append(filter);
    vtkSmartPointer< vtkPolyDataAlgorithm > surface =
            vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                ModelUtilities::modelSurfaceFrom(dataSource));
    vtkSmartPointer< vtkTransformPolyDataFilter > ifilter =
            vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    ifilter->SetInputConnection(surface->GetOutputPort());
    vtkSmartPointer< vtkTransform > itransform =
            vtkSmartPointer< vtkTransform >::New();
    itransform->Identity();
    ifilter->SetTransform(transform);
    ifilter->Update();
    surfaceDataForConf.append(ifilter);
    atomDataForConf.append(vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                               ModelUtilities::modelAtomsFrom(dataSource)));
    QScopedPointer<PQP_Model> collisionModel(new PQP_Model());
    // populate the PQP collision detection model
    ModelUtilities::makePQP_Model(collisionModel.data(),
                                  surfaceDataForConf.last()->GetOutput());
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

void SketchModel::setResolutionForConformation(
        int conformation, ModelResolution::ResolutionType resolution)
{
    QPair< int, ModelResolution::ResolutionType > key(conformation, resolution);
    QPair< int, ModelResolution::ResolutionType > current(
                conformation, resolutionLevelForConf[conformation]);
    if (fileNames.contains(key)
            && resolutionLevelForConf[conformation] != resolution
            && fileNames.value(key) != fileNames.value(current))
    {
        vtkSmartPointer< vtkPolyDataAlgorithm > dataSource =
                vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                    ModelUtilities::read(fileNames.value(key)));
        modelDataForConf[conformation]->SetInputConnection(
                    dataSource->GetOutputPort());
        modelDataForConf[conformation]->Update();
        vtkSmartPointer< vtkPolyDataAlgorithm > surface =
                vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                    ModelUtilities::modelSurfaceFrom(dataSource));
        vtkSmartPointer< vtkPolyDataAlgorithm > atoms =
                vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                    ModelUtilities::modelAtomsFrom(dataSource));
        surfaceDataForConf[conformation]->SetInputConnection(
                    surface->GetOutputPort());
        atomDataForConf[conformation] = atoms;
        resolutionLevelForConf[conformation] = resolution;
        ModelUtilities::makePQP_Model(collisionModelForConf[conformation],
                                      surfaceDataForConf[conformation]->GetOutput());
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
        setResolutionForConformation(conformation,ModelResolution::SIMPLIFIED_1000);
    else if (uses > 10 && res > 2000)
        setResolutionForConformation(conformation,ModelResolution::SIMPLIFIED_2000);
    else if (uses > 5 && res > 5000)
        setResolutionForConformation(conformation,ModelResolution::SIMPLIFIED_5000);
    else if (uses != 0 && res > 50000)
        setResolutionForConformation(conformation,ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
}

