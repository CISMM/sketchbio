#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

#include <sketchtests.h>
#include <test/TestCoreHelpers.h>

int testDeleteSpring();
int testSnapSpringToTerminus();
int testCreateSpring();
int testCreateTransparentConnector();

int main(int argc, char *argv[])
{
  int errors = 0;
  errors += testDeleteSpring();
  errors += testSnapSpringToTerminus();
  errors += testCreateSpring();
  errors += testCreateTransparentConnector();
  return errors;
}

int testDeleteSpring()
{
  vtkSmartPointer< vtkRenderer > renderer =
  vtkSmartPointer< vtkRenderer >::New();
  SketchBio::Project proj(renderer,".");
  q_vec_type pos1 = {1, 0, 0}, pos2 = Q_NULL_VECTOR;
  
  int connectorsToAdd = 5;
  
  int i = 0;
  while (i < connectorsToAdd)
  {
    SpringConnection *spring = SpringConnection::makeSpring(NULL, NULL, pos1, pos2, true, 1.0, 0, true);
    proj.getWorldManager().addConnector(spring);
    i++;
  }
    
  int connectors = proj.getWorldManager().getNumberOfConnectors();

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
  
  
  SpringConnection *spring = SpringConnection::makeSpring(NULL, NULL, pos1, pos2, true, 1.0, 0, true);
  proj.getWorldManager().addConnector(spring);
  
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
  
  //button pressed
  ControlFunctions::snapSpringToTerminus(&proj, 1, true);
  
  SketchBio::OperationState *snap = proj.getOperationState();
  snap->doFrameUpdates();
  
  q_vec_type conPos0;
  spring->getEnd1WorldPosition(conPos0);
  
  //parameter values here?
  spring->snapToTerminus(true, true);
  
  q_vec_type conPos1;
  spring->getEnd1WorldPosition(conPos1);
  
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
  
  //toggle terminus end
  ControlFunctions::setTerminusToSnapSpring(&proj, 1, true);
  
  //snap to the other terminus
  snap->doFrameUpdates();
  
  handObj.computeNearestObjectAndConnector();
  
  q_vec_type conPos2;
  spring->getEnd1WorldPosition(conPos2);
  
  if (q_vec_equals(conPos1,conPos2))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should have moved." << std::endl;
    return 1;
  }
  
  //release toggle
  ControlFunctions::setTerminusToSnapSpring(&proj, 1, false);
  
  snap->doFrameUpdates();
  
  q_vec_type conPos3;
  spring->getEnd1WorldPosition(conPos3);
  
  if (!q_vec_equals(conPos0,conPos3))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should have moved back to first position." << std::endl;
    return 1;
  }

  //button released
  ControlFunctions::snapSpringToTerminus(&proj, 1, false);
  
  q_vec_type conPosFinal0;
  spring->getEnd1WorldPosition(conPosFinal0);
  
  //shouldn't move anything
  ControlFunctions::setTerminusToSnapSpring(&proj, 1, true);
  
  q_vec_type conPosFinal1;
  spring->getEnd1WorldPosition(conPosFinal1);
  
  //should be the same since spring should not have moved
  if (!q_vec_equals(conPosFinal0,conPosFinal1))
  {
    std::cout << "Error at " << __FILE__ << ":" << __LINE__ <<
    "  Connector should have moved back to first position." << std::endl;
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
  while (i< springsToCreate)
  {
    if (i!=numSprings)
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
  while (i< connectorsToCreate)
  {
    if (i!=numConnectors)
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