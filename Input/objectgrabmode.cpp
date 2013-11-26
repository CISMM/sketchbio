#include "objectgrabmode.h"

#include <limits>
#include <cmath>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>

ObjectGrabMode::ObjectGrabMode(SketchProject *proj, const bool * const b,
                               const double * const a) :
    HydraInputMode(proj,b,a),
    worldGrabbed(WORLD_NOT_GRABBED),
    lDist(std::numeric_limits<double>::max()),
    rDist(std::numeric_limits<double>::max()),
    lObj(NULL),
    rObj(NULL)
{
}

ObjectGrabMode::~ObjectGrabMode()
{
}

void ObjectGrabMode::buttonPressed(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
  {
      if (lDist > DISTANCE_THRESHOLD)
      {
          if (worldGrabbed == WORLD_NOT_GRABBED)
              worldGrabbed = LEFT_GRABBED_WORLD;
      }
      else
      {
          project->grabObject(lObj,true);
      }
  }
  else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
  {
      if (rDist > DISTANCE_THRESHOLD)
      {
          if (worldGrabbed == WORLD_NOT_GRABBED)
              worldGrabbed = RIGHT_GRABBED_WORLD;
      }
      else
      {
          project->grabObject(rObj,false);
      }

  }
}

void ObjectGrabMode::buttonReleased(int vrpn_ButtonNum)
{
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
  {
      if (worldGrabbed == LEFT_GRABBED_WORLD)
      {
          worldGrabbed = WORLD_NOT_GRABBED;
      }
      else if (!project->getWorldManager()->getLeftSprings()->empty())
      {
          project->getWorldManager()->clearLeftHandSprings();
          addXMLUndoState();
      }
  }
  else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
  {
      if (worldGrabbed == RIGHT_GRABBED_WORLD)
      {
          worldGrabbed = WORLD_NOT_GRABBED;
      }
      else if (!project->getWorldManager()->getRightSprings()->empty())
      {
          project->getWorldManager()->clearRightHandSprings();
          addXMLUndoState();
      }
  }
}

inline void computeClosestObject(SketchProject *proj,
                                 SketchObject * & obj, // reference to pointer
                                 SketchObject * & baseObj, // reference to pointer
                                 double &distance,
                                 int &level,
                                 SketchObject *trackerObject,
                                 int side)
{
    bool oldOutlinesShown = proj->isOutlineVisible(side);
    SketchObject *closest = proj->getWorldManager()->getClosestObject(
                trackerObject,distance);

    if (baseObj != closest) {
        proj->setOutlineObject(side,closest);
        baseObj = obj = closest;
        level = 0;
    }
    else
    {
        SketchObject *levelObj = baseObj;
        double dist = distance;
        if (distance < DISTANCE_THRESHOLD)
        {
            for (int i = 1; i <= level; i++)
            {
                QList<SketchObject *> *subObjs = levelObj->getSubObjects();
                if (subObjs != NULL)
                {
                    closest = WorldManager::getClosestObject(
                                *subObjs,trackerObject,dist);
                }
                else
                {
                    level = i - 1;
                    break;
                }
                if (dist < DISTANCE_THRESHOLD)
                {
                    levelObj = closest;
                }
                else
                {
                    level = i;
                    break;
                }
            }
            if (obj != levelObj)
            {
                proj->setOutlineObject(side,levelObj);
            }
            obj = levelObj;
            distance = dist;
        }
        else
        {
            level = 0;
        }
    }
    if (distance < DISTANCE_THRESHOLD) {
        if (!oldOutlinesShown) {
            proj->setOutlineVisible(side,true);
        }
    } else if (oldOutlinesShown) {
        proj->setOutlineVisible(side,false);
    }
}

void ObjectGrabMode::doUpdatesForFrame()
{
  if (worldGrabbed == LEFT_GRABBED_WORLD)
      grabWorldWithLeft();
  else if (worldGrabbed == RIGHT_GRABBED_WORLD)
      grabWorldWithRight();
  WorldManager *world = project->getWorldManager();
  SketchObject *leftHand = project->getLeftHandObject();
  SketchObject *rightHand = project->getRightHandObject();

  // we don't want to show bounding boxes during animation
  if (world->getNumberOfObjects() > 0) {

      if (world->getLeftSprings()->size() == 0 ) {
          computeClosestObject(project,lObj,lBase,lDist,
                               leftLevel,leftHand,LEFT_SIDE_OUTLINE);
      }

      if (world->getRightSprings()->size() == 0 ) {
          computeClosestObject(project,rObj,rBase,rDist,
                               rightLevel,rightHand,RIGHT_SIDE_OUTLINE);
      }
  }
}

void ObjectGrabMode::clearStatus()
{
  worldGrabbed = WORLD_NOT_GRABBED;
  lDist = rDist = std::numeric_limits<double>::max();
  lObj = rObj = NULL;
  project->getWorldManager()->clearLeftHandSprings();
  project->getWorldManager()->clearRightHandSprings();
  resetViewPoint();
}

void ObjectGrabMode::analogsUpdated()
{
    if (!bumpLevels && std::abs(analogStatus[ANALOG_RIGHT(UP_DOWN_ANALOG_IDX)]) < 0.25)
    {
        bumpLevels = true;
    }
    else if (bumpLevels && analogStatus[ANALOG_RIGHT(UP_DOWN_ANALOG_IDX)] > 0.25)
    {
        bumpLevels = false;
        leftLevel++;
        rightLevel++;
    }
    else if (bumpLevels && analogStatus[ANALOG_RIGHT(UP_DOWN_ANALOG_IDX)] < -0.25)
    {
        bumpLevels = false;
        if (leftLevel > 0)
        {
            leftLevel--;
        }
        if (rightLevel > 0)
        {
            rightLevel--;
        }
    }
}
