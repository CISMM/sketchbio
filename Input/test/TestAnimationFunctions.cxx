#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

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
  SketchProject proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager()->addModel(model);
  q_vec_type vector = Q_NULL_VECTOR;
  q_type orient = Q_ID_QUAT;
  
  //adds a few objects
  for (int i = 0; i < 5; i++){
    //can you add multiple objects using the same model??? ask shawn!!
    proj.getWorldManager()->addObject(model, vector, orient);
  }
  
  proj.updateTrackerPositions();
  
  ControlFunctions::keyframeAll(&proj, 1, false);
  
  QListIterator< SketchObject * > itr =
    proj.getWorldManager()->getObjectIterator();
  
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

//!!! INCOMPLETE !!!//
int testToggleKeyframeObject()
{
  return 0;
}

int testAddCamera()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchProject proj(renderer,".");
  
  ControlFunctions::addCamera(&proj, 1, false);
  
  const QHash< SketchObject* , vtkSmartPointer< vtkCamera > >* cams = proj.getCameras();
  
  int camLength = cams->count();
  ControlFunctions::addCamera(&proj, 1, false);
  
  int camLengthAfterAdd = cams->count();
  
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
  SketchProject proj(renderer,".");
  
  bool isShowingAnim = proj.isShowingAnimation();
  ControlFunctions::showAnimationPreview(&proj, 1, false);
  bool isShowingAnim2 = proj.isShowingAnimation();
  
  if (isShowingAnim==isShowingAnim2){
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Showing animation did not toogle" << std::endl;
    return 1;
  }
  
  return 0;
}