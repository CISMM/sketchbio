#include "objectgroup.h"

#include <vtkPolyDataAlgorithm.h>
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
    orientedBBs(vtkSmartPointer<vtkAppendPolyData>::New())
{
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
    // calculate new position & orientation
    if (numInstances() == 0)
    { // special case when first thing is added
        q_vec_type pos, idV = Q_NULL_VECTOR;
        q_type orient, idQ = Q_ID_QUAT;
        obj->getPosition(pos);
        obj->getOrientation(orient);
        setPosAndOrient(pos,orient);
        // object gets position equal to group center, orientation is not messed with
        obj->setPosAndOrient(idV,idQ);
    }
    else
    { // if we already have some items in the list...
        q_vec_type pos, nPos, oPos;
        q_type idQ = Q_ID_QUAT;
        getPosition(pos);
        obj->getPosition(oPos);
        q_vec_scale(nPos,children.length(),pos);
        q_vec_add(nPos,oPos,nPos);
        q_vec_scale(nPos,1.0/(children.length()+1),nPos);
        // calculate change and apply it to children
        for (int i = 0; i < children.length(); i++)
        {
            q_vec_type cPos;
            q_type cOrient;
            children[i]->getPosition(cPos);
            children[i]->getOrientation(cOrient);
            q_vec_subtract(cPos,cPos,nPos);
            children[i]->setPosAndOrient(cPos,cOrient);
        }
        // set group's new position and orientation
        setPosAndOrient(nPos,idQ);
        // set the new object's position
        q_vec_subtract(oPos,oPos,nPos);
        obj->setPosition(oPos);
    }
    // set parent and child relation for new object
    obj->setParent(this);
    children.append(obj);
    orientedBBs->AddInputConnection(0,obj->getOrientedBoundingBoxes()->GetOutputPort(0));
    orientedBBs->Update();
    notifyObjectAdded(obj);
}

//#########################################################################
void ObjectGroup::removeObject(SketchObject *obj)
{
    if (!children.contains(obj))
        return;
    q_vec_type pos, nPos, oPos, idV = Q_NULL_VECTOR;
    q_type oOrient, idQ = Q_ID_QUAT;
    // get inital positions/orientations
    getPosition(pos);
    obj->getPosition(oPos);
    obj->getOrientation(oOrient);
    // compute new group position...
    q_vec_scale(nPos,children.length(),pos);
    q_vec_subtract(nPos,nPos,oPos);
    q_vec_scale(nPos,1.0/(children.length()-1),nPos);
    // remove the item
    children.removeOne(obj);
    obj->setParent((SketchObject *)NULL);
    obj->setPosAndOrient(oPos,oOrient);
    orientedBBs->RemoveInputConnection(0,obj->getOrientedBoundingBoxes()->GetOutputPort(0));
    orientedBBs->Update();
    // apply the change to the group's children
    for (int i = 0; i < children.length(); i++)
    {
        q_vec_type cPos;
        q_type cOrient;
        children[i]->getPosition(cPos);
        children[i]->getOrientation(cOrient);
        q_vec_subtract(cPos,cPos,nPos);
        children[i]->setPosAndOrient(cPos,cOrient);
    }
    // reset the group's center and orientation
    setPosAndOrient(nPos,idQ);
    // if there is only one thing left in the group
    if (numInstances() == 1)
    {
        SketchObject *child = NULL;
        for (int i = 0; i < children.length(); i++)
        {
            if (children[i]->numInstances() == 1)
                child = children[i];
        }
        q_vec_type cPos, cOrient;
        child->getPosition(cPos);
        child->getOrientation(cOrient);
        child->setPosAndOrient(idV,idQ);
        setPosAndOrient(cPos,cOrient);
    }
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
