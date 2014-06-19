#include "sketchmodel.h"

#include <iostream>

#include <vtkSmartPointer.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <QString>
#include <QHash>
#include <QSharedPointer>

#include <PQP.h>

#include "modelutilities.h"
#include "colormaptype.h"

struct SketchModel::ConformationData
{
public:
    // The source string for the conformation.  This is not the data file but
    // what was used to generate it such as a pdb id, unless there is nothing
    // to define it but the data file, in which case this will be that filename
    QString src;
    // The resolution level in use for the conformation
    ModelResolution::ResolutionType level;
    // The data read in for the conformation
    vtkSmartPointer< vtkPolyDataAlgorithm > data;
    // The surface data for the conformation
    // Note: this filter is an identity transform that will have its input
    // changed instead of setting a new filter.  This allows getSurface() to
    // always return the same filter
    vtkSmartPointer< vtkPolyDataAlgorithm > surface;
    // The atoms data for the conformation
    vtkSmartPointer< vtkPolyDataAlgorithm > atoms;
    // The mapper for solid-colored objects with this model and conformation
    vtkSmartPointer< vtkPolyDataMapper > solidMapper;
    QHash< ColorMapType::ColorMap, vtkSmartPointer< vtkMapper > > mappers;
    // The collision model for the conformation
    QSharedPointer< PQP_Model > collisionModel;
    // The file names for all the resolutions for the conformation
    QHash< ModelResolution::ResolutionType, QString > filenames;
    // The count of uses of the conformation
    int useCount;

    //#########################################################################
    ConformationData() :
        level(ModelResolution::SIMPLIFIED_FULL_RESOLUTION),
        collisionModel(new PQP_Model()),
        useCount(0)
    {
        vtkSmartPointer< vtkTransformPolyDataFilter > id =
                vtkSmartPointer< vtkTransformPolyDataFilter >::New();
        vtkSmartPointer< vtkTransform > trans =
                vtkSmartPointer< vtkTransform >::New();
        trans->Identity();
        trans->Update();
        id->SetTransform(trans);
        surface = id;
        solidMapper.TakeReference(vtkPolyDataMapper::New());
        solidMapper->SetInputConnection(surface->GetOutputPort());
    }

    ConformationData(const ConformationData& other) :
        src(other.src),
        level(other.level),
        data(other.data),
        surface(other.surface),
        atoms(other.atoms),
        solidMapper(other.solidMapper),
        collisionModel(other.collisionModel),
        filenames(other.filenames),
        useCount(other.useCount)
    {}

    ConformationData& operator=(const ConformationData& other)
    {
        if (&other == this)
            return *this;
        src = other.src;
        level = other.level;
        data = other.data;
        surface = other.surface;
        atoms = other.atoms;
        solidMapper = other.solidMapper;
        collisionModel = other.collisionModel;
        filenames = other.filenames;
        useCount = other.useCount;
        return *this;
    }
    void updateData(vtkPolyDataAlgorithm* dataSource)
    {
        data = dataSource;
        vtkSmartPointer< vtkPolyDataAlgorithm > surf =
                vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                    ModelUtilities::modelSurfaceFrom(dataSource));
        surface->SetInputConnection(surf->GetOutputPort());
        surface->Update();
        solidMapper->Update();
        atoms.TakeReference(ModelUtilities::modelAtomsFrom(dataSource));
		// Only make the PQP model if it is empty. This way the collision detection
		// model will always use the full resolution because updateData() is always
		// called first using the full resolution data by addConformation().
		if (collisionModel->build_state == 0) {
			ModelUtilities::makePQP_Model(collisionModel.data(),
                                      surface->GetOutput());
		}
    }
};

SketchModel::SketchModel(double iMass, double iMoment, QObject *parent) :
    QObject(parent),
    numConformations(0),
    conformations(),
    invMass(iMass),
    invMomentOfInertia(iMoment)
{
}

SketchModel::~SketchModel()
{
}

int SketchModel::getNumberOfConformations() const
{
    return numConformations;
}

ModelResolution::ResolutionType SketchModel::getResolutionLevel(int conformationNum) const
{
    return conformations[conformationNum].level;
}

vtkPolyDataAlgorithm *SketchModel::getVTKSource(int conformationNum)
{
    return conformations[conformationNum].data;
}

vtkPolyDataAlgorithm *SketchModel::getVTKSurface(int conformationNum)
{
    return conformations[conformationNum].surface;
}

vtkMapper* SketchModel::getSolidSurfaceMapper(int conformationNum)
{
    return conformations[conformationNum].solidMapper;
}

