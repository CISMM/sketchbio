
/*
 *
 * This class was originally written for a homework assignment in
 * Physically Based Modeling (COMP768)
 * Author: Shawn Waldon
 *
 */

#include <vtkSmartPointer.h>
#include <vtkConeSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <QDebug>
#include <QDir>
#include <QString>
#include <QScopedPointer>

#include "modelmanager.h"
#include "modelutilities.h"
#include "sketchmodel.h"
#include "sketchioconstants.h"

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
SketchModel *ModelManager::modelForVTKSource(const QString &sourceName,
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

SketchModel *ModelManager::makeModel(const QString &source, const QString &filename,
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

SketchModel *ModelManager::addConformation(const QString &originalSource,
                                           const QString &newSource,
                                           const QString &newFilename)
{
    if (modelSourceToIdx.contains(originalSource))
    {
        SketchModel *model = models[modelSourceToIdx.value(originalSource)];
        return addConformation(model,newSource,newFilename);
    }
    return NULL;
}

SketchModel *ModelManager::addConformation(SketchModel *model,
                                           const QString &newSource,
                                           const QString &newFilename)
{
    if (model == NULL)
        return NULL;
    if (modelSourceToIdx.contains(newSource))
    {
        return models[modelSourceToIdx.value(newSource)];
    }
    model->addConformation(newSource,newFilename);
    modelSourceToIdx.insert(newSource,models.indexOf(model));
    return model;
}

SketchModel *ModelManager::addModel(SketchModel *model)
{
    QString src = model->getSource(0);
    if (modelSourceToIdx.contains(src))
    {
        SketchModel *other = models.at(modelSourceToIdx.value(src));
        if (other->getNumberOfConformations() == model->getNumberOfConformations())
        {
            bool same = true;
            for (int i = 0; i < other->getNumberOfConformations(); i++)
            {
                if (other->getSource(i) != model->getSource(i))
                {
                    same = false;
                }
            }
            if (same)
            {
                return other;
            }
        }
    }
    int idx = models.size();
    models.append(model);
    for (int i = 0; i < model->getNumberOfConformations(); i++)
    {
        modelSourceToIdx.insert(model->getSource(i),idx);
    }
    return model;
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

bool ModelManager::hasModel(const QString &source) const
{
    return modelSourceToIdx.contains(source);
}

SketchModel *ModelManager::getModel(const QString &source) const
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

