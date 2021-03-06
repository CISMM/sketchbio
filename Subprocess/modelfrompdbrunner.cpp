#include "modelfrompdbrunner.h"

#include <PQP.h>

#include <vtkSmartPointer.h>
#include <vtkDecimatePro.h>
#include <vtkGeometryFilter.h>
#include <vtkThreshold.h>
#include <vtkAppendPolyData.h>

#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>

#include <QDebug>
#include <QDir>

#include <sketchioconstants.h>
#include <sketchmodel.h>
#include <modelmanager.h>
#include <modelutilities.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>

#include "subprocessutils.h"

ModelFromPDBRunner::ModelFromPDBRunner(
        SketchBio::Project *proj, const QString &pdb,
        const QString &toDelete, bool shouldExportBiologicalUnit,
        QObject *parent) :
    SubprocessRunner(parent),
    pdbId(pdb.toLower()),
    chainsToDelete(toDelete),
    modelFilePrefix(ModelUtilities::createSourceNameFor(pdb,toDelete)),
    project(proj),
    model(NULL),
    conformation(-1),
    currentRunner(NULL),
    stepNum(0),
    importFromLocalFile(false),
    exportWholeBiologicalUnit(shouldExportBiologicalUnit)
{
}

ModelFromPDBRunner::ModelFromPDBRunner(SketchBio::Project *proj, const QString &filename,
                                       const QString &modelFilePre,
                                       const QString &toDelete,
                                       bool shouldExportBiologicalUnit, QObject *parent) :
    SubprocessRunner(parent),
    pdbId(filename),
    chainsToDelete(toDelete),
    modelFilePrefix(modelFilePre),
    project(proj),
    model(NULL),
    conformation(-1),
    currentRunner(NULL),
    stepNum(0),
    importFromLocalFile(true),
    exportWholeBiologicalUnit(shouldExportBiologicalUnit)
{
}

ModelFromPDBRunner::~ModelFromPDBRunner()
{
}

void ModelFromPDBRunner::start()
{
    QString filename = (project->getProjectDir() + "/" + modelFilePrefix
                        + ".vtk").trimmed();
    currentRunner = SubprocessUtils::makeChimeraSurfaceFor(
                pdbId,filename,0,chainsToDelete,exportWholeBiologicalUnit);
    if (currentRunner == NULL) {
        emit finished(false);
        deleteLater();
    }
    connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
    currentRunner->start();
    emit statusChanged("Creating Connolly surface for " + pdbId);
    qDebug() << "Creating Connolly surface for " << pdbId;
}

void ModelFromPDBRunner::cancel()
{
    currentRunner->cancel();
    deleteLater();
}

bool ModelFromPDBRunner::isValid()
{
    return true;
}

