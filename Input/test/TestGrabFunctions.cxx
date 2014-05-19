#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkLinearTransform.h>
#include <vtkMatrix4x4.h>

#include <QList>

#include <transformmanager.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <objectgroup.h>
#include <sketchtests.h>
#include <springconnection.h>
#include <measuringtape.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <connector.h>
#include <controlFunctions.h>

#include <test/CompareBeforeAndAfter.h>
#include <test/TestCoreHelpers.h>

int testGrabObjectOrWorld();
int testGrabSpringOrWOrld();
int testSelectRelativeOfCurrent();
bool compareMatrices(vtkMatrix4x4*, vtkMatrix4x4*);

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testGrabObjectOrWorld();
  errors += testGrabSpringOrWOrld();
  errors += testSelectRelativeOfCurrent();
  return errors;
}

//returns true if the two matrices are the same (within Q_EPSILON
bool compareMatrices(vtkMatrix4x4 *m0, vtkMatrix4x4 *m1)
{
  double mat0[16];
  for (int i = 0; i < 4; i++) {
  for (int j = 0; j < 4; j++) {
  mat0[i * 4 + j] = m0->GetElement(i, j);
  }
  }
  
  double mat1[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat1[i * 4 + j] = m1->GetElement(i, j);
    }
  }
  
  
  for (int i = 0; i < 16; i++)
  {
    if (Q_ABS(mat0[i]-mat1[1]) > Q_EPSILON)
    {
      return false;
    }
  }
  
  return true;
}

