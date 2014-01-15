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
ObjectGroup::ObjectGroup()
    : SketchObject(),
      children(),
      orientedBBs(vtkSmartPointer< vtkAppendPolyData >::New()),
      orientedHalfPlaneOutlines(vtkSmartPointer< vtkAppendPolyData >::New())
{
  vtkSmartPointer< vtkPoints > pts = vtkSmartPointer< vtkPoints >::New();
  pts->InsertNextPoint(0.0, 0.0, 0.0);
  vtkSmartPointer< vtkPolyData > pdata = vtkSmartPointer< vtkPolyData >::New();
  pdata->SetPoints(pts);
  orientedBBs->AddInputData(pdata);
  orientedHalfPlaneOutlines->AddInputData(pdata);
}

//#########################################################################
ObjectGroup::~ObjectGroup()
{
  qDeleteAll(children);
  children.clear();
}

//#########################################################################
int ObjectGroup::numInstances() const
{
  int sum = 0;
  for (int i = 0; i < children.length(); i++) {
    sum += Q_ABS(children[i]->numInstances());
  }
  return -sum;
}

//#########################################################################
ColorMapType::Type ObjectGroup::getColorMapType() const
{
  return ColorMapType::SOLID_COLOR_RED;
}

//#########################################################################
void ObjectGroup::setColorMapType(ColorMapType::Type cmap) {}

//#########################################################################
QString ObjectGroup::getArrayToColorBy() const { return ""; }

//#########################################################################
void ObjectGroup::setArrayToColorBy(const QString &arrayName) {}

//#########################################################################
void ObjectGroup::addObject(SketchObject *obj)
{
  SketchObject *p = this;
  // no "I'm my own grandpa" allowed
  while (p != NULL) {
    if (obj == p) return;
    p = p->getParent();
  }
  // compute and set the position of the object relative to the group
  q_vec_type pos;
  q_type orient;
  obj->getPosition(pos);
  obj->getOrientation(orient);
  SketchObject::setParentRelativePositionForAbsolutePosition(obj, this, pos,
                                                             orient);
  // set parent and child relation for new object
  obj->setParent(this);
  children.append(obj);
  orientedBBs->AddInputConnection(
      0, obj->getOrientedBoundingBoxes()->GetOutputPort(0));
  orientedBBs->Update();
  orientedHalfPlaneOutlines->AddInputConnection(
      0, obj->getOrientedHalfPlaneOutlines()->GetOutputPort(0));
  orientedHalfPlaneOutlines->Update();
  notifyObjectAdded(obj);
}

//#########################################################################
void ObjectGroup::removeObject(SketchObject *obj)
{
  if (!children.contains(obj)) return;
  q_vec_type oPos;
  q_type oOrient;
  // get inital positions/orientations
  obj->getPosition(oPos);
  obj->getOrientation(oOrient);
  // remove the item
  children.removeOne(obj);
  obj->setParent((SketchObject *)NULL);
  // ensure the object doesn't move on removal
  obj->setPosAndOrient(oPos, oOrient);
  // fix bounding boxes and stuff
  orientedBBs->RemoveInputConnection(
      0, obj->getOrientedBoundingBoxes()->GetOutputPort(0));
  orientedBBs->Update();
  orientedHalfPlaneOutlines->RemoveInputConnection(
      0, obj->getOrientedHalfPlaneOutlines()->GetOutputPort(0));
  notifyObjectRemoved(obj);
}

//#########################################################################
QList< SketchObject * > *ObjectGroup::getSubObjects() { return &children; }

//#########################################################################
const QList< SketchObject * > *ObjectGroup::getSubObjects() const
{
  return &children;
}

//#########################################################################
bool ObjectGroup::collide(SketchObject *other, PhysicsStrategy *physics,
                          int pqp_flags)
{
  bool isCollision = false;
  for (int i = 0; i < children.length(); i++) {
    isCollision =
        isCollision || children[i]->collide(other, physics, pqp_flags);
    if (isCollision && pqp_flags == PQP_FIRST_CONTACT) {
      break;
    }
  }
  return isCollision;
}

//#########################################################################
void ObjectGroup::getBoundingBox(double bb[])
{
  if (children.size() == 0) {
    bb[0] = bb[1] = bb[2] = bb[3] = bb[4] = bb[5] = 0;
  } else {
    orientedBBs->Update();
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
  for (int i = 0; i < children.size(); i++) {
    SketchObject::localTransformUpdated(children[i]);
  }
}

//#########################################################################
SketchObject *ObjectGroup::getCopy()
{
  ObjectGroup *nObj = new ObjectGroup();
  for (int i = 0; i < children.length(); i++) {
    nObj->addObject(children[i]->getCopy());
  }
  return nObj;
}

//#########################################################################
SketchObject *ObjectGroup::deepCopy()
{
  ObjectGroup *nObj = new ObjectGroup();
  nObj->makeDeepCopyOf(this);
  for (int i = 0; i < children.length(); i++) {
    nObj->addObject(children[i]->getCopy());
  }
  return nObj;
}
