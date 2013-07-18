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
            ObjectGroup *grp = dynamic_cast<ObjectGroup *>(lObj);
            if (grp == NULL)
            {
                grp = new ObjectGroup();
                world->removeObject(lObj);
                grp->addObject(lObj);
                world->addObject(grp);
            }
            world->removeObject(rObj);
            grp->addObject(rObj);
        }
        emit newDirectionsString(" ");
    }
}

void GroupEditingMode::analogsUpdated()
{
}
