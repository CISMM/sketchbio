#include "controlFunctions.h"
#include "savedxmlundostate.h"

#include <sstream>

#include <QApplication>
#include <QClipboard>

#include <vtkXMLUtilities.h>
#include <vtkXMLDataElement.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkTransform.h>
#include <vtkSmartPointer.h>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <sketchproject.h>
#include <connector.h>
#include <keyframe.h>
#include <objectgroup.h>
#include <projecttoxml.h>
#include <springconnection.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <hand.h>

#include "TransformInputDialog.h"
#include "transformeditoperationstate.h"

static const double DEFAULT_SPRING_STIFFNESS = 1.0;

static inline void addXMLUndoState(SketchBio::Project *project)
{
    UndoState *last = project->getLastUndoState();
    SavedXMLUndoState *lastXMLState = dynamic_cast< SavedXMLUndoState * >(last);
    SavedXMLUndoState *newXMLState = NULL;
    if (lastXMLState != NULL) {
        QSharedPointer< std::string > lastState =
            lastXMLState->getAfterState().toStrongRef();
        newXMLState = new SavedXMLUndoState(*project, lastState);
        if (lastXMLState->getBeforeState().isNull()) {
            project->popUndoState();
        }
    } else {
        QSharedPointer< std::string > lastStr(NULL);
        newXMLState = new SavedXMLUndoState(*project, lastStr);
    }
    project->addUndoState(newXMLState);
}

void TransformEditOperationState::setObjectTransform(double x, double y,
                                                     double z, double rX,
                                                     double rY, double rZ)
{
    assert(objs.size() == 2);
    vtkSmartPointer< vtkTransform > transform =
        vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    transform->Concatenate(objs[0]->getLocalTransform());
    transform->RotateY(rY);
    transform->RotateX(rX);
    transform->RotateZ(rZ);
    transform->Translate(x, y, z);
    q_vec_type pos;
    q_type orient;
    SketchObject::getPositionAndOrientationFromTransform(transform, pos,
                                                         orient);
    if (objs[1]->getParent() != NULL) {
        SketchObject::setParentRelativePositionForAbsolutePosition(
            objs[1], objs[1]->getParent(), pos, orient);
    } else {
        objs[1]->setPosAndOrient(pos, orient);
    }
    // clears operation state and calls deleteLater
    cancelOperation();
}

void TransformEditOperationState::cancelOperation()
{
    proj->setOperationState(NULL);
}

namespace ControlFunctions
{
// ===== BEGIN ANIMATION FUNCTIONS =====

void keyframeAll(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections("Release to keyframe all objects.");
    } else  // button released
    {
        double time = project->getViewTime();
        QListIterator< SketchObject * > itr =
            project->getWorldManager().getObjectIterator();
        while (itr.hasNext()) {
            itr.next()->addKeyframeForCurrentLocation(time);
        }
        project->getWorldManager().setAnimationTime(time);
        project->clearDirections();
        addXMLUndoState(project);
    }
    return;
}

void toggleKeyframeObject(SketchBio::Project *project, int hand,
                          bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Move to an object and release to add or remove a keyframe\nfor "
            "the current location and time.");
    } else  // button released
    {

        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        SketchObject *nearestObj = handObj.getNearestObject();
        double nearestObjDist = handObj.getNearestObjectDistance();

        SketchObject *nearestObjParent = nearestObj->getParent();

        if (nearestObjDist < DISTANCE_THRESHOLD && nearestObjParent == NULL) {
            // add/update/remove keyframe
            double time = project->getViewTime();
            bool newFrame = nearestObj->hasChangedSinceKeyframe(time);
            if (newFrame) {
                nearestObj->addKeyframeForCurrentLocation(time);
            } else {
                nearestObj->removeKeyframeForTime(time);
            }
            // make sure the object doesn't move just because we removed
            // the keyframe... principle of least astonishment.
            // using this instead of setAnimationTime since it doesn't update
            // positions
            project->getWorldManager().setKeyframeOutlinesForTime(time);
            addXMLUndoState(project);
            project->clearDirections();
        } else if (nearestObjParent != NULL) {
            project->setDirections(
                "Cannot keyframe objects in a group individually,\n"
                "keyframe the group and it will save the state of all of "
                "them.");
        } else {
            project->clearDirections();
        }
    }
    return;
}

