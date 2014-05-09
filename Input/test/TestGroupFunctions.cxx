#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

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

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testToggleGroupMembership();
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
  
  //make sure theres only one object in the world after adding to group
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
  
  // select group with left, instance with right
  leftHandObj.computeNearestObjectAndConnector();
  rightHandObj.computeNearestObjectAndConnector();
  rightHandObj.selectSubObjectOfCurrent();

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