int testGrabObjectOrWorld()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = {10,10,10};
  q_type orient = Q_ID_QUAT;

  proj.getWorldManager().addObject(model, vector, orient);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  SketchObject *obj = handObj.getNearestObject();
  double nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its outside threshold
  if (nearestObjDist < DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too close to hand." << std::endl;
    return 1;
  }
  
  TransformManager &t = proj.getTransformManager();
  vtkLinearTransform *ltOld = t.getRoomToWorldTransform();
  vtkMatrix4x4 *oldMat = ltOld->GetMatrix();
  
  //grab world
  ControlFunctions::grabObjectOrWorld(&proj, 1, true);
  
  handObj.updateGrabbed();
  vtkLinearTransform *ltNew = t.getRoomToWorldTransform();
  vtkMatrix4x4 *newMat = ltNew->GetMatrix();
  
  //grabbing world changes room to world transform
  if (compareMatrices(oldMat, newMat))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Room to world transform did not change." << std::endl;
    return 1;
  }
  
  //release world
  ControlFunctions::grabObjectOrWorld(&proj, 1, false);
  handObj.updateGrabbed();
  
  //move object inside threshold
  q_vec_type nullVec = Q_NULL_VECTOR;
  obj->setPosition(nullVec);
  
  handObj.computeNearestObjectAndConnector();
  nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its inside threshold
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too far from hand." << std::endl;
    return 1;
  }
  
  //grab object
  ControlFunctions::grabObjectOrWorld(&proj, 1, true);
  
  //make sure spring is connected to the object
  QList< Connector * > springs = *proj.getWorldManager().getUISprings();
  
  int numberFound = 0;
  foreach (Connector *c, springs) {
    std::cout << "Testing...";
    if (c->getObject2() == obj) {
      ++numberFound;
      std::cout << "Found object" << std::endl;
    } else {
      std::cout << std::endl;
    }
  }
  if (numberFound != 3) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object wasn't connected to the right hand springs." << std::endl;
    return 1;
  }
  
  //release object
  ControlFunctions::grabObjectOrWorld(&proj, 1, false);
  
  return 0;
}
int testGrabSpringOrWOrld()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  q_vec_type pos1 = {10, 10, 10}, pos2 = {11,10,10};
  
  SpringConnection *spring = SpringConnection::makeSpring(NULL, NULL, pos1, pos2, true, 1.0, 0, true);
  proj.getWorldManager().addConnector(spring);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  
  int nearestConnectorDist = handObj.getNearestConnectorDistance();
  
  //make sure its outside threshold
  if (nearestConnectorDist < DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "nearestConnectorDist = " << nearestConnectorDist <<
    "  Connector is too close to hand." << std::endl;
    return 1;
  }
  
  TransformManager &t = proj.getTransformManager();
  vtkLinearTransform *ltOld = t.getRoomToWorldTransform();
  vtkMatrix4x4 *oldMat = ltOld->GetMatrix();
  
  //grab world
  ControlFunctions::grabSpringOrWorld(&proj, 1, true);
  
  handObj.updateGrabbed();
  vtkLinearTransform *ltNew = t.getRoomToWorldTransform();
  vtkMatrix4x4 *newMat = ltNew->GetMatrix();
  
  //make sure room to world transform changes when world is grabbed
  if (compareMatrices(oldMat, newMat))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Room to world transform did not change." << std::endl;
    return 1;
  }
  
  //release world
  ControlFunctions::grabSpringOrWorld(&proj, 1, false);
  handObj.updateGrabbed();
  
  //move spring within distance threshold
  q_vec_type nullVec = Q_NULL_VECTOR;
  spring->setEnd1WorldPosition(nullVec);
  q_vec_type end2 = {1,0,0};
  spring->setEnd2WorldPosition(end2);
  
  handObj.computeNearestObjectAndConnector();
  
  nearestConnectorDist = handObj.getNearestConnectorDistance();
  
  q_vec_type handPos = {0,0,0};
  handObj.getPosition(handPos);
  q_vec_type springPos1 = {0,0,0};
  spring->getObject1ConnectionPosition(springPos1);
  q_vec_type springPos2 = {0,0,0};
  spring->getObject2ConnectionPosition(springPos2);
  
  //make sure its inside threshold
  if (nearestConnectorDist > SPRING_DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "\nnearestConnectorDist = " << nearestConnectorDist <<
    "\nSpring_Distance_Threshold=  " << SPRING_DISTANCE_THRESHOLD <<
    "\nhand is at :  " << *handPos <<
    "\nspring pos 1 at:  " << *springPos1 <<
    "\nspring pos 2 at:  " << *springPos2 <<
    "  Connector is too far from hand." << std::endl;
    return 1;
  }
  
  //grab spring
  ControlFunctions::grabSpringOrWorld(&proj, 1, true);
  handObj.updateGrabbed();
  
  q_vec_type moveHand = {5,5,5};
  
  //move the hand that grabbed the spring
  TestCoreHelpers::setTrackerWorldPosition(t, SketchBioHandId::RIGHT, moveHand);
  handObj.updatePositionAndOrientation();
  handObj.updateGrabbed();
  
  q_vec_type end1AfterMove;
  
  spring->getEnd1WorldPosition(end1AfterMove);
  
  //make sure the end of the connector moved, as it should since it was grabbed and the tracker moved
  if (!(q_vec_equals(moveHand, end1AfterMove)))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "\nend 1 before move:  " << "null vec" <<
    "\nend 1 after move:  " << end1AfterMove[0] << "," <<end1AfterMove[1]<<","<<end1AfterMove[2]<<
    "  Object not properly grabbed, didn't move when hand moved." << std::endl;
    return 1;
  }
  
  //release spring
  ControlFunctions::grabSpringOrWorld(&proj, 1, false);
  
  return 0;
}

//tests both selectParentOfCurrent and selectChildOfCurrent
int testSelectRelativeOfCurrent()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector ={0,0,0};
  q_type orient = Q_ID_QUAT;
  
  //make a group (parent) and add a child to the group (child)
  ObjectGroup *parent = new ObjectGroup();
  SketchObject *child = proj.getWorldManager().addObject(model, vector, orient);
  proj.getWorldManager().removeObject(child);
  parent->addObject(child);
  proj.getWorldManager().addObject(parent);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  
  SketchObject *nearestObj = handObj.getNearestObject();
  
  //make sure parent is nearest object
  if (nearestObj!=parent)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Parent not selected" << std::endl;
    return 1;
  }
  
  if (handObj.getNearestObjectDistance() > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object too far from hand" << std::endl;
    return 1;
  }
  
  //button released, should select child
  ControlFunctions::selectChildOfCurrent(&proj, 1, false);
  
  handObj.computeNearestObjectAndConnector();
  
  nearestObj = handObj.getNearestObject();
  
  if (nearestObj!=child)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Child not selected" << std::endl;
    return 1;
  }
  
  //button released, should select parent again
  ControlFunctions::selectParentOfCurrent(&proj, 1, false);
  
  handObj.computeNearestObjectAndConnector();
  
  nearestObj = handObj.getNearestObject();
  
  if (nearestObj!=parent)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Parent not selected" << std::endl;
    return 1;
  }
  
  return 0;
}