void ModelFromPDBRunner::stepFinished(bool succeeded)
{
    if (succeeded)
    {
        QString filename = (project->getProjectDir() + "/" + modelFilePrefix
                              + ".vtk").trimmed();
        QString simplified = (project->getProjectDir() + "/" + modelFilePrefix
                              + "_isosurface.vtk").trimmed();
        QString sourceName; // = ModelUtilities::createSourceNameFor(pdbId,chainsToDelete);
        if (importFromLocalFile)
        {
            sourceName = pdbId;
        }
        else
        {
            sourceName = modelFilePrefix;
        }
        switch (stepNum)
        {
        case 0:
            //ModelUtilities::vtkConvertAsciiToBinary(filename);
        {
            QString newFileName;
            if (project->getFileInProjDir(filename,newFileName))
            {
                model =  project->getModelManager().makeModel(sourceName,newFileName,
                                                              DEFAULT_INVERSE_MASS,
                                                              DEFAULT_INVERSE_MOMENT);
            }
        }
			if (model == NULL)
			{
				qDebug() << "Failed to generate model!";
				emit finished(false);
				deleteLater();
				return;
			}
            for (int conf = 0; conf < model->getNumberOfConformations(); conf++)
            {
                if (model->getSource(conf) == sourceName)
                {
                    conformation = conf;
                    break;
                }
            }
            if (! model->hasFileNameFor(conformation,ModelResolution::SIMPLIFIED_1000))
            {
                currentRunner = SubprocessUtils::makeChimeraSurfaceFor(
                            pdbId,simplified,
                            5,chainsToDelete,exportWholeBiologicalUnit);
                if (currentRunner == NULL)
                {
                    emit finished(true);
                    deleteLater();
                }
                else
                {
                    connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
                    currentRunner->start();
                    emit statusChanged("Creating simpler surface geometry for " + pdbId);
                    qDebug() << "Creating a simpler surface geometry for " << pdbId;
                }
            }
            else
            {
                emit finished(true);
                deleteLater();
            }
            break;
        case 1:
            //ModelUtilities::vtkConvertAsciiToBinary(simplified);
            model->addSurfaceFileForResolution(conformation,
                                               ModelResolution::SIMPLIFIED_FULL_RESOLUTION,
                                               simplified);
            model->setResolutionForConformation(conformation,
                                                ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
        {
            q_vec_type pos = Q_NULL_VECTOR;
            q_type orient = Q_ID_QUAT;
            project->getWorldManager().addObject(model,pos,orient);
        }
            vtkSmartPointer< vtkPolyDataAlgorithm > source =
                    model->getVTKSource(conformation);
            vtkSmartPointer< vtkDecimatePro > decimator =
                    vtkSmartPointer< vtkDecimatePro >::New();
            QDir dir(project->getProjectDir());
            decimator->SetInputConnection(source->GetOutputPort());
            int numTris = source->GetOutput()->GetNumberOfPolys();
            decimator->SetTargetReduction(std::max(0.0,1.0 - 5000.0/numTris));
            decimator->SetFeatureAngle(60.0);
            decimator->BoundaryVertexDeletionOn();
            decimator->PreserveTopologyOn();
            decimator->Update();
            vtkSmartPointer< vtkThreshold > thresh =
                    vtkSmartPointer< vtkThreshold >::New();
            thresh->SetInputConnection(source->GetOutputPort());
            thresh->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                           "modelNum");
            thresh->ThresholdByLower(0.0);
            thresh->AllScalarsOn();
            thresh->Update();
            vtkSmartPointer< vtkGeometryFilter > convertToPolyData =
                    vtkSmartPointer< vtkGeometryFilter >::New();
            convertToPolyData->SetInputConnection(thresh->GetOutputPort());
            convertToPolyData->MergingOff();
            convertToPolyData->Update();
            vtkSmartPointer< vtkAppendPolyData > appended =
                    vtkSmartPointer< vtkAppendPolyData >::New();
            appended->AddInputConnection(decimator->GetOutputPort());
            appended->AddInputConnection(convertToPolyData->GetOutputPort());
            appended->Update();
            QString fname = ModelUtilities::createFileFromVTKSource(
                        appended,modelFilePrefix + ".decimated.5000",dir);
            model->addSurfaceFileForResolution(conformation,
                                               ModelResolution::SIMPLIFIED_5000,
                                               fname);
            decimator->SetTargetReduction(std::max(0.0,1.0 - 2000.0/numTris));
            decimator->Update();
            appended->Update();
            fname = ModelUtilities::createFileFromVTKSource(
                        appended,modelFilePrefix + ".decimated.2000",dir);
            model->addSurfaceFileForResolution(conformation,
                                               ModelResolution::SIMPLIFIED_2000,
                                               fname);
            decimator->SetTargetReduction(std::max(0.0,1.0 - 1000.0/numTris));
            decimator->Update();
            appended->Update();
            fname = ModelUtilities::createFileFromVTKSource(
                        appended,modelFilePrefix + ".decimated.1000",dir);
            model->addSurfaceFileForResolution(conformation,
                                               ModelResolution::SIMPLIFIED_1000,
                                               fname);
            emit finished(true);
            deleteLater();
            break;
        }
        stepNum++;
    }
    else
    {
        emit finished(false);
        deleteLater();
    }
}
