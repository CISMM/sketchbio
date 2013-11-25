#include "modelinstance.h"

#include <iostream>

#include <vtkCubeSource.h>
#include <vtkExtractEdges.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <PQP.h>

#include <vtkGiftRibbonSource.h>

#include "sketchmodel.h"
#include "physicsstrategy.h"

//#########################################################################
//#########################################################################
//#########################################################################
// Model Instance class
//#########################################################################
//#########################################################################
//#########################################################################
ModelInstance::ModelInstance(SketchModel *m, int confNum) :
    SketchObject(),
    actor(vtkSmartPointer<vtkActor>::New()),
    model(m),
    colorMap(ColorMapType::SOLID_COLOR_RED),
    arrayToColorBy("modelNum"),
    conformation(confNum),
    modelTransformed(vtkSmartPointer< vtkTransformPolyDataFilter >::New()),
    orientedBB(vtkSmartPointer< vtkTransformPolyDataFilter >::New()),
    orientedHalfPlaneOutlines(vtkSmartPointer< vtkTransformPolyDataFilter >::New()),
    solidMapper(vtkSmartPointer< vtkPolyDataMapper >::New())
{
    vtkSmartPointer< vtkCubeSource > cube =
            vtkSmartPointer< vtkCubeSource >::New();
    cube->SetBounds(model->getVTKSurface(conformation)
                    ->GetOutput()->GetBounds());
    cube->Update();
    vtkSmartPointer< vtkExtractEdges > cubeEdges =
            vtkSmartPointer< vtkExtractEdges >::New();
    cubeEdges->SetInputConnection(cube->GetOutputPort());
    cubeEdges->Update();
    orientedBB->SetInputConnection(cubeEdges->GetOutputPort());
    orientedBB->SetTransform(getLocalTransform());
    orientedBB->Update();
    vtkSmartPointer< vtkGiftRibbonSource > ribbons =
            vtkSmartPointer< vtkGiftRibbonSource >::New();
    ribbons->SetBounds(cube->GetOutput()->GetBounds());
    ribbons->Update();
    orientedHalfPlaneOutlines->SetInputConnection(ribbons->GetOutputPort());
    orientedHalfPlaneOutlines->SetTransform(getLocalTransform());
    orientedHalfPlaneOutlines->Update();
    modelTransformed->SetInputConnection(model->getVTKSurface(conformation)
                                         ->GetOutputPort());
    modelTransformed->SetTransform(getLocalTransform());
    modelTransformed->Update();
    solidMapper->SetInputConnection(modelTransformed->GetOutputPort());
    updateColorMap();
    solidMapper->Update();
    actor->SetMapper(solidMapper);
    model->incrementUses(conformation);
}

//#########################################################################
ModelInstance::~ModelInstance()
{
    model->decrementUses(conformation);
}

//#########################################################################
int ModelInstance::numInstances() const
{
    return 1;
}

//#########################################################################
SketchModel *ModelInstance::getModel()
{
    return model;
}

//#########################################################################
const SketchModel *ModelInstance::getModel() const
{
    return model;
}

//#########################################################################
ColorMapType::Type ModelInstance::getColorMapType() const
{
    return colorMap;
}

//#########################################################################
void ModelInstance::setColorMapType(ColorMapType::Type cmap)
{
    colorMap = cmap;
    updateColorMap();
}

//#########################################################################
QString ModelInstance::getArrayToColorBy() const
{
    return arrayToColorBy;
}

//#########################################################################
void ModelInstance::setArrayToColorBy(const QString &arrayName)
{
    arrayToColorBy = arrayName;
    updateColorMap();
}

//#########################################################################
vtkTransformPolyDataFilter *ModelInstance::getTransformedGeometry()
{
    return modelTransformed;
}

//#########################################################################
vtkActor *ModelInstance::getActor()
{
    return actor;
}

//#########################################################################
int ModelInstance::getModelConformation() const
{
    return conformation;
}

