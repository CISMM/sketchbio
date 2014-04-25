#include <cassert>
#include <iostream>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include <transformeditoperationstate.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

#include <sketchtests.h>
#include <test/TestCoreHelpers.h>

int testDeleteObject();
int testReplicateObject();
int testSetTransforms(double,double,double,double,double,double);

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testDeleteObject();
  errors += testReplicateObject();
  //one position transform
  errors += testSetTransforms(11.0,0,0,0,0,0);
  //one orientation transform
  errors += testSetTransforms(0,0,0,6.9,0,0);
  //one position and one orientation transform
  errors += testSetTransforms(2.5,0,0,3.7,0,0);
  //two positions
  errors += testSetTransforms(2.5,6.3,0,0,0,0);
  //two orientations
  errors += testSetTransforms(0,0,0,3.7,9.5,0);
  //two positions and one orientation
  errors += testSetTransforms(2.5,0,6.4,0,4.3,0);
  //one position and two orientations
  errors += testSetTransforms(2.5,0,0,3.7,0,9.2);
  //all position and orientation transforms;
  errors += testSetTransforms(3.2,4.5,11.7,5.0,1.1,8.9);
  
  return errors;
}

int testDeleteObject()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = Q_NULL_VECTOR;
  q_type orient = Q_ID_QUAT;
  
  //this obj is inside distance threshold
  proj.getWorldManager().addObject(model, vector, orient);
  
  q_vec_set(vector,10,10,10);
  
  //this obj is outside distance threshold
  proj.getWorldManager().addObject(model, vector, orient);
  proj.updateTrackerPositions();
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  //this should be the distance to obj at origin
  double nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its within threshold
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is not close enough to hand." << std::endl;
    return 1;
  }
  
  //should delete objNear
  ControlFunctions::deleteObject(&proj, 1, false);
  
  handObj.computeNearestObjectAndConnector();
  
  //get new nearest object distance
  double nearestObjDistAfterDelete = handObj.getNearestObjectDistance();
  
  //make sure this changed
  if (nearestObjDist == nearestObjDistAfterDelete)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Nearest object was not deleted!." << std::endl;
    return 1;
  }
  
  if (nearestObjDistAfterDelete < DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object needs to be further away for this test." << std::endl;
    return 1;
  }
  
  //shouldn't delete anything
  ControlFunctions::deleteObject(&proj, 1, false);
  
  handObj.computeNearestObjectAndConnector();
  
  //get new nearest object distance (which shouldn't have changed)
  double nearestObjDistAfterSecondDelete = handObj.getNearestObjectDistance();
  
  //make sure these values are still the same
  if (nearestObjDistAfterDelete != nearestObjDistAfterSecondDelete)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object was incorrectly deleted." << std::endl;
    return 1;
  }
  
  
  return 0;
}


int testReplicateObject()
{
  
  //create object
  //replicate it
  //make sure replicaes.size() gets increased by 1 using getNumberOfCrystalByExamples();
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = {10,10,10};
  q_type orient = Q_ID_QUAT;
  
  
  //this obj is OUTSIDE distance threshold
  proj.getWorldManager().addObject(model, vector, orient);
  
  int initialReplicaSize = proj.getNumberOfCrystalByExamples();
  
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
  
  ControlFunctions::replicateObject(&proj, 1, true);
  
  int newReplicaSize = proj.getNumberOfCrystalByExamples();
  
  if (newReplicaSize != initialReplicaSize)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Replicas size changed incorrectly." << std::endl;
    return 1;
  }
  
  q_vec_type vector2 = Q_NULL_VECTOR;
  
  proj.getWorldManager().addObject(model, vector2, orient);
  
  handObj.computeNearestObjectAndConnector();
  //this should be the distance to obj at origin
  nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its inside threshold
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too far from hand." << std::endl;
    return 1;
  }
  
  ControlFunctions::replicateObject(&proj, 1, true);
  
  newReplicaSize = proj.getNumberOfCrystalByExamples();
  
  if (newReplicaSize - initialReplicaSize != 1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Replicas size did not increase by 1" << std::endl;
    return 1;
  }
  
  return 0;
}

int testSetTransforms(double x, double y, double z, double rX, double rY, double rZ)
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type const vector0 = {5,3,2};
  q_vec_type vector1 = {1,1,1};
  q_type orient = Q_ID_QUAT;
  q_make(orient,1,0,0,Q_PI/3.0);
  
  TransformEditOperationState teos(&proj);
  
  SketchObject *obj0 = proj.getWorldManager().addObject(model, vector0, orient);
  SketchObject *obj1 = proj.getWorldManager().addObject(model, vector1, orient);
  
  teos.addObject(obj0);
  teos.addObject(obj1);
  
  teos.setObjectTransform(x, y, z, rX, rY, rZ);
  
  //make sure obj0 did not move
  q_vec_type pos0;
  obj0->getPosition(pos0);
  
  if (!(q_vec_equals(vector0, pos0)))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object 0 should not have moved." << std::endl;
    return 1;
  }
  
  //make sure obj1 did move, and to the right place
  vtkSmartPointer< vtkTransform > transform =
          vtkSmartPointer< vtkTransform >::Take(
              SketchObject::computeRelativeTransform(obj0,obj1));
  
  q_vec_type pos1;
  transform->GetPosition(pos1);
  q_vec_type ort1;
  transform->GetOrientation(ort1);
  
  teos.setObjectTransform(pos1[0], pos1[1], pos1[2], ort1[0], ort1[1], ort1[2]);
  
  q_vec_type pos2;
  transform->GetPosition(pos2);
  q_vec_type ort2;
  transform->GetOrientation(ort2);

  if(!(q_vec_equals(pos2, pos1) && q_vec_equals(ort2, ort1)))
  {
    std::cout << "\nError at " << __FILE__ << ":" << __LINE__ <<
    "  input values: " <<x<<","<<y<<","<<z<<","<<rX<<","<<rY<<","<<rZ<<
    "\nShould have moved to: ";
    q_vec_print(pos1);
    std::cout << "Moved to: ";
    q_vec_print(pos2);
    std::cout << "Should have orientation: ";
    q_vec_print(ort1);
    std::cout << "Has orientation: ";
    q_vec_print(ort2);
    std::cout << "  Object 1 is in the wrong place." << std::endl;
    return 1;
  }
  
  q_vec_type pos2BeforeCancel;
  obj1->getPosition(pos2BeforeCancel);
  q_type ort2BeforeCancel;
  obj1->getOrientation(ort2BeforeCancel);
 
  //make sure cancelOp doesn't move any of the objects
  teos.cancelOperation();
  
  //make sure obj0 did not move
  obj0->getPosition(pos0);
  
  if (!(q_vec_equals(vector0, pos0)))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object 0 should not have moved." << std::endl;
    return 1;
  }
  
  q_vec_type pos2AfterCancel;
  obj1->getPosition(pos2AfterCancel);
  q_type ort2AfterCancel;
  obj1->getOrientation(ort2AfterCancel);
  
  if (!(q_vec_equals(pos2BeforeCancel, pos2AfterCancel) && q_equals(ort2BeforeCancel, ort2AfterCancel)))
  {
    std::cout << "\nError at " << __FILE__ << ":" << __LINE__ <<
    "  input values: " <<x<<","<<y<<","<<z<<","<<rX<<","<<rY<<","<<rZ<<
    "\n  Object 1 should not have moved." << std::endl;
    return 1;
  }
  
  return 0;
}