void addCamera(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Release to add a camera at the current location.");
    } else  // button released
    {
        q_vec_type pos;
        q_type orient;
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        handObj.getPosition(pos);
        handObj.getOrientation(orient);
        project->addCamera(pos, orient);
        addXMLUndoState(project);
        project->clearDirections();
    }
    return;
}

void showAnimationPreview(SketchBio::Project *project, int hand,
                          bool wasPressed)
{
    if (wasPressed) {
        if (project->isShowingAnimation()) {
            project->setDirections("Release to stop the animation preview.");
        } else {
            project->setDirections("Release to preview the animation.");
        }
    } else {  // button released
        if (project->isShowingAnimation()) {
            project->stopAnimation();
        } else {
            // play sample animation
            project->startAnimation();
        }
        project->clearDirections();
    }
    return;
}

// ===== END ANIMATION FUNCTIONS =====

// ===== BEGIN GROUP EDITING FUNCTIONS =====

void toggleGroupMembership(SketchBio::Project *project, int hand,
                           bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Select the group with the left hand\n"
            "and the object to add/remove with the right hand");
    } else {  // button released
        WorldManager &world = project->getWorldManager();

        SketchBio::Hand &handObj0 = project->getHand(
            hand == 1 ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        SketchObject *nearestObj0 = handObj0.getNearestObject();
        double nearestObjDist0 = handObj0.getNearestObjectDistance();

        SketchBio::Hand &handObj1 = project->getHand(
            hand == 1 ? SketchBioHandId::RIGHT : SketchBioHandId::LEFT);

        SketchObject *nearestObj1 = handObj1.getNearestObject();
        double nearestObjDist1 = handObj1.getNearestObjectDistance();

        if (nearestObjDist0 < DISTANCE_THRESHOLD &&
            nearestObjDist1 < DISTANCE_THRESHOLD) {
            ObjectGroup *grp = dynamic_cast< ObjectGroup * >(nearestObj0);
            if (nearestObj0 == nearestObj1) {
                return;
            } else if (nearestObj0 == nearestObj1->getParent()) {
                assert(grp != NULL);
                grp->removeObject(nearestObj1);
                world.addObject(nearestObj1);
            } else {
                if (grp == NULL ||
                    (grp != NULL &&
                     dynamic_cast< ObjectGroup * >(nearestObj1) != NULL)) {
                    grp = new ObjectGroup();
                    world.removeObject(nearestObj0);
                    grp->addObject(nearestObj0);
                    world.addObject(grp);
                }
                world.removeObject(nearestObj1);
                grp->addObject(nearestObj1);
            }
            addXMLUndoState(project);
        }
        project->clearDirections();
    }
    return;
}

void copyObject(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Select an object and release the button to copy");
    } else  // button released
    {
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        SketchObject *nearestObj = handObj.getNearestObject();
        double nearestObjDist = handObj.getNearestObjectDistance();

        if (nearestObjDist < DISTANCE_THRESHOLD) {
            vtkSmartPointer< vtkXMLDataElement > elem =
                vtkSmartPointer< vtkXMLDataElement >::Take(
                    ProjectToXML::objectToClipboardXML(nearestObj));
            std::stringstream ss;
            vtkXMLUtilities::FlattenElement(elem, ss);
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(ss.str().c_str());
        }
        project->clearDirections();
    }
    return;
}