//#########################################################################
bool ModelInstance::collide(SketchObject *other, PhysicsStrategy *physics,
                            int pqp_flags)
{
    if (other->numInstances() != 1 || other->getModel() == NULL)
    {
        return other->collide(this,physics,pqp_flags);
    }
    else
    {
        PQP_CollideResult *cr = new PQP_CollideResult();
        PQP_REAL r1[3][3], r2[3][3], t1[3], t2[3];
        getPosition(t1);
        getOrientation(r1);
        other->getPosition(t2);
        other->getOrientation(r2);
        PQP_Collide(cr,r1,t1,model->getCollisionModel(conformation),r2,t2,
                    other->getModel()->getCollisionModel(conformation),pqp_flags);
        if (cr->NumPairs() != 0)
        {
            physics->respondToCollision(this,other,cr,pqp_flags);
        }
        return cr->NumPairs() != 0;
    }
}

//#########################################################################
void ModelInstance::getBoundingBox(double bb[])
{
    model->getVTKSurface(conformation)->GetOutput()->GetBounds(bb);
}

//#########################################################################
vtkPolyDataAlgorithm *ModelInstance::getOrientedBoundingBoxes()
{
    return orientedBB;
}

//#########################################################################
vtkAlgorithm *ModelInstance::getOrientedHalfPlaneOutlines()
{
    return orientedHalfPlaneOutlines;
}

//#########################################################################
void ModelInstance::localTransformUpdated()
{
    modelTransformed->Update();
}

//#########################################################################
SketchObject *ModelInstance::getCopy()
{
    SketchObject *nObj = new ModelInstance(model,conformation);
    q_vec_type pos;
    q_type orient;
    getPosition(pos);
    getOrientation(orient);
    nObj->setPosAndOrient(pos,orient);
    return nObj;
}

//#########################################################################
SketchObject* ModelInstance::deepCopy()
{
    ModelInstance *nObj = new ModelInstance(model,conformation);
    nObj->makeDeepCopyOf(this);
    return nObj;
}

//#########################################################################
void ModelInstance::updateColorMap()
{
    vtkPointData *pointData = modelTransformed->GetOutput()->GetPointData();
    double range[2] = { 0.0, 1.0};
    if (pointData->HasArray(arrayToColorBy.toStdString().c_str()))
    {
        pointData->GetArray(arrayToColorBy.toStdString().c_str())->GetRange(range);
    }
    if (arrayToColorBy == "charge")
    {
        range[0] =  10.0;
        range[1] = -10.0;
    }
    vtkSmartPointer< vtkColorTransferFunction > colorFunc =
            vtkSmartPointer< vtkColorTransferFunction >::Take(
                ColorMapType::getColorMap(colorMap,range[0],range[1])
            );
    if (pointData->HasArray(arrayToColorBy.toStdString().c_str()))
    {
        solidMapper->ScalarVisibilityOn();
        solidMapper->SetColorModeToMapScalars();
        solidMapper->SetScalarModeToUsePointFieldData();
        solidMapper->SelectColorArray(arrayToColorBy.toStdString().c_str());
        solidMapper->SetLookupTable(colorFunc);
        solidMapper->Update();
    }
    else
    {
        double rgb[3];
        colorFunc->GetColor(range[1],rgb);
        actor->GetProperty()->SetColor(rgb);
    }
}

//#########################################################################
void ModelInstance::setSolidColor(double color[])
{
    vtkPointData *pointData = modelTransformed->GetOutput()->GetPointData();
    if (pointData->HasArray(arrayToColorBy.toStdString().c_str()))
    {
        vtkSmartPointer< vtkColorTransferFunction > colorFunc =
                vtkSmartPointer< vtkColorTransferFunction >::New();
        colorFunc->SetScaleToLinear();
        colorFunc->AddRGBPoint(0.0,color[0],color[1],color[2]);
        solidMapper->ScalarVisibilityOn();
        solidMapper->SetColorModeToMapScalars();
        solidMapper->SetScalarModeToUsePointFieldData();
        solidMapper->SelectColorArray(arrayToColorBy.toStdString().c_str());
        solidMapper->SetLookupTable(colorFunc);
        solidMapper->Update();
    }
    else
    {
        actor->GetProperty()->SetColor(color);
    }
}
