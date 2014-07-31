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
	luminance(1.0),
	minLuminance(0.5),
	maxLuminance(1.0),
    actor(vtkSmartPointer<vtkActor>::New()),
    model(m),
    conformation(confNum),
	displayingFullRes(false),
    orientedBB(vtkSmartPointer< vtkTransformPolyDataFilter >::New()),
    orientedHalfPlaneOutlines(vtkSmartPointer< vtkTransformPolyDataFilter >::New())
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
    updateColorMap();
    actor->SetMapper(model->getSolidSurfaceMapper(conformation));
    actor->SetUserTransform(localTransform);
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
void ModelInstance::getBBVertices(q_vec_type vertices[])
{
    double bb[6];
	getBoundingBox(bb);
	q_vec_set(vertices[0], bb[0], bb[2], bb[4]);
	q_vec_set(vertices[1], bb[0], bb[3], bb[4]);
	q_vec_set(vertices[2], bb[0], bb[2], bb[5]);
	q_vec_set(vertices[3], bb[0], bb[3], bb[5]);
	q_vec_set(vertices[4], bb[1], bb[2], bb[4]);
	q_vec_set(vertices[5], bb[1], bb[3], bb[4]);
	q_vec_set(vertices[6], bb[1], bb[2], bb[5]);
	q_vec_set(vertices[7], bb[1], bb[3], bb[5]);
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
SketchObject *ModelInstance::getCopy()
{
    SketchObject *nObj = new ModelInstance(model,conformation);
    q_vec_type pos;
    q_type orient;
    getPosition(pos);
    getOrientation(orient);
    nObj->setPosAndOrient(pos,orient);
    nObj->setIsVisible(isVisible());
    nObj->setActive(isActive());
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
void ModelInstance::showFullResolution() 
{
	if (!displayingFullRes) {
		displayingFullRes = true;
		actor->SetMapper(model->getFullResSolidSurfaceMapper(conformation));
	}
}

//#########################################################################
void ModelInstance::hideFullResolution() 
{
	if (displayingFullRes) {
		displayingFullRes = false;
		actor->SetMapper(model->getSolidSurfaceMapper(conformation));
	}
}

//#########################################################################
double ModelInstance::getLuminance() const 
{ 
	return luminance; 
}

//#########################################################################
double ModelInstance::getDisplayLuminance() const 
{ 
	return ((maxLuminance - minLuminance) * luminance) + minLuminance; 
}

//#########################################################################
void ModelInstance::setLuminance(double lum) 
{
	luminance = lum;
	updateColorMap();
}

//#########################################################################
void ModelInstance::setMinLuminance(double minLum) 
{
	minLuminance = minLum;
	updateColorMap();
}

//#########################################################################
void ModelInstance::setMaxLuminance(double maxLum) 
{
	maxLuminance = maxLum;
	updateColorMap();
}

//#########################################################################
void ModelInstance::updateColorMap()
{
    const ColorMapType::ColorMap& cmap = getColorMap();
    if (cmap.isSolidColor())
    {
		if (!displayingFullRes) {
			actor->SetMapper(model->getSolidSurfaceMapper(conformation));
		} 
		else {
			actor->SetMapper(model->getFullResSolidSurfaceMapper(conformation));	
		}
        vtkSmartPointer< vtkColorTransferFunction > colorFunc =
                vtkSmartPointer< vtkColorTransferFunction >::Take(
                    cmap.getColorMap(0,1)
                );
        double rgb[3];
		double displayLum = getDisplayLuminance(); 
        colorFunc->GetColor(1,rgb);
		rgb[0] *= displayLum;
		rgb[1] *= displayLum;
		rgb[2] *= displayLum;
        actor->GetProperty()->SetColor(rgb);
    }
    else
    {
        actor->SetMapper(model->getColoredSurfaceMapper(conformation,cmap));
    }
}

//#########################################################################
void ModelInstance::setSolidColor(double color[])
{
    actor->GetProperty()->SetColor(color);
}
