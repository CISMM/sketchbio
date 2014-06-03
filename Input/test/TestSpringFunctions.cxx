#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>
#include <OperationState.h>

#include <sketchtests.h>
#include <test/TestCoreHelpers.h>

int testDeleteSpring();
int testSnapSpringToTerminus();
int testCreateSpring();
int testCreateTransparentConnector();
int testCreateMeasuringTape();

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testDeleteSpring();
  errors += testSnapSpringToTerminus();
  errors += testCreateSpring();
  errors += testCreateTransparentConnector();
  errors += testCreateMeasuringTape();
  return errors;
}

int testDeleteSpring()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  q_vec_type pos1 = {1, 0, 0}, pos2 = Q_NULL_VECTOR;
  
  int connectorsToAdd = 5;
  
  //add a few connectors to the world
  int i = 0;
  while (i < connectorsToAdd)
  {
    SpringConnection *spring = SpringConnection::makeSpring(NULL, NULL, pos1, pos2, true, 1.0, 0, true);
    proj.getWorldManager().addConnector(spring);
    i++;
  }
    
  int connectors = proj.getWorldManager().getNumberOfConnectors();

  //make sure they were all correctly added
  if (connectors != connectorsToAdd)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Incorrect number of connectors created." << std::endl;
    return 1;
  }
  
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  
  int connectorsAfterDelete;
  double nearestConnectorDist;
  //until no more connectors, delete one and make sure the connector count dec'd by 1
  while (proj.getWorldManager().getNumberOfConnectors() != 0)
  {
    
    handObj.computeNearestObjectAndConnector();
    
    nearestConnectorDist = handObj.getNearestConnectorDistance();
    
    //make sure its within threshold
    if (nearestConnectorDist > SPRING_DISTANCE_THRESHOLD)
    {
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "nearestConnectorDist = " << nearestConnectorDist <<
      "  Connector is not close enough to hand." << std::endl;
      return 1;
    }
    
    ControlFunctions::deleteSpring(&proj, 1, false);
    connectorsAfterDelete = proj.getWorldManager().getNumberOfConnectors();
    if ((connectors - connectorsAfterDelete) !=1)
    {
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  connectors = " << connectors << "  and proj.getNumCons is "
      << proj.getWorldManager().getNumberOfConnectors() <<
      "  Number of connectors did not decrease by 1." << std::endl;
      return 1;
    }
    connectors=connectorsAfterDelete;
  }
  
  //now create a spring outside distance threshold, call delete and make sure it stays
  
  q_vec_type pos21 = {40, 40, 40}, pos22 = {40, 40, 41};
  
  SpringConnection *spring = SpringConnection::makeSpring(NULL, NULL, pos21, pos22, true, 1.0, 0, true);
  proj.getWorldManager().addConnector(spring);
  
  handObj.computeNearestObjectAndConnector();
  
  nearestConnectorDist = handObj.getNearestConnectorDistance();
  
  //make sure its outside threshold
  if (nearestConnectorDist < SPRING_DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "nearestConnectorDist = " << nearestConnectorDist <<
    "\ndist threshold:" << SPRING_DISTANCE_THRESHOLD <<
    "  Connector is too close to hand." << std::endl;
    return 1;
  }
  
  connectors = proj.getWorldManager().getNumberOfConnectors();
  
  ControlFunctions::deleteSpring(&proj, 1, false);
  connectorsAfterDelete = proj.getWorldManager().getNumberOfConnectors();
  
  //make sure delete did not decrease number of connectors!
  if (connectors != connectorsAfterDelete)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Number of connectors changed incorrectly" <<
    "\nconnectors before:  " <<connectors<<
    "\nconnectors after delete:  " <<connectorsAfterDelete<<
    std::endl;
    return 1;
  }
  
  return 0;
  
}