void pasteObject(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections("Release the button to paste");
    } else  // button released
    {
        std::stringstream ss;
        QClipboard *clipboard = QApplication::clipboard();
        ss.str(clipboard->text().toStdString());
        vtkSmartPointer< vtkXMLDataElement > elem =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                vtkXMLUtilities::ReadElementFromStream(ss));
        if (elem) {
            q_vec_type rpos;
            project->getHand((hand == 0)
                                 ? SketchBioHandId::LEFT
                                 : SketchBioHandId::RIGHT).getPosition(rpos);
            if (ProjectToXML::objectFromClipboardXML(project, elem, rpos) ==
                ProjectToXML::XML_TO_DATA_FAILURE) {
                std::cout << "Read xml correctly, but reading object failed."
                          << std::endl;
            } else {
                addXMLUndoState(project);
            }
        } else {
            std::cout << "Failed to read object." << std::endl;
        }
        project->clearDirections();
    }
    return;
}

// ===== END GROUP EDITING FUNCTIONS =====

// ===== BEGIN COLOR EDITING FUNCTIONS =====

void changeObjectColor(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Move to the object and release to change\n"
            " the color map of the object");
    } else  // button released
    {
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        SketchObject *nearestObj = handObj.getNearestObject();
        double nearestObjDist = handObj.getNearestObjectDistance();

        Connector *nearestSpring = handObj.getNearestConnector(NULL);
        double nearestSpringDist = handObj.getNearestConnectorDistance();

        if (nearestObjDist < DISTANCE_THRESHOLD &&
            !nearestObj->getArrayToColorBy().isEmpty()) {
            ColorMapType::Type cmap = nearestObj->getColorMapType();
            using namespace ColorMapType;
            switch (cmap) {
                case SOLID_COLOR_RED:
                    cmap = SOLID_COLOR_GREEN;
                    break;
                case SOLID_COLOR_GREEN:
                    cmap = SOLID_COLOR_BLUE;
                    break;
                case SOLID_COLOR_BLUE:
                    cmap = SOLID_COLOR_YELLOW;
                    break;
                case SOLID_COLOR_YELLOW:
                    cmap = SOLID_COLOR_PURPLE;
                    break;
                case SOLID_COLOR_PURPLE:
                    cmap = SOLID_COLOR_CYAN;
                    break;
                case SOLID_COLOR_CYAN:
                    cmap = BLUE_TO_RED;
                    break;
                case BLUE_TO_RED:
                    cmap = SOLID_COLOR_RED;
                    break;
                default:
                    cmap = SOLID_COLOR_RED;
                    break;
            }
            nearestObj->setColorMapType(cmap);
            addXMLUndoState(project);
        } else if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD) {
            ColorMapType::Type cmap = nearestSpring->getColorMapType();
            using namespace ColorMapType;
            switch (cmap) {
                case SOLID_COLOR_RED:
                    cmap = SOLID_COLOR_GREEN;
                    break;
                case SOLID_COLOR_GREEN:
                    cmap = SOLID_COLOR_BLUE;
                    break;
                case SOLID_COLOR_BLUE:
                    cmap = SOLID_COLOR_YELLOW;
                    break;
                case SOLID_COLOR_YELLOW:
                    cmap = SOLID_COLOR_PURPLE;
                    break;
                case SOLID_COLOR_PURPLE:
                    cmap = SOLID_COLOR_CYAN;
                    break;
                case SOLID_COLOR_CYAN:
                    cmap = SOLID_COLOR_GRAY;
                    break;
                case SOLID_COLOR_GRAY:
                    cmap = SOLID_COLOR_RED;
                    break;
                default:
                    cmap = SOLID_COLOR_GRAY;
                    break;
            }
            nearestSpring->setColorMapType(cmap);
            addXMLUndoState(project);
        }
        project->clearDirections();
    }
    return;
}

