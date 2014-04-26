#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkMatrix4x4.h>

#include <QApplication>
#include <QClipboard>

#include <structurereplicator.h>
#include <transformeditoperationstate.h>
#include <transformmanager.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <objectgroup.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

#include <test/CompareBeforeAndAfter.h>
#include <test/TestCoreHelpers.h>

int testRotateCameraYaw();
int testRotateCameraPitch();
int testSetCrystalByExampleCopies();
int testMoveAlongTimeline();

static const char ROTATE_CAMERA_OPERATION_FUNC_NAME[30] = "rotate_camera";

int main(int argc, char *argv[])
{
  QApplication app(argc,argv);
  
  int errors = 0;
  errors += testRotateCameraYaw();
  errors += testRotateCameraPitch();
  errors += testSetCrystalByExampleCopies();
  errors += testMoveAlongTimeline();
  return errors;
}

//compares values in two arrays, returns true if theyre the same
bool compareMatArrays(double *arr0, double *arr1)
{
  for (int i = 0; i < 16; i++)
  {
    if (Q_ABS(arr0[i]-arr1[i]) > Q_EPSILON)
    {
      return false;
    }
  }
  return true;
}

int testRotateCameraYaw()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  const vtkMatrix4x4 *oldRTE = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat0[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat0[i * 4 + j] = oldRTE->GetElement(i, j);
    }
  }
  
  double value = 0.8;
  ControlFunctions::rotateCameraYaw(&proj,1,value);
  
  //this should actually change the matrix
  proj.getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME)->doFrameUpdates();
  
  const vtkMatrix4x4 *newRTE = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat1[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat1[i * 4 + j] = newRTE->GetElement(i, j);
    }
  }
  
  if (compareMatArrays(mat0, mat1))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Room to eye matrix did not change on rotate" << std::endl;
    return 1;
  }
  
  return 0;
  
}

int testRotateCameraPitch()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  const vtkMatrix4x4 *oldRTE = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat0[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat0[i * 4 + j] = oldRTE->GetElement(i, j);
    }
  }
  
  double value = 0.8;
  ControlFunctions::rotateCameraPitch(&proj,1,value);
  
  //this should actually change the matrix
  proj.getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME)->doFrameUpdates();
  
  const vtkMatrix4x4 *newRTE = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat1[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat1[i * 4 + j] = newRTE->GetElement(i, j);
    }
  }
  
  if (compareMatArrays(mat0, mat1))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Room to eye matrix did not change on rotate" << std::endl;
    return 1;
  }
  
  return 0;
  
}

int testSetCrystalByExampleCopies()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector0 = Q_NULL_VECTOR, vector1 = {0.5,0.5,0.5};
  q_type orient = Q_ID_QUAT;
  
  SketchObject *obj0 = proj.getWorldManager().addObject(model, vector0, orient);
  SketchObject *obj1 = proj.getWorldManager().addObject(model, vector1, orient);
  
  //create replication
  proj.addReplication(obj0, obj1, 1);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  double nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its inside threshold
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too far from hand." << std::endl;
    return 1;
  }
  
  int numShownBefore = proj.getCrystalByExamples().at(0)->getNumShown();
  
  double value = 1;
  ControlFunctions::setCrystalByExampleCopies(&proj, 1, value);
  
  int numShownAfter = proj.getCrystalByExamples().at(0)->getNumShown();
  
  if (!(numShownAfter-numShownBefore)>0)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of replications shown did not increase." << std::endl;
    return 1;
  }
  
  q_vec_type vector2 = {10,10,-10};
  
  SketchObject *objFar = proj.getWorldManager().addObject(model, vector2, orient);
  
  //move hand over this new object at vector2
  TestCoreHelpers::setTrackerWorldPosition(proj.getTransformManager(), SketchBioHandId::RIGHT, vector2);
  handObj.updatePositionAndOrientation();
  handObj.computeNearestObjectAndConnector();
  
  if (objFar != handObj.getNearestObject())
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Neareset obj dist:  " << handObj.getNearestObjectDistance() <<
    "  This object needs to be the closest to the hand" << std::endl;
    return 1;
  }
  
  double value2 = value/2;
  
  //this shouldn't do anything
  ControlFunctions::setCrystalByExampleCopies(&proj, 1, value2);
  
  int numShownFinal = proj.getCrystalByExamples().at(0)->getNumShown();
  
  if (!(numShownAfter==numShownFinal))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Num before: " <<numShownAfter <<
    "  Num after:  " <<numShownFinal <<
    "  Number of replications should not have changed." << std::endl;
    return 1;
  }
  
  return 0;
  
}

int testMoveAlongTimeline()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  double currentTime = proj.getViewTime();
  
  double highVal =0.9;
  
  //should move in the positive direction
  ControlFunctions::moveAlongTimeline(&proj,1,highVal);
  
  double time2 = proj.getViewTime();
  
  if (time2-currentTime<=0)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Time should have increased" << std::endl;
    return 1;
  }
  
  double lowVal = 0.1;
  
  //should move in negative direction
  ControlFunctions::moveAlongTimeline(&proj, 1, lowVal);
  
  double time3 = proj.getViewTime();
  
  if (time3-time2>=0)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Time should have decreased" << std::endl;
    return 1;
  }
  
  return 0;
}