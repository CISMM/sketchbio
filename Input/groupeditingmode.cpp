#include "groupeditingmode.h"

#include <string>
#include <sstream>

#include <QApplication>
#include <QClipboard>

#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

#include <sketchioconstants.h>
#include <transformmanager.h>
#include <sketchobject.h>
#include <objectgroup.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <projecttoxml.h>

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
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        emit newDirectionsString("Select an object and release the button to copy");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        emit newDirectionsString("Release the button to paste");
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
    else if (vrpn_ButtonNum == BUTTON_RIGHT(TWO_BUTTON_IDX))
    {
        if (rDist < DISTANCE_THRESHOLD)
        {
            vtkSmartPointer< vtkXMLDataElement > elem =
                    vtkSmartPointer< vtkXMLDataElement >::Take(
                        ProjectToXML::objectToClipboardXML(rObj)
                    );
            std::stringstream ss;
            vtkXMLUtilities::FlattenElement(elem,ss);
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(ss.str().c_str());
        }
        emit newDirectionsString(" ");
    }
    else if (vrpn_ButtonNum == BUTTON_RIGHT(FOUR_BUTTON_IDX))
    {
        std::stringstream ss;
        QClipboard *clipboard = QApplication::clipboard();
        ss.str(clipboard->text().toStdString());
        vtkSmartPointer< vtkXMLDataElement > elem =
                vtkSmartPointer< vtkXMLDataElement >::Take(
                    vtkXMLUtilities::ReadElementFromStream(ss)
                    );
        if (elem)
        {
            q_vec_type rpos;
            project->getTransformManager()->getRightTrackerPosInWorldCoords(rpos);
            if (ProjectToXML::objectFromClipboardXML(project,elem,rpos) ==
                    ProjectToXML::XML_TO_DATA_FAILURE)
            {
                std::cout << "Read xml correctly, but reading object failed."
                          << std::endl;
            }
        }
        else
        {
            std::cout << "Failed to read object." << std::endl;
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
                project->setRightOutlineObject(obj);
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