void changeObjectColorVariable(SketchBio::Project *project, int hand,
                               bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Move to the object and release to change\n"
            " the variable the object is colored by.");
    } else  // button released
    {
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        SketchObject *nearestObj = handObj.getNearestObject();
        double nearestObjDist = handObj.getNearestObjectDistance();

        if (nearestObjDist < DISTANCE_THRESHOLD) {
            QString arr(nearestObj->getArrayToColorBy());
            if (arr == QString("modelNum")) {
                arr = QString("chainPosition");
            } else if (arr == QString("chainPosition")) {
                arr = QString("charge");
            } else if (arr == QString("charge")) {
                arr = QString("modelNum");
            }
            nearestObj->setArrayToColorBy(arr);
            addXMLUndoState(project);
        }
        project->clearDirections();
    }
    return;
}

void toggleObjectVisibility(SketchBio::Project *project, int hand,
                            bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Move to an object and release to toggle "
            "object visibility.");
    } else  // button released
    {

        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        SketchObject *nearestObj = handObj.getNearestObject();
        double nearestObjDist = handObj.getNearestObjectDistance();

        if (nearestObjDist < DISTANCE_THRESHOLD) {
            // toggle object visible
            SketchObject::setIsVisibleRecursive(nearestObj,
                                                !nearestObj->isVisible());
            addXMLUndoState(project);
        }
        project->clearDirections();
    }
    return;
}

void toggleShowInvisibleObjects(SketchBio::Project *project, int hand,
                                bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Release to toggle whether invisible objects are shown.");
    } else  // button released
    {
        // toggle invisible objects shown
        WorldManager &world = project->getWorldManager();
        if (world.isShowingInvisible())
            world.hideInvisibleObjects();
        else
            world.showInvisibleObjects();
        project->clearDirections();
    }
    return;
}

// ===== END COLOR EDITING FUNCTIONS =====

// ===== BEGIN SPRING EDITING FUNCTIONS =====

void deleteSpring(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Move to a spring and release to delete the spring.");
    } else  // button released
    {

        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        Connector *thisHandNearestSpring = handObj.getNearestConnector(NULL);
        double thisHandNearestSpringDist =
            handObj.getNearestConnectorDistance();

        SketchBio::Hand &otherHandObj = project->getHand(
            (hand == 1) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

        Connector *otherHandNearestSpring =
            otherHandObj.getNearestConnector(NULL);

        if (thisHandNearestSpringDist < SPRING_DISTANCE_THRESHOLD) {
            if (thisHandNearestSpring == otherHandNearestSpring) {
                otherHandObj.clearNearestConnector();
            }
            handObj.clearNearestConnector();
            project->getWorldManager().removeSpring(thisHandNearestSpring);
        }
        project->clearDirections();
    }
    return;
}

class SnapModeOperationState : public SketchBio::OperationState
{
   public:
    SnapModeOperationState(Connector *c, bool end1)
        : conn(c), atEnd1(end1), snapToNTerminus(true)
    {
    }
    virtual void doFrameUpdates()
    {
        conn->snapToTerminus(atEnd1, snapToNTerminus);
    }
    void setSnapToNTerminus(bool nTerminus) { snapToNTerminus = nTerminus; }

   private:
    Connector *conn;
    bool atEnd1;
    bool snapToNTerminus;
};

void snapSpringToTerminus(SketchBio::Project *project, int hand,
                          bool wasPressed)
{

    if (wasPressed) {
        bool atEnd1;
        SketchBio::Hand &handObj = project->getHand(
            hand == 0 ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        Connector *nearestSpring = handObj.getNearestConnector(&atEnd1);
        double nearestSpringDist = handObj.getNearestConnectorDistance();
        if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD &&
            project->getOperationState() == NULL) {
            project->setOperationState(
                new SnapModeOperationState(nearestSpring, atEnd1));
            project->setDirections(
                "Toggle which terminus the connector is attached to,\n"
                "then release to finalize.");
        }
    } else  // button released
    {
        SnapModeOperationState *snap = dynamic_cast< SnapModeOperationState * >(
            project->getOperationState());
        if (snap != NULL) {
            project->setOperationState(NULL);
            project->clearDirections();
        }
    }
    return;
}

