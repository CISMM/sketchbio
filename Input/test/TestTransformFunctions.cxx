#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <controlFunctions.h>

#include <test/TestCoreHelpers.h>

int testDeleteObject();
int testReplicateObject();
int testSetTransforms();

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testDeleteObject();
  errors += testReplicateObject();
  errors += testSetTransforms();
  return errors;
}

int testDeleteObject()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchProject proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager()->addModel(model);
  q_vec_type vector = Q_NULL_VECTOR;
  q_type orient = Q_ID_QUAT;
  
  //this obj is inside distance threshold
  proj.getWorldManager()->addObject(model, vector, orient);
  
  q_vec_set(vector,10,10,10);
  
  //this obj is outside distance threshold
  proj.getWorldManager()->addObject(model, vector, orient);
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
  
  //!!! ask Shawn if this is necessary... I think it is
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
  return 0;
}

int testSetTransforms()
{
  return 0;
}