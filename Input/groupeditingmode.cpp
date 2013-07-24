#include "groupeditingmode.h"

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <objectgroup.h>
#include <worldmanager.h>
#include <sketchproject.h>

GroupEditingMode::GroupEditingMode(SketchProject *proj, bool const * const b,
                                   double const * const a) :
    ObjectGrabMode(proj,b,a)
{
}

GroupEditingMode::~GroupEditingMode()
{
}

void GroupEditingMode::buttonPressed(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonPressed(vrpn_ButtonNum);
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        emit newDirectionsString("Select the group with the left hand\n"
                                 "and the object to add/remove with the right hand");
    }
}

void GroupEditingMode::buttonReleased(int vrpn_ButtonNum)
{
    ObjectGrabMode::buttonReleased(vrpn_ButtonNum);
    WorldManager *world = project->getWorldManager();
    if (vrpn_ButtonNum == BUTTON_RIGHT(ONE_BUTTON_IDX))
    {
        if (lDist < DISTANCE_THRESHOLD && rDist < DISTANCE_THRESHOLD)
        {
            ObjectGroup *grp = dynamic_cast< ObjectGroup * >(lObj);
            if (lObj == rObj)
            {
                if (grp == NULL)
                    return;
                SketchObject *rHand = project->getRightHandObject();
                SketchObject *obj = WorldManager::getClosestObject(
                            *grp->getSubObjects(),rHand,rDist);
                if (rDist < DISTANCE_THRESHOLD)
                {
                    grp->removeObject(obj);
                    world->addObject(obj);
                }
            }
            else
            {
                if (grp == NULL ||
                        (grp != NULL &&
                         dynamic_cast< ObjectGroup * >(rObj) != NULL ))
                {
                    grp = new ObjectGroup();
                    world->removeObject(lObj);
                    grp->addObject(lObj);
                    world->addObject(grp);
                }
                world->removeObject(rObj);
                grp->addObject(rObj);
            }
        }
        emit newDirectionsString(" ");
    }
}

void GroupEditingMode::analogsUpdated()
{
}

void GroupEditingMode::doUpdatesForFrame()
{
    ObjectGrabMode::doUpdatesForFrame();
    bool givenNewDirections = false;
    if (lDist < DISTANCE_THRESHOLD && rDist < DISTANCE_THRESHOLD &&
            isButtonDown[BUTTON_RIGHT(ONE_BUTTON_IDX)])
    {
        ObjectGroup *lGrp = dynamic_cast< ObjectGroup * >(lObj);
        ObjectGroup *rGrp = dynamic_cast< ObjectGroup * >(rObj);
        if (lObj == rObj && lGrp != NULL)
        {
            SketchObject *obj = WorldManager::getClosestObject(
                        *rGrp->getSubObjects(),
                        project->getRightHandObject(),rDist);
            if (rDist < DISTANCE_THRESHOLD)
            {
                emit newDirectionsString("Release to remove object from group");
                givenNewDirections = true;
            }
        }
        else if (lObj != rObj)
        {
            if (lGrp != NULL && rGrp != NULL)
            {
                emit newDirectionsString("Release to join groups into one");
                givenNewDirections = true;
            }
            else
            {
                emit newDirectionsString("Release to join objects into group");
                givenNewDirections = true;
            }
        }
    }
    if ( isButtonDown[BUTTON_RIGHT(ONE_BUTTON_IDX)] && !givenNewDirections )
    {
        emit newDirectionsString("Select the group with the left hand\n"
                                 "and the object to add/remove with the right hand");
    }
}
