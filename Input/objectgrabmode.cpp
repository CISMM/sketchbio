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
  if (project->isShowingAnimation()) // ignore grabbing during an animation preview
  {
	  return;
  }
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
  {
    SketchBio::Hand &hand = project->getHand(SketchBioHandId::LEFT);
      if (hand.getNearestObjectDistance() > DISTANCE_THRESHOLD)
      {
        hand.grabWorld();
      }
      else
      {
        hand.grabNearestObject();
      }
  }
  else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
  {
    SketchBio::Hand &hand = project->getHand(SketchBioHandId::RIGHT);
      if (hand.getNearestObjectDistance() > DISTANCE_THRESHOLD)
      {
        hand.grabWorld();
      }
      else
      {
        hand.grabNearestObject();
      }

  }
}

void ObjectGrabMode::buttonReleased(int vrpn_ButtonNum)
{
  if (project->isShowingAnimation()) // ignore grabbing during an animation preview
  {
	  return;
  }
  if (vrpn_ButtonNum == BUTTON_LEFT(BUMPER_BUTTON_IDX))
  {
    SketchBio::Hand &hand = project->getHand(SketchBioHandId::LEFT);
    hand.releaseGrabbed();
    addXMLUndoState();
  }
  else if (vrpn_ButtonNum == BUTTON_RIGHT(BUMPER_BUTTON_IDX))
  {
    SketchBio::Hand &hand = project->getHand(SketchBioHandId::RIGHT);
    hand.releaseGrabbed();
    addXMLUndoState();
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
		proj->setNearest(side,obj);
		proj->setDistance(side,distance);

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
			proj->setNearest(side,obj);
			proj->setDistance(side,distance);
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
  project->setOutlineType(SketchProject::OUTLINE_OBJECTS);
}

void ObjectGrabMode::clearStatus()
{
  worldGrabbed = WORLD_NOT_GRABBED;
  lDist = rDist = std::numeric_limits<double>::max();
  lObj = rObj = NULL;
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