void setTerminusToSnapSpring(SketchBio::Project *project, int, bool wasPressed)
{
    SnapModeOperationState *snap =
        dynamic_cast< SnapModeOperationState * >(project->getOperationState());
    if (wasPressed) {
        if (snap != NULL) {
            snap->setSnapToNTerminus(false);
        }
    } else  // button released
    {
        if (snap != NULL) {
            snap->setSnapToNTerminus(true);
        }
    }
    return;
}

void createSpring(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections("Release at location of new spring.");
    } else  // button released
    {
        q_vec_type pos1, pos2 = {0, 1, 0};
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        handObj.getPosition(pos1);

        q_vec_add(pos2, pos1, pos2);
        SpringConnection *spring = SpringConnection::makeSpring(
            NULL, NULL, pos1, pos2, true, DEFAULT_SPRING_STIFFNESS, 0, true);
        project->getWorldManager().addConnector(spring);
        addXMLUndoState(project);
        project->clearDirections();
    }
    return;
}

void createTransparentConnector(SketchBio::Project *project, int hand,
                                bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Release to create a transparent connector "
            "at the current location.");
    } else  // button released
    {
        q_vec_type pos1, pos2 = {0, 1, 0};
        project->getHand((hand == 0)
                             ? SketchBioHandId::LEFT
                             : SketchBioHandId::RIGHT).getPosition(pos1);
        q_vec_add(pos2, pos1, pos2);
        Connector *conn = new Connector(NULL, NULL, pos1, pos2, 0.3, 10);
        project->getWorldManager().addConnector(conn);
        addXMLUndoState(project);
        project->clearDirections();
    }
    return;
}

// ===== END SPRING EDITING FUNCTIONS =====

