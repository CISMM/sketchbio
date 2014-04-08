#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

#include <test/TestCoreHelpers.h>

int testChangeObjectColor();
int testChangeObjectColorVariable();
int testToggleObjectVisibility();
int testToggleShowInvisibleObjects();

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testChangeObjectColor();
  errors += testChangeObjectColorVariable();
  errors += testToggleObjectVisibility();
  errors += testToggleShowInvisibleObjects();
  return errors;
}

//also test nearest connector in the case that object is too far away
//test when both connector and object are there
int testChangeObjectColor()
{
  vtkSmartPointer< vtkRenderer > renderer =
    vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = Q_NULL_VECTOR;
  q_type orient = Q_ID_QUAT;
  SketchObject *obj = proj.getWorldManager().addObject(model, vector, orient);
  proj.updateTrackerPositions();
  
  //given that we know there are 7 different colors
  ColorMapType::Type t = obj->getColorMapType();
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type t2 = obj->getColorMapType();
  if (t == t2) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type t3 = obj->getColorMapType();
  if (t == t3 || t2==t3) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type t4 = obj->getColorMapType();
  if (t == t4 || t2==t4 || t3==t4) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type t5 = obj->getColorMapType();
  if (t == t5 || t2==t5 || t3==t5 || t4==t5) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type t6 = obj->getColorMapType();
  if (t == t6 || t2==t6 || t3==t6 || t4==t6 || t5==t6) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type t7 = obj->getColorMapType();
  if (t == t7 || t2==t7 || t3==t7 || t4==t7 || t5==t7 || t6==t7) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color was repeated too soon." << std::endl;
    return 1;
  }
  //end of the loop of colors, should be the same again
  ControlFunctions::changeObjectColor(&proj, 1, false);
  ColorMapType::Type tfinal = obj->getColorMapType();
  if (t != tfinal) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object color did not cycle through all 7 colors." << std::endl;
    return 1;
  }
  
  //case where nearest object distant less than threshold, but connector isnt
  q_vec_type pos1 = {0,0,0};
  q_vec_type pos2 = {3,0,0};
  Connector *c = new Connector(NULL,NULL,pos1,pos2);
  proj.getWorldManager().addConnector(c);
  q_vec_set(vector,10,10,10);
  obj->setPosition(vector);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  double nearestObjDist = handObj.getNearestObjectDistance();
  double nearestConnectorDist = handObj.getNearestConnectorDistance();
  
  if (nearestObjDist < DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Object needs to be moved further away for test to work" << std::endl;
    return 1;
  }
  if (proj.getWorldManager().getNumberOfConnectors() == 0) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector not added to world." << std::endl;
    return 1;
  }
  if (!(nearestConnectorDist < SPRING_DISTANCE_THRESHOLD))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector needs to be moved closer for test to work: "<<nearestConnectorDist << std::endl;
    return 1;
  }
  
  //nearest obj (obj) is outside distance threshold, so changeObjectColor should change the color of the nearest connector (c)
  
  //same test as above, now on c
  t = c->getColorMapType();
  ControlFunctions::changeObjectColor(&proj, 1, false);
  t2 = c->getColorMapType();
  if (t == t2) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  t3 = c->getColorMapType();
  if (t == t3 || t2==t3) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  t4 = c->getColorMapType();
  if (t == t4 || t2==t4 || t3==t4) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  t5 = c->getColorMapType();
  if (t == t5 || t2==t5 || t3==t5 || t4==t5) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  t6 = c->getColorMapType();
  if (t == t6 || t2==t6 || t3==t6 || t4==t6 || t5==t6) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color was repeated too soon." << std::endl;
    return 1;
  }
  ControlFunctions::changeObjectColor(&proj, 1, false);
  t7 = c->getColorMapType();
  if (t == t7 || t2==t7 || t3==t7 || t4==t7 || t5==t7 || t6==t7) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color was repeated too soon." << std::endl;
    return 1;
  }
  //end of the loop of colors, should be the same again
  ControlFunctions::changeObjectColor(&proj, 1, false);
  tfinal = c->getColorMapType();
  if (t != tfinal) {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector color did not cycle through all 7 colors." << std::endl;
    return 1;
  }
  
  
  return 0;
}

int testChangeObjectColorVariable()
{
  return 0;
}

int testToggleObjectVisibility()
{
  return 0;
}

int testToggleShowInvisibleObjects()
{
  return 0;
}
