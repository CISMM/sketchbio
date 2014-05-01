#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <hand.h>
#include <keyframe.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <controlFunctions.h>

#include <test/TestCoreHelpers.h>

int testKeyframeAll();
int testToggleKeyframeObject();
int testAddCamera();
int testShowAnimationPreview();

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testKeyframeAll();
  errors += testToggleKeyframeObject();
  errors += testAddCamera();
  errors += testShowAnimationPreview();
  return errors;
}


int testKeyframeAll()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = Q_NULL_VECTOR;
  q_type orient = Q_ID_QUAT;
  
  //adds a few objects
  for (int i = 0; i < 5; i++){
    proj.getWorldManager().addObject(model, vector, orient);
  }
  
  proj.updateTrackerPositions();
  
  ControlFunctions::keyframeAll(&proj, 1, false);
  
  QListIterator< SketchObject * > itr =
    proj.getWorldManager().getObjectIterator();
  
  //make sure after calling keyframeAll that all objects have keyframes
  while (itr.hasNext()) {
    if(!(itr.next()->hasKeyframes())){
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  Object doesn't have keyframe after calling keyframeAll" << std::endl;
      return 1;
    };
  }
  
  return 0;
}

//in progress
int testToggleKeyframeObject()
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
  SketchObject *farObj = handObj.getNearestObject();
  double nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its outside threshold
  if (nearestObjDist < DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too close to hand." << std::endl;
    return 1;
  }
  
  //make sure control function call does nothing
  int numFrames = farObj->getNumKeyframes();
  
  ControlFunctions::toggleKeyframeObject(&proj, 1, false);
  
  int numFramesAfterTog = farObj->getNumKeyframes();
  
  //make sure num frames didn't change
  if (numFrames != numFramesAfterTog)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of keyframes changed incorrectly." << std::endl;
    return 1;
  }
  
  q_vec_type nullVec = Q_NULL_VECTOR;
  
  proj.getWorldManager().addObject(model, nullVec, orient);
  
  handObj.computeNearestObjectAndConnector();
  
  SketchObject *nearestObj = handObj.getNearestObject();
  nearestObjDist = handObj.getNearestObjectDistance();
  
  //make sure its inside threshold
  if (nearestObjDist > DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object is too far from hand." << std::endl;
    return 1;
  }
  
  //make sure parent is null
  if (nearestObj->getParent() != NULL)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object's parent is not null." << std::endl;
    return 1;
  }
  
  numFrames = nearestObj->getNumKeyframes();
  
  //should add a keyframe
  ControlFunctions::toggleKeyframeObject(&proj, 1, false);
  
  numFramesAfterTog = nearestObj->getNumKeyframes();
  
  //make sure num frames changed by exactly 1
  if (numFramesAfterTog - numFrames !=1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of keyframes changed incorrectly." << std::endl;
    return 1;
  }
  
  ColorMapType::Type cmap = nearestObj->getColorMapType();
  if (cmap == ColorMapType::SOLID_COLOR_RED)
  {
    nearestObj->setColorMapType(ColorMapType::SOLID_COLOR_BLUE);
  } else
  {
    nearestObj->setColorMapType(ColorMapType::SOLID_COLOR_RED);
  }
  
  numFrames = numFramesAfterTog;
  
  int cmapBefore = nearestObj->getKeyframes()->values().at(0).getColorMapType();
  
  //should replace the current keyframe, the object has changed
  ControlFunctions::toggleKeyframeObject(&proj, 1, false);
  
  numFramesAfterTog = nearestObj->getNumKeyframes();
  
  int cmapAfter = nearestObj->getKeyframes()->values().at(0).getColorMapType();
  
  //make sure number didn't change, and that they're both still 1.
  if (numFramesAfterTog != numFrames || numFrames !=1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of keyframes changed incorrectly." <<
    "  Num frames before:  " << numFrames <<
    "  Num frames after:  " << numFramesAfterTog
    << std::endl;
    return 1;
  }
  
  //make sure color changed
  if (cmapBefore == cmapAfter)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Keyframe not properly replaced." << std::endl;
    return 1;
  }
  
  numFrames = numFramesAfterTog;
  
  //should toggle keyframe, removing it
  ControlFunctions::toggleKeyframeObject(&proj, 1, false);
  
  numFramesAfterTog = nearestObj->getNumKeyframes();
  
  //negative 1, because now afterTog is smaller
  if (numFramesAfterTog - numFrames != -1)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of keyframes changed incorrectly." << std::endl;
    return 1;
  }
  
  //give it a parent, so the control function should do nothing
  nearestObj->setParent(farObj);
  
  numFrames = numFramesAfterTog;
  
  ControlFunctions::toggleKeyframeObject(&proj, 1, false);
  
  numFramesAfterTog = nearestObj->getNumKeyframes();
  
  if (numFrames != numFramesAfterTog)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of keyframes changed incorrectly." << std::endl;
    return 1;
  }
  
  return 0;
}

int testAddCamera()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  ControlFunctions::addCamera(&proj, 1, false);
  
  int camLength = proj.getCameras().count();
  ControlFunctions::addCamera(&proj, 1, false);
  
  int camLengthAfterAdd = proj.getCameras().count();
  
  //make sure the length of cams increased by exactly 1 after adding a camera
  if ((camLengthAfterAdd-camLength)!=1){
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Exactly one camera was not added" << std::endl;
    return 1;
  }
  
  return 0;
}

int testShowAnimationPreview()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  bool isShowingAnim = proj.isShowingAnimation();
  ControlFunctions::showAnimationPreview(&proj, 1, false);
  bool isShowingAnim2 = proj.isShowingAnimation();
  
  if (isShowingAnim==isShowingAnim2){
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Showing animation did not toggle" << std::endl;
    return 1;
  }
  
  return 0;
}