int testSnapSpringToTerminus()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  q_vec_type pos1 = {0,0,0}, pos2 = {0,0,1};
  q_vec_type nTerm = {-1,-1,-1}, cTerm = {1,1,1};
  SketchModel *model = TestCoreHelpers::getCubeModel();
  proj.getModelManager().addModel(model);
  q_vec_type vector = {0,0,0};
  q_type orient = Q_ID_QUAT;
  
  SketchObject * obj1 = proj.getWorldManager().addObject(model, vector, orient);
  
  SpringConnection *spring = SpringConnection::makeSpring(NULL, NULL, pos1, pos2, true, 1.0, 0, true);
  proj.getWorldManager().addConnector(spring);
  
  //connects springs to object
  spring->setObject1(obj1);
  
  SketchBio::Hand &handObj = proj.getHand(SketchBioHandId::RIGHT);
  handObj.computeNearestObjectAndConnector();
  
  double nearestConnectorDist = handObj.getNearestConnectorDistance();
  
  //make sure its within threshold
  if (nearestConnectorDist > SPRING_DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "nearestConnectorDist = " << nearestConnectorDist <<
    "  Connector is too far from hand." << std::endl;
    return 1;
  }
  
  //button pressed, sets operation state
  ControlFunctions::snapSpringToTerminus(&proj, 1, true);
  
  // a bit hacky, but I know what the constant is...
  SketchBio::OperationState *snap = proj.getOperationState("snap_spring");
  //should actually snap spring to n terminus
  snap->doFrameUpdates();
  
  q_vec_type conPos0;
  //should be 1,1,1 I think
  spring->getObject1ConnectionPosition(conPos0);
  
  //check the connector position is the same as the n terminus position
  if (!q_vec_equals(conPos0,nTerm))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should be at n terminus." << std::endl;
    return 1;
  }
  
  //second true snaps to n, but it's already at the n terminus so it shouldn't change
  spring->snapToTerminus(true, true);
  
  q_vec_type conPos1;
  spring->getObject1ConnectionPosition(conPos1);
  
  //make sure it didn't move (still at n terminus)
  if (!q_vec_equals(conPos0,conPos1))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should not have moved." << std::endl;
    return 1;
  }
  
  //clear state with button release
  ControlFunctions::snapSpringToTerminus(&proj, 1, false);
  
  if (handObj.getNearestConnectorDistance() > SPRING_DISTANCE_THRESHOLD)
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector too far from hand." << std::endl;
    return 1;
  }
  
  //button pressed, creates operation state
  ControlFunctions::snapSpringToTerminus(&proj, 1, true);
  snap = proj.getOperationState("snap_spring");
  
  //toggle terminus end to c terminus
  ControlFunctions::setTerminusToSnapSpring(&proj, 1, true);
  
  //snap to the c terminus
  snap->doFrameUpdates();
  
  handObj.computeNearestObjectAndConnector();
  
  q_vec_type conPos2;
  spring->getObject1ConnectionPosition(conPos2);
  
  //make sure its at the c terminus
  if (!q_vec_equals(conPos2,cTerm))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should have moved to c terminus." << std::endl;
    return 1;
  }
  
  //release toggle
  ControlFunctions::setTerminusToSnapSpring(&proj, 1, false);
  
  //should snap back to n terminus
  snap->doFrameUpdates();
  
  q_vec_type conPos3;
  spring->getObject1ConnectionPosition(conPos3);
  
  //check the connector is at the n terminus
  if (!q_vec_equals(nTerm,conPos3))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should have moved back to n terminus." << std::endl;
    return 1;
  }

  //button released, clears state
  ControlFunctions::snapSpringToTerminus(&proj, 1, false);
  
  q_vec_type conPosFinal0;
  spring->getObject1ConnectionPosition(conPosFinal0);
  
  //shouldn't move anything, there's no state
  ControlFunctions::setTerminusToSnapSpring(&proj, 1, true);
  
  q_vec_type conPosFinal1;
  spring->getObject1ConnectionPosition(conPosFinal1);
  
  //should be the same since spring should not have moved
  if (!q_vec_equals(conPosFinal0,conPosFinal1))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should not have moved." << std::endl;
    return 1;
  }
  
  return 0;
}


int testCreateSpring()
{
  
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  int springsToCreate = 5;
  int numSprings = proj.getWorldManager().getNumberOfConnectors();
  
  int i = 0;
  
  //create some springs, make sure the number of connectors goes up by 1 every time
  while (i < springsToCreate)
  {
    if (i != numSprings)
    {
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  Number of springs did not increase by 1." << std::endl;
      return 1;
    }
    ControlFunctions::createSpring(&proj, 1, false);
    numSprings = proj.getWorldManager().getNumberOfConnectors();
    i++;
  }
  
  return 0;
}

int testCreateTransparentConnector()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  int connectorsToCreate = 5;
  int numConnectors = proj.getWorldManager().getNumberOfConnectors();
  
  int i = 0;
  
  //create some springs, make sure the number of connectors goes up by 1 every time
  while (i < connectorsToCreate)
  {
    if (i != numConnectors)
    {
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  Number of connectors did not increase by 1." << std::endl;
      return 1;
    }
    ControlFunctions::createTransparentConnector(&proj, 1, false);
    numConnectors = proj.getWorldManager().getNumberOfConnectors();
    i++;
  }
  
  return 0;
}

int testCreateMeasuringTape()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  
  int connectorsToCreate = 5;
  int numConnectors = proj.getWorldManager().getNumberOfConnectors();
  int numActors = renderer->VisibleActorCount();
  
  int i = 0;
  
  //create some springs, make sure the number of connectors goes up by 1 every time
  while (i < connectorsToCreate)
  {
    if (i != numConnectors)
    {
      std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  Number of connectors did not increase by 1." << std::endl;
      return 1;
    }
	if (2*i != numActors) {
	  std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
      "  Number of actors did not increase by 2." << std::endl;
      return 1;
	}
    ControlFunctions::createMeasuringTape(&proj,1, false);
    numConnectors = proj.getWorldManager().getNumberOfConnectors();
	numActors = renderer->VisibleActorCount();
    i++;
  }
  
  return 0;
}