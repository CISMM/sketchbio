#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <transformeditoperationstate.h>
#include <transformmanager.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

#include <test/TestCoreHelpers.h>

int testResetViewPoint();

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testResetViewPoint();
  return errors;
}


int testResetViewPoint()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");

  TransformManager &t = proj.getTransformManager();
  
  t.getRoomToEyeTransform();
  
  //TransformEditOperationState::setObjectTransform(<#double#>, <#double#>, <#double#>, <#double#>, <#double#>, <#double#>);
  
  return 0;
}
