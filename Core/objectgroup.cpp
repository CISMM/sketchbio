#include "objectgroup.h"

#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkTransform.h>
#include <vtkAppendPolyData.h>

#include <PQP.h>

//#########################################################################
//#########################################################################
//#########################################################################
// Object Group class
//#########################################################################
//#########################################################################
//#########################################################################

//#########################################################################
ObjectGroup::ObjectGroup() :
    SketchObject(),
    children(),
    orientedBBs(vtkSmartPointer< vtkAppendPolyData >::New()),
    orientedHalfPlaneOutlines(vtkSmartPointer< vtkAppendPolyData >::New())
{
    vtkSmartPointer< vtkPoints > pts =
            vtkSmartPointer< vtkPoints >::New();
    pts->InsertNextPoint(0.0,0.0,0.0);
    vtkSmartPointer< vtkPolyData > pdata =
            vtkSmartPointer< vtkPolyData >::New();
    pdata->SetPoints(pts);
    orientedBBs->AddInputData(pdata);
    orientedHalfPlaneOutlines->AddInputData(pdata);
}

//#########################################################################
ObjectGroup::~ObjectGroup()
{
    SketchObject *obj;
    for (int i = 0; i < children.length(); i++)
    {
        obj = children[i];
        delete obj;
        children[i] = (SketchObject *) NULL;
    }
    children.clear();
}

//#########################################################################
int ObjectGroup::numInstances() const
{
    int sum = 0;
    for (int i = 0; i < children.length(); i++)
    {
        sum += children[i]->numInstances();
    }
    return sum;
}

//#########################################################################
SketchModel *ObjectGroup::getModel()
{
    SketchModel *retVal = NULL;
    if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getModel();
        }
    }
    return retVal;
}

//#########################################################################
const SketchModel *ObjectGroup::getModel() const
{
    const SketchModel *retVal = NULL;
    if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getModel();
        }
    }
    return retVal;
}

//#########################################################################
SketchObject::ColorMapType::Type ObjectGroup::getColorMapType() const
{
    ColorMapType::Type retVal = ColorMapType::SOLID_COLOR_RED;
    if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getColorMapType();
        }
    }
    return retVal;
}

//#########################################################################
void ObjectGroup::setColorMapType(ColorMapType::Type cmap)
{
    for (int i = 0; i < children.length(); i++)
    {
        children[i]->setColorMapType(cmap);
    }
}

//#########################################################################
const QString &ObjectGroup::getArrayToColorBy() const
{
    if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                return children[i]->getArrayToColorBy();
        }
    }
    throw "Groups are not colored by arrays.";
}

//#########################################################################
void ObjectGroup::setArrayToColorBy(const QString &arrayName)
{
    for (int i = 0; i < children.length(); i++)
    {
        children[i]->setArrayToColorBy(arrayName);
    }
}

//#########################################################################
vtkTransformPolyDataFilter *ObjectGroup::getTransformedGeometry()
{
    vtkTransformPolyDataFilter *retVal = NULL;
    if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getTransformedGeometry();
        }
    }
    return retVal;
}

//#########################################################################
vtkActor *ObjectGroup::getActor()
{
    vtkActor *retVal = NULL;
    if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                retVal = children[i]->getActor();
        }
    }
    return retVal;
}

//#########################################################################
void ObjectGroup::addObject(SketchObject *obj)
{
    SketchObject *p = this;
    // no "I'm my own grandpa" allowed
    while (p != NULL)
    {
        if (obj == p) return;
        p = p->getParent();
    }
    // compute and set the position of the object relative to the group
    vtkSmartPointer< vtkTransform > tfrm =
            vtkSmartPointer< vtkTransform >::New();
    tfrm->Identity();
    tfrm->PostMultiply();
    tfrm->Concatenate(obj->getLocalTransform());
    tfrm->Concatenate(getInverseLocalTransform());
    q_vec_type relPos;
    q_type relOrient;
    double wxyz[4];
    tfrm->GetPosition(relPos);
    tfrm->GetOrientationWXYZ(wxyz);
    q_from_axis_angle(relOrient,wxyz[1],wxyz[2],wxyz[3],Q_PI/180.0 * wxyz[0]);
    obj->setPosAndOrient(relPos,relOrient);
    // set parent and child relation for new object
    obj->setParent(this);
    children.append(obj);
    orientedBBs->AddInputConnection(0,obj->getOrientedBoundingBoxes()->GetOutputPort(0));
    orientedBBs->Update();
    orientedHalfPlaneOutlines->AddInputConnection(
                0,obj->getOrientedHalfPlaneOutlines()->GetOutputPort(0));
    orientedHalfPlaneOutlines->Update();
    notifyObjectAdded(obj);
}

//#########################################################################
void ObjectGroup::removeObject(SketchObject *obj)
{
    if (!children.contains(obj))
        return;
    q_vec_type oPos;
    q_type oOrient;
    // get inital positions/orientations
    obj->getPosition(oPos);
    obj->getOrientation(oOrient);
    // remove the item
    children.removeOne(obj);
    obj->setParent((SketchObject *)NULL);
    // ensure the object doesn't move on removal
    obj->setPosAndOrient(oPos,oOrient);
    // fix bounding boxes and stuff
    orientedBBs->RemoveInputConnection(0,obj->getOrientedBoundingBoxes()->GetOutputPort(0));
    orientedBBs->Update();
    orientedHalfPlaneOutlines->RemoveInputConnection(
                0,obj->getOrientedHalfPlaneOutlines()->GetOutputPort(0));
    notifyObjectRemoved(obj);
}

//#########################################################################
QList<SketchObject *> *ObjectGroup::getSubObjects()
{
    return &children;
}

//#########################################################################
const QList<SketchObject *> *ObjectGroup::getSubObjects() const
{
    return &children;
}

//#########################################################################
bool ObjectGroup::collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags)
{
    bool isCollision = false;
    for (int i = 0; i < children.length(); i++)
    {
        isCollision = isCollision || children[i]->collide(other,physics,pqp_flags);
        if (isCollision && pqp_flags == PQP_FIRST_CONTACT)
        {
            break;
        }
    }
    return isCollision;
}

//#########################################################################
void ObjectGroup::getBoundingBox(double bb[])
{
    if (children.size() == 0)
    {
        bb[0] = bb[1] = bb[2] = bb[3] = bb[4] = bb[5] = 0;
    }
    else if (numInstances() == 1)
    {
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
            {
                children[i]->getBoundingBox(bb);
            }
        }
    }
    else
    {
        orientedBBs->GetOutput()->GetBounds(bb);
    }
}

//#########################################################################
vtkPolyDataAlgorithm *ObjectGroup::getOrientedBoundingBoxes()
{
    return orientedBBs;
}

//#########################################################################
vtkAlgorithm *ObjectGroup::getOrientedHalfPlaneOutlines()
{
    return orientedHalfPlaneOutlines;
}

//#########################################################################
void ObjectGroup::localTransformUpdated()
{
    for (int i = 0; i < children.size(); i++)
    {
        SketchObject::localTransformUpdated(children[i]);
    }
}

//#########################################################################
void ObjectGroup::setIsVisible(bool isVisible)
{
    SketchObject::setIsVisible(isVisible);
    for (int i = 0; i < children.size(); i++)
    {
        children[i]->setIsVisible(isVisible);
    }
}

//#########################################################################
SketchObject *ObjectGroup::deepCopy()
{
    ObjectGroup *nObj = new ObjectGroup();
    for (int i = 0; i < children.length(); i++)
    {
        nObj->addObject(children[i]->deepCopy());
    }
    // TODO - keyframes, etc.
    return nObj;
}
