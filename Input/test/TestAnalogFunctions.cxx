#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

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

int testRotateCameraYaw()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = {10,10,10};
  q_type orient = Q_ID_QUAT;
  
}
