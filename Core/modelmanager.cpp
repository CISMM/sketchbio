
/*
 *
 * This class was originally written for a homework assignment in
 * Physically Based Modeling (COMP768)
 * Author: Shawn Waldon
 *
 */

#include "modelmanager.h"
#include "sketchmodel.h"
#include "sketchioconstants.h"

#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCellArray.h>
#include <vtkOBJReader.h>
#include <vtkConeSource.h>

#include <QDebug>
#include <QDir>
#include <QScopedPointer>


ModelManager::ModelManager() :
    models(),
    modelSourceToIdx()
{
}

ModelManager::~ModelManager() {
    modelSourceToIdx.clear();
    qDeleteAll(models);
    models.clear();
}

/*****************************************************************************
  *
  * This method returns the model that is currently being used for the camera
  * objects that the user can place and move.  This will probably be something
  * created from vtk sources, although that is not necessary.  This model is
  * guaranteed to have the key CAMERA_MODEL_KEY in the models hash.
  *
  ****************************************************************************/
SketchModel *ModelManager::getCameraModel(QDir &projectDir) {
    QString cameraKey(CAMERA_MODEL_KEY);
    if (modelSourceToIdx.contains(cameraKey)) {
        return models[modelSourceToIdx.value(cameraKey)];
    } else {
        // create the "camera"
        vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
        cone->SetDirection(0,0,-1);
        cone->SetRadius(30);
        cone->SetHeight(30);
        cone->SetResolution(4);
        cone->CappingOff();
        cone->Update();
        // create the model file
        QString filename = ModelUtilities::createFileFromVTKSource(
                    cone,
                    cameraKey,
                    projectDir);
        // initialize the model data
        SketchModel *sModel = new SketchModel(DEFAULT_INVERSE_MASS,
                                              DEFAULT_INVERSE_MOMENT);
        sModel->addConformation(cameraKey,filename);
        // insert into datastructure and return the new model
        models.append(sModel);
        modelSourceToIdx.insert(cameraKey,models.size()-1);
        return sModel;
    }
}

/*****************************************************************************
  *
  * This method creates a SketchModel from the given vtk source using the given
  * scale factor.  If there is already a model using a vtk source of the same
  * class name, this
  * method simply returns the old one (ignores scale for now).
  *
  ****************************************************************************/
SketchModel *ModelManager::modelForVTKSource(QString sourceName,
                                             vtkPolyDataAlgorithm *source,
                                             double scale,
                                             QDir &dir) {

    if (modelSourceToIdx.contains(sourceName)) {
        return models[modelSourceToIdx.value(sourceName)];
    }

    vtkSmartPointer<vtkTransformPolyDataFilter> transformPD =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    source->Update();
    transformPD->SetInputConnection(source->GetOutputPort());

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->Scale(scale,scale,scale);

    transformPD->SetTransform(transform);
    transformPD->Update();

    QString filename = ModelUtilities::createFileFromVTKSource(transformPD,
                                                               sourceName,
                                                               dir);

    SketchModel *sModel = new SketchModel(DEFAULT_INVERSE_MASS,DEFAULT_INVERSE_MOMENT);
    sModel->addConformation(sourceName,filename);

    models.append(sModel);
    modelSourceToIdx.insert(sourceName,models.size()-1);

    return sModel;
}

SketchModel *ModelManager::makeModel(QString source, QString filename,
                                     double iMass, double iMoment)
{
    if (modelSourceToIdx.contains(source))
        return models[modelSourceToIdx.value(source)];
    else
    {
        SketchModel *model = new SketchModel(iMass, iMoment);
        model->addConformation(source,filename);
        models.append(model);
        modelSourceToIdx.insert(source,models.size()-1);
        return model;
    }
}

SketchModel *ModelManager::addConformation(QString originalSource,
                                            QString newSource,
                                            QString newFilename)
{
    if (modelSourceToIdx.contains(originalSource))
    {
        SketchModel *model = models[modelSourceToIdx.value(originalSource)];
        return addConformation(model,newSource,newFilename);
    }
    return NULL;
}

SketchModel *ModelManager::addConformation(SketchModel *model,
                                            QString newSource,
                                            QString newFilename)
{
    if (model == NULL)
        return NULL;
    model->addConformation(newSource,newFilename);
    return model;
}

void ModelManager::addModel(SketchModel *model)
{
    int idx = models.size();
    models.append(model);
    for (int i = 0; i < model->getNumberOfConformations(); i++)
    {
        modelSourceToIdx.insert(model->getSource(i),idx);
    }
}

/*****************************************************************************
  *
  * This method returns an iterator that may be used to examine each of the
  * models in the ModelManager.
  *
  ****************************************************************************/
QVectorIterator<SketchModel *> ModelManager::getModelIterator() const {
    return QVectorIterator<SketchModel *>(models);
}

bool ModelManager::hasModel(QString source) const
{
    return modelSourceToIdx.contains(source);
}

SketchModel *ModelManager::getModel(QString source) const
{
    if (hasModel(source))
        return models[modelSourceToIdx.value(source)];
    else
        return NULL;
}

/*****************************************************************************
  *
  * This method returns the number of models in the model manager
  *
  ****************************************************************************/
int ModelManager::getNumberOfModels() const {
    return models.size();
}