// ===== BEGIN GRAB FUNCTIONS =====
void grabObjectOrWorld(SketchBio::Project *project, int hand, bool wasPressed)
{

    if (project->isShowingAnimation())  // ignore grabbing during an animation
                                        // preview
    {
        return;
    }

    SketchBio::Hand &handObj = project->getHand(
        (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

    if (wasPressed) {

        double nearestObjDist = handObj.getNearestObjectDistance();

        if (nearestObjDist > DISTANCE_THRESHOLD) {
            handObj.grabWorld();
        } else {
            handObj.grabNearestObject();
        }
    } else  // button released
    {
        handObj.releaseGrabbed();
        addXMLUndoState(project);
    }
    return;
}
void grabSpringOrWorld(SketchBio::Project *project, int hand, bool wasPressed)
{
    SketchBio::Hand &handObj = project->getHand(
        (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);

    double nearestSpringDist = handObj.getNearestConnectorDistance();

    if (wasPressed) {
        if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD)
            handObj.grabNearestConnector();
        else
            handObj.grabWorld();

    } else  // button released
    {
        handObj.releaseGrabbed();
    }
    return;
}

void selectParentOfCurrent(SketchBio::Project *project, int hand,
                           bool wasPressed)
{
    if (!wasPressed) {
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        handObj.selectParentObjectOfCurrent();
    }
}
void selectChildOfCurrent(SketchBio::Project *project, int hand,
                          bool wasPressed)
{
    if (!wasPressed) {
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        if (handObj.getNearestObjectDistance() < DISTANCE_THRESHOLD) {
            handObj.selectSubObjectOfCurrent();
        }
    }
}

// ===== END GRAB FUNCTIONS =====

// ===== BEGIN TRANSFORM EDITING FUNCTIONS =====

void deleteObject(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        project->setDirections(
            "Move to the object you want to delete and"
            " release the button.\nWarning: this is a"
            " permanent delete!");
    } else {
        SketchBioHandId::Type h =
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT;
        SketchBio::Hand &handObj = project->getHand(h);
        double hDist = handObj.getNearestObjectDistance();
        if (hDist < DISTANCE_THRESHOLD) {  // object is selected
            SketchObject *obj = handObj.getNearestObject();
            handObj.clearNearestObject();
            project->getWorldManager().deleteObject(obj);
        }
        addXMLUndoState(project);
        project->clearDirections();
    }
}

void replicateObject(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        if (project->getOperationState() != NULL) {
            return;
        }
        SketchBio::Hand &handObj = project->getHand(
            (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        double hDist = handObj.getNearestObjectDistance();
        SketchObject *obj = handObj.getNearestObject();

        if (hDist < DISTANCE_THRESHOLD) {  // object is selected
            q_vec_type pos;
            double bb[6];
            obj->getPosition(pos);
            obj->getOrientedBoundingBoxes()->GetOutput()->GetBounds(bb);
            double difference;
            difference = bb[3] - bb[2];
            pos[Q_Y] += difference;
            SketchObject *nObj = obj->getCopy();
            nObj->setPosition(pos);
            project->getWorldManager().addObject(nObj);
            int nCopies = 1;
            project->addReplication(obj, nObj, nCopies);
            /*emit newDirectionsString("Pull the trigger to set the number
            of replicas to add,\nthen release the button.");*/
        }

    } else {
        addXMLUndoState(project);
        // emit newDirectionsString(" ");
    }
}
  
void setTransforms(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        if (project->getOperationState() == NULL) {
            project->setOperationState(
                new TransformEditOperationState(project));
            project->setDirections(
                "Select the object to use as the base of the\n"
                "coordinate system and release the button.");
        } else if (dynamic_cast< TransformEditOperationState * >(
                       project->getOperationState())) {
            project->setDirections(
                "Select the object to define the transformation\n"
                "to and release the button.");
        }
    } else  // released
    {
        // TODO add control function
        TransformEditOperationState *state =
            dynamic_cast< TransformEditOperationState * >(
                project->getOperationState());
        if (state != NULL) {
            const QVector< SketchObject * > &objectsSelected = state->getObjs();

            SketchBio::Hand &handObj = project->getHand(
                (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
            double hDist = handObj.getNearestObjectDistance();
            SketchObject *obj = handObj.getNearestObject();

            if (objectsSelected.empty()) {

                if (hDist < DISTANCE_THRESHOLD) {
                    state->addObject(obj);
                    project->setDirections(
                        "Press to select the object to define relative\n"
                        "to the selected object.");
                } else {
                    project->clearDirections();
                }
            } else {
                if (hDist < DISTANCE_THRESHOLD) {
                    state->addObject(obj);
                    vtkSmartPointer< vtkTransform > transform =
                        vtkSmartPointer< vtkTransform >::New();
                    transform->Identity();
                    transform->PreMultiply();
                    transform->Concatenate(
                        objectsSelected[1]->getLocalTransform());
                    transform->Concatenate(
                        objectsSelected[0]->getInverseLocalTransform());
                    transform->Update();
                    TransformInputDialog *dialog = new TransformInputDialog(
                        "Set Transformation from First to Second");
                    dialog->setTranslation(transform->GetPosition());
                    dialog->setRotation(transform->GetOrientation());
                    QObject::connect(
                        dialog,
                        SIGNAL(transformAquired(double, double, double, double,
                                                double, double)),
                        state,
                        SLOT(setObjectTransform(double, double, double, double,
                                                double, double)));
                    QObject::connect(dialog, SIGNAL(rejected()), state,
                                     SLOT(cancelOperation()));
                    dialog->exec();
                    delete dialog;
                }
                project->clearDirections();
            }
        }
    }
}

// ===== END TRANSFORM EDITING FUNCTIONS =====

// ===== BEGIN UTILITY FUNCTIONS =====

void resetViewPoint(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (!wasPressed)  // button released
    {
        project->getTransformManager().setRoomEyeOrientation(0, 0);
    }
}

// ===== END UTILITY FUNCTIONS =====
}
