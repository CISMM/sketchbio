#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkMatrix4x4.h>

#include <QApplication>
#include <QClipboard>

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

int testCopyPaste();
int testResetViewPoint();
int testToggleCollisionChecks();
int testToggleSpringsEnabled();

bool compareMatrices(vtkMatrix4x4, vtkMatrix4x4);

int main(int argc, char *argv[])
{
  QApplication app(argc,argv);
  
  int errors = 0;
  errors += testCopyPaste();
  errors += testResetViewPoint();
  errors += testToggleCollisionChecks();
  errors += testToggleSpringsEnabled();
  return errors;
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


static const char ROTATE_CAMERA_OPERATION_FUNC_NAME[30] = "rotate_camera";

int testResetViewPoint()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  const vtkMatrix4x4 *originalRTE = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat[i * 4 + j] = originalRTE->GetElement(i, j);
    }
  }
  
  /*
   for (int i = 0; i < 16; i++) {
   cout<< mat[i] << " ";
   }
   std::cout << "     " << std::endl;*/
  
  //reset the viewpoint so we can get the matrix 
  ControlFunctions::resetViewPoint(&proj, 1, false);
  proj.getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME)->doFrameUpdates();
  const vtkMatrix4x4 *resetRTE = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat0[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat0[i * 4 + j] = resetRTE->GetElement(i, j);
    }
  }
  
  /*
  for (int i = 0; i < 16; i++) {
    cout<< mat0[i] << " ";
  }
  std::cout << "     " << std::endl;*/
  
  double moveAmt = 0.7;
  
  //rotate camera
  ControlFunctions::rotateCameraPitch(&proj, 1, moveAmt);
  proj.getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME)->doFrameUpdates();
  
  const vtkMatrix4x4 *movedRTE = proj.getTransformManager().getRoomToEyeMatrix();

  double mat1[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat1[i * 4 + j] = movedRTE->GetElement(i, j);
    }
  }
  
  /*for (int i = 0; i < 16; i++) {
    cout<< mat1[i] << " ";
  }*/
  
  if (compareMatArrays(mat0, mat1))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "RTE1: " << std::endl;
    for (int i = 0; i < 16; i++) {
        cout<< mat0[i] << " ";
    }
    std::cout << "movedRTE: " << std::endl;
    for(int i = 0; i < 16; i++) {
        cout<< mat1[i] << " ";
    }
    std::cout << "  Room to eye matrix should have changed" << std::endl;
    return 1;
  }
  
  //reset view
  ControlFunctions::resetViewPoint(&proj, 1, false);
  proj.getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME)->doFrameUpdates();
  const vtkMatrix4x4 *resetRTE2 = proj.getTransformManager().getRoomToEyeMatrix();
  
  double mat2[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat2[i * 4 + j] = resetRTE2->GetElement(i, j);
    }
  }
  
  //make sure it went back to original reset
  if (!compareMatArrays(mat0, mat2))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "RTE1: " << std::endl;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        cout<< resetRTE->GetElement(i, j) << " ";
      }
    }
    std::cout << "RTE2: " << std::endl;
    for(int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        cout<< resetRTE2->GetElement(i, j) << " ";
      }
    }
    std::cout << "  Room to eye matrix did not reset" << std::endl;
    return 1;
  }
  
  return 0;
}


int testToggleCollisionChecks()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  bool isOn = proj.getWorldManager().isCollisionTestingOn();
  
  //button pressed, should toggle
  ControlFunctions::toggleCollisionChecks(&proj, 1, true);
  
  bool isOn2=proj.getWorldManager().isCollisionTestingOn();
  
  if (isOn==isOn2)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Collision checks did not toggle" << std::endl;
    return 1;
  }
  
  //button pressed, should toggle back
  ControlFunctions::toggleCollisionChecks(&proj, 1, true);
  
  if (isOn!=proj.getWorldManager().isCollisionTestingOn())
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Collision checks did not toggle back" << std::endl;
    return 1;
  }
  
  return 0;
}

int testToggleSpringsEnabled()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  bool isOn = proj.getWorldManager().areSpringsEnabled();
  
  //button pressed, should toggle
  ControlFunctions::toggleSpringsEnabled(&proj, 1, true);
  
  bool isOn2=proj.getWorldManager().areSpringsEnabled();
  
  if (isOn==isOn2)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Springs enabled did not toggle" << std::endl;
    return 1;
  }
  
  //button pressed, should toggle back
  ControlFunctions::toggleSpringsEnabled(&proj, 1, true);
  
  if (isOn!=proj.getWorldManager().areSpringsEnabled())
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Spring enabled did not toggle back" << std::endl;
    return 1;
  }
  return 0;
}