vtkMapper* SketchModel::getColoredSurfaceMapper(int conformationNum,
                                                const ColorMapType::ColorMap &cmap)
{
    ConformationData& conf = conformations[conformationNum];
    if (!conf.mappers.contains(cmap))
    {
        // create it
        vtkSmartPointer< vtkPolyDataMapper > mapper =
                vtkSmartPointer< vtkPolyDataMapper >::New();

        vtkPointData *pointData = conf.surface->GetOutput()->GetPointData();
        double range[2] = { 0.0, 1.0};
        if (pointData->HasArray(cmap.second.toStdString().c_str()))
        {
            pointData->GetArray(cmap.second.toStdString().c_str())->GetRange(range);
        }
        if (cmap.second == "charge")
        {
            range[0] =  10.0;
            range[1] = -10.0;
        }

        if (!cmap.isSolidColor() &&
                pointData->HasArray(cmap.second.toStdString().c_str()))
        {
            vtkSmartPointer< vtkColorTransferFunction > colorFunc =
                    vtkSmartPointer< vtkColorTransferFunction >::Take(
                        cmap.getColorMap(range[0],range[1]));
            mapper->SetInputConnection(conf.surface->GetOutputPort());
            mapper->ScalarVisibilityOn();
            mapper->SetColorModeToMapScalars();
            mapper->SetScalarModeToUsePointFieldData();
            mapper->SelectColorArray(cmap.second.toStdString().c_str());
            mapper->SetLookupTable(colorFunc);
            mapper->Update();
            // add it to the datastructure
            conf.mappers.insert(cmap,mapper.GetPointer());
        }
        else
        {
            return conf.solidMapper;
        }
    }
    return conf.mappers.value(cmap);
}

vtkPolyDataAlgorithm *SketchModel::getAtomData(int conformation)
{
    return conformations[conformation].atoms;
}

PQP_Model *SketchModel::getCollisionModel(int conformationNum)
{
    return conformations[conformationNum].collisionModel.data();
}

int SketchModel::getNumberOfUses(int conformation) const
{
    return conformations[conformation].useCount;
}

bool SketchModel::hasFileNameFor(int conformation,
                                 ModelResolution::ResolutionType resolution) const
{
    return conformations[conformation].filenames.contains(resolution);
}

QString SketchModel::getFileNameFor(int conformation,
                                    ModelResolution::ResolutionType resolution) const
{
    return conformations[conformation].filenames.value(resolution);
}

const QString &SketchModel::getSource(int conformation) const
{
    return conformations[conformation].src;
}

int SketchModel::getConformationNumber(QString source) const
{
	for(int i = 0; i < getNumberOfConformations(); i++) {
		if (conformations[i].src == source) {
			return i;
		}
	}
	return -1;
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
    ConformationData newConf;
    newConf.src = src;
    newConf.filenames.insert(ModelResolution::FULL_RESOLUTION,fullResolutionFileName);
    newConf.level = ModelResolution::FULL_RESOLUTION;
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
	// The collision detection model should always use the full resolution.
	// Currently this happens because updateData() is always called the first time
	// with the full resolution, and will not make a new PQP Model if it is non-empty
    newConf.updateData(filter);
    // populate the PQP collision detection model
    PQP_Model* collisionModel = newConf.collisionModel.data();
    // get the orientation of the model
    if (collisionModel->num_tris < 5000)
    {
        newConf.filenames.insert(ModelResolution::SIMPLIFIED_FULL_RESOLUTION,
                                 fullResolutionFileName);
        newConf.filenames.insert(ModelResolution::SIMPLIFIED_5000,
                                 fullResolutionFileName);
    }
    if (collisionModel->num_tris < 2000)
    {
        newConf.filenames.insert(ModelResolution::SIMPLIFIED_2000,
                                 fullResolutionFileName);
    }
    if (collisionModel->num_tris < 1000)
    {
        newConf.filenames.insert(ModelResolution::SIMPLIFIED_1000,
                                 fullResolutionFileName);
    }
    conformations.append(newConf);
    return numConformations++;
}

void SketchModel::incrementUses(int conformation)
{
    conformations[conformation].useCount++;
    setResolutionLevelByUses(conformation);
}

void SketchModel::decrementUses(int conformation)
{
    conformations[conformation].useCount--;
}

void SketchModel::addSurfaceFileForResolution(
        int conformation,
        ModelResolution::ResolutionType resolution,
        const QString &filename)
{
    conformations[conformation].filenames.insert(resolution,filename);
    setResolutionLevelByUses(conformation);
}

void SketchModel::setResolutionForConformation(
        int conformation, ModelResolution::ResolutionType resolution)
{
    ConformationData& conf = conformations[conformation];
    if (conf.filenames.contains(resolution) && conf.level != resolution
            && conf.filenames.value(conf.level) != conf.filenames.value(resolution))
    {
        vtkSmartPointer< vtkPolyDataAlgorithm > dataSource =
                vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                    ModelUtilities::read(conf.filenames.value(resolution)));
        conf.updateData(dataSource);
        conf.level = resolution;
    }
}

void SketchModel::setResolutionLevelByUses(int conformation)
{
    ConformationData& conf = conformations[conformation];
    int uses = conf.useCount;
    int res;
    switch (conf.level)
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

