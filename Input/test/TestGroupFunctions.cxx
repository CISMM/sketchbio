#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>

#include <transformmanager.h>
#include <objectgroup.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

#include <test/CompareBeforeAndAfter.h>
#include <test/TestCoreHelpers.h>

int testToggleGroupMembership();
int testCopyPaste();

int main(int argc, char *argv[])
{
  QCoreApplication app(argc,argv);
  
  int errors = 0;
  errors += testToggleGroupMembership();
  errors += testCopyPaste();
  return errors;
}

int testToggleGroupMembership()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector1 = {10, 0, 0};
  q_vec_type vector0 = {0,0,0};
  q_type orient = Q_ID_QUAT;
  
  //this obj is outside distance threshold
  proj.getWorldManager().addObject(model, vector1, orient);
  
  SketchObject *case1obj = proj.getWorldManager().getObjectIterator().next();
  
  //test case where objects nearest to both hands are the same object
  ControlFunctions::toggleGroupMembership(&proj,1, false);
  
  SketchObject *case1objAfter = proj.getWorldManager().getObjectIterator().next();
  
  if (proj.getWorldManager().getNumberOfObjects() != 1 || case1obj!=case1objAfter)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Control function didn't return upon detecting same nearest object" << std::endl;
    return 1;
  }
  
  SketchBio::Hand &leftHandObj = proj.getHand(SketchBioHandId::LEFT);
  SketchBio::Hand &rightHandObj = proj.getHand(SketchBioHandId::RIGHT);
  leftHandObj.computeNearestObjectAndConnector();
  
  proj.getWorldManager().addObject(model, vector0, orient);
  
  leftHandObj.computeNearestObjectAndConnector();
  
  TransformManager &t = proj.getTransformManager();
  
  //right hand should be over object 1, left over obj 0
  TestCoreHelpers::setTrackerWorldPosition(t, SketchBioHandId::RIGHT, vector1);
  rightHandObj.updatePositionAndOrientation();
  rightHandObj.getPosition(vector1);
  q_vec_print(vector1);
  
  leftHandObj.computeNearestObjectAndConnector();
  rightHandObj.computeNearestObjectAndConnector();
  
  double nearestObjDist0 = leftHandObj.getNearestObjectDistance();
  double nearestObjDist1 = rightHandObj.getNearestObjectDistance();
  
  if (nearestObjDist0>DISTANCE_THRESHOLD || nearestObjDist1 > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Objects too far from hand" << std::endl;
    return 1;
  }
  
  SketchObject *nearestLeft = leftHandObj.getNearestObject();
  SketchObject *nearestRight = rightHandObj.getNearestObject();
  
  if (nearestLeft==nearestRight) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Nearest objects are same for both hand" << std::endl;
    return 1;
  }
  
  //should remove obj0 and obj1 from world, and create a group with both
  ControlFunctions::toggleGroupMembership(&proj, 1, false);
  
  
  QListIterator< SketchObject * > itr =
  proj.getWorldManager().getObjectIterator();
  if (proj.getWorldManager().getNumberOfObjects() != 1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "\nobjects in world:  " << proj.getWorldManager().getNumberOfObjects() <<
    "\nobjects num instances:  " << itr.next()->numInstances() <<
    "\nobjects num instances:  " << itr.next()->numInstances() <<
    "  Wrong num of objects in world." << std::endl;
    return 1;
  }
  
  if (proj.getWorldManager().getObjectIterator().next()->getSubObjects()->size() != 2) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Group object doesn't have two instances." << std::endl;
    return 1;
  }
  
  //toggle, should add obj1 back to world
  ControlFunctions::toggleGroupMembership(&proj, 1, false);
  
  if (proj.getWorldManager().getNumberOfObjects() != 2)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Wrong num of objects in world." << std::endl;
    return 1;
  }
  
  return 0;
}

int testCopyPaste()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = {10,10,10};
  q_type orient = Q_ID_QUAT;
  
  //this obj is outside distance threshold
  proj.getWorldManager().addObject(model, vector, orient);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  double nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its outside threshold
  if (nearestObjDist < DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too close to hand." << std::endl;
    return 1;
  }
  
  int initialNumObjects = proj.getWorldManager().getNumberOfObjects();
  
  QClipboard *cb = QApplication::clipboard();
  cb->clear();
  
  //shouldn't copy anything
  ControlFunctions::copyObject(&proj, 1, false);
  
  //clipboard is empty so nothing should paste
  ControlFunctions::pasteObject(&proj, 1, false);
  
  int newNumObjects = proj.getWorldManager().getNumberOfObjects();
  
  if (initialNumObjects != newNumObjects)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of objects changed incorrectly." << std::endl;
    return 1;
  }
  
  q_vec_type nullVec = Q_NULL_VECTOR;
  
  //inside threshold
  proj.getWorldManager().addObject(model, nullVec, orient);
  
  initialNumObjects = proj.getWorldManager().getNumberOfObjects();
  
  handObj.computeNearestObjectAndConnector();
  nearestObjDist = handObj.getNearestObjectDistance();
  
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too far from hand." << std::endl;
    return 1;
  }
  
  //should copy object
  ControlFunctions::copyObject(&proj, 1, false);
  
  //should paste
  ControlFunctions::pasteObject(&proj, 1, false);
  
  //should have increased by exactly 1 
  newNumObjects = proj.getWorldManager().getNumberOfObjects();
  
  if (newNumObjects - initialNumObjects != 1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Num objects did not increase by exactly 1" << std::endl;
    return 1;
  }
  
  ObjectGroup *group = new ObjectGroup();
  
  QListIterator< SketchObject * > itr =
    proj.getWorldManager().getObjectIterator();
  
  SketchObject *obj;
  while (itr.hasNext())
  {
    obj = itr.next();
    group->addObject(obj);
    proj.getWorldManager().removeObject(obj);
  }
  
  proj.getWorldManager().addObject(group);
  
  if (proj.getWorldManager().getNumberOfObjects()!=1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  There should only be 1 object in the world" <<
    "\n  Num objects:  " << proj.getWorldManager().getNumberOfObjects() <<
    std::endl;
    return 1;
  }
  
  handObj.clearNearestObject();
  handObj.computeNearestObjectAndConnector();
  
  nearestObjDist = handObj.getNearestObjectDistance();
  
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Group is too far from hand." << std::endl;
    return 1;
  }
  
  //copy group
  ControlFunctions::copyObject(&proj, 1, false);
  
  //should paste group
  ControlFunctions::pasteObject(&proj, 1, false);
  
  itr = proj.getWorldManager().getObjectIterator();
  
  SketchObject *group1, *group2;
  
  if (itr.hasNext())
  {
    group1 = itr.next();
    if (itr.hasNext())
    {
      group2 = itr.next();
      if (itr.hasNext())
      {
        std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
        "  World has too many objects" << std::endl;
        return 1;
      }
    } else
    {
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  Group didn't copy" << std::endl;
      return 1;
    }
  } else
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  No objects in world" << std::endl;
    return 1;
  }
  
  int groupDifs = 0;
  
  group2->setPosition(nullVec);
  
  CompareBeforeAndAfter::compareObjects(group1, group2, groupDifs, true, true);
  
  if (groupDifs != 0)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Group copied, but they are different" <<
    "\n  Number of differences:  " << groupDifs <<
    "\n  Num instances g1:  " << group1->numInstances() <<
    "\n  Num instances g2:  " << group2->numInstances()
    << std::endl;
    return 1;
  }
  
  
  return 0;
}