#include "controlFunctions.h"
#include "savedxmlundostate.h"

#include <sstream>

#include <QApplication>
#include <QClipboard>

#include <quat.h>
#include <cmath>

#include <vtkXMLUtilities.h>
#include <vtkXMLDataElement.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkTransform.h>
#include <vtkSmartPointer.h>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <sketchproject.h>
#include <connector.h>
#include <structurereplicator.h>
#include <transformequals.h>
#include <keyframe.h>
#include <objectgroup.h>
#include <projecttoxml.h>
#include <springconnection.h>
#include <transformmanager.h>
#include <worldmanager.h>
#include <hand.h>
#include <sketchtests.h>

#include <OperationState.h>

#include "TransformInputDialog.h"
#include "transformeditoperationstate.h"

static const double DEFAULT_SPRING_STIFFNESS = 1.0;

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
        addUndoState(project);
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
            addUndoState(project);
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
        addUndoState(project);
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
            addUndoState(project);
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
            addUndoState(project);
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
            addUndoState(project);
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
            addUndoState(project);
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
            addUndoState(project);
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

static const char SNAP_SPRING_OPERATION_FUNC_NAME[30] = "snap_spring";
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
            project->getOperationState(SNAP_SPRING_OPERATION_FUNC_NAME) == NULL) {
            project->setOperationState(SNAP_SPRING_OPERATION_FUNC_NAME,
                new SnapModeOperationState(nearestSpring, atEnd1));
            project->setDirections(
                "Toggle which terminus the connector is attached to,\n"
                "then release to finalize.");
        }
    } else  // button released
    {
        SnapModeOperationState *snap = dynamic_cast< SnapModeOperationState * >(
            project->getOperationState(SNAP_SPRING_OPERATION_FUNC_NAME));
        if (snap != NULL) {
            project->clearOperationState(SNAP_SPRING_OPERATION_FUNC_NAME);
            project->clearDirections();
        }
    }
    return;
}

void setTerminusToSnapSpring(SketchBio::Project *project, int, bool wasPressed)
{
    SnapModeOperationState *snap =
        dynamic_cast< SnapModeOperationState * >(project->getOperationState(
                                                     SNAP_SPRING_OPERATION_FUNC_NAME));
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
        addUndoState(project);
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
        addUndoState(project);
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
        addUndoState(project);
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
        addUndoState(project);
        project->clearDirections();
    }
}

void replicateObject(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
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
        addUndoState(project);
        // emit newDirectionsString(" ");
    }
}
  
void setTransforms(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (wasPressed) {
        if (project->getOperationState(
                    TransformEditOperationState::SET_TRANSFORMS_OPERATION_FUNCTION)
                == NULL) {
            project->setOperationState(
                        TransformEditOperationState::SET_TRANSFORMS_OPERATION_FUNCTION,
                new TransformEditOperationState(project));
            project->setDirections(
                "Select the object to use as the base of the\n"
                "coordinate system and release the button.");
        } else if (dynamic_cast< TransformEditOperationState * >(
                       project->getOperationState(TransformEditOperationState::SET_TRANSFORMS_OPERATION_FUNCTION))) {
            project->setDirections(
                "Select the object to define the transformation\n"
                "to and release the button.");
        }
    } else  // released
    {
        // TODO add control function
        TransformEditOperationState *state =
            dynamic_cast< TransformEditOperationState * >(
                project->getOperationState(TransformEditOperationState::SET_TRANSFORMS_OPERATION_FUNCTION));
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
                            vtkSmartPointer< vtkTransform >::Take(
                                SketchObject::computeRelativeTransform(
                                    objectsSelected[0],objectsSelected[1]));
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

class LockTransformsOperationState : public SketchBio::OperationState {
public:
    LockTransformsOperationState() : OperationState(), lock(NULL) {}
    virtual ~LockTransformsOperationState() {}
    void addObject(SketchObject *o) {
        list.append(o);
    }
    QList<SketchObject*> &getList() {
        return list;
    }
    int getHand() {
        return hand;
    }
    QSharedPointer<TransformEquals> &getLock() {
        return lock;
    }
    void setLock(const QSharedPointer<TransformEquals> &l) {
        lock = l;
    }

private:
    QList<SketchObject*> list;
    QSharedPointer< TransformEquals > lock;
    int hand;
};

static const char LOCK_TRANSFORMS_OPERATION_FUNCTION_NAME[] = "Lock Transforms";
void lockTransforms(SketchBio::Project *proj, int hand, bool wasPressed)
{
    LockTransformsOperationState *state = dynamic_cast<LockTransformsOperationState*>(
                proj->getOperationState(LOCK_TRANSFORMS_OPERATION_FUNCTION_NAME));
    if (state != NULL && state->getHand() != hand) {
        proj->clearOperationState(LOCK_TRANSFORMS_OPERATION_FUNCTION_NAME);
        state = NULL;
    }
    if (wasPressed) {
        if (state == NULL) {
            proj->setDirections("Move to the first object and release the button to lock transforms");
        } else {
            QList<SketchObject*> &list = state->getList();
            if (list.size() % 2 == 1) {
                proj->setDirections("Move to the second object in the pair and release to add the pair");
            } else {
                proj->setDirections("Move to the first object in the next pair and release\n"
                                    "or release in empty space to cancel");
            }
        }
    } else {
        SketchBio::Hand &handObj = proj->getHand(
                    hand== 0 ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
        if (handObj.getNearestObjectDistance() < DISTANCE_THRESHOLD) {
            if (state == NULL) {
                state = new LockTransformsOperationState();
                proj->setOperationState(LOCK_TRANSFORMS_OPERATION_FUNCTION_NAME,state);
            }
            state->addObject(handObj.getNearestObject());
            QList<SketchObject*> &list = state->getList();
            if (list.size() % 2 == 0) {
                int s = list.size();
                if (state->getLock().isNull()) {
                    QWeakPointer<TransformEquals> l = proj->addTransformEquals(list[0],list[1]);
                    state->setLock(l.toStrongRef());
                } else {
                    state->getLock()->addPair(list[s-2],list[s-1]);
                }
                proj->setDirections("Press to select the first object in the next pair");
            } else {
                proj->setDirections("Press to select the second object in this pair");
            }
        } else {
            proj->clearOperationState(LOCK_TRANSFORMS_OPERATION_FUNCTION_NAME);
            proj->clearDirections();
        }
    }
}

// ===== END TRANSFORM EDITING FUNCTIONS =====

// ===== BEGIN UTILITY FUNCTIONS =====
  
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
          addUndoState(project);
        }
      } else {
        std::cout << "Failed to read object." << std::endl;
      }
      project->clearDirections();
    }
    return;
  }
  
class RotateCameraOperationState : public SketchBio::OperationState
  {
  public:
    RotateCameraOperationState(SketchBio::Project *project)
    : proj(project), dX(0), dY(0),x(0), y(0)
    {
    }
    virtual void doFrameUpdates()
    {
      x+=dX;
      y+=dY;
      proj->getTransformManager().setRoomEyeOrientation(x,y);
    }
    void setdX(double xAdd)
    {
      if (std::abs(xAdd) > 0.3)
      {
        dX = xAdd;
      } else {
        dX = 0;
      }
    }
    void setdY(double yAdd)
    { 
      if (std::abs(yAdd) > 0.3)
      {
        dY = yAdd;
      } else {
        dY = 0;
      }
    }
    void reset() {x=0; y=0; dX=0; dY=0;}
    
  private:
    SketchBio::Project *proj;
    double dX;
    double dY;
    double x;
    double y;
  };
  
static const char ROTATE_CAMERA_OPERATION_FUNC_NAME[30] = "rotate_camera";  
void resetViewPoint(SketchBio::Project *project, int hand, bool wasPressed)
{
    if (!wasPressed)  // button released
    {
      if (project->getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME) == NULL)
      {
        project->setOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME, new RotateCameraOperationState(project));
      } else {
        RotateCameraOperationState *rotCam = dynamic_cast< RotateCameraOperationState * >(
          project->getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME));
        rotCam->reset();
      }
    }
}
  
void redo(SketchBio::Project *project, int hand, bool wasPressed)
{
  if (wasPressed)  // button pressed
  {
    project->applyRedo();
  }
}

void undo(SketchBio::Project *project, int hand, bool wasPressed)
  {
    if (wasPressed)  // button pressed
    {
      project->applyUndo();
    } 
  }
  
void toggleCollisionChecks(SketchBio::Project *project, int hand, bool wasPressed)
  {
    if (wasPressed)  // button pressed
    {
      project->getWorldManager().setCollisionCheckOn(!project->getWorldManager().isCollisionTestingOn());
    }
  }
  
void toggleSpringsEnabled(SketchBio::Project *project, int hand, bool wasPressed)
  {
    if (wasPressed)  // button pressed
    {
      project->getWorldManager().setPhysicsSpringsOn(!project->getWorldManager().areSpringsEnabled());
    } 
  }

class ZoomOperationState : public SketchBio::OperationState {
public:
    ZoomOperationState(TransformManager &t, int h) : OperationState(), mgr(t), hand(h) {}
    ~ZoomOperationState() {}
    virtual void doFrameUpdates() {
        double dist = mgr.getOldWorldDistanceBetweenHands();
        double delta = mgr.getWorldDistanceBetweenHands() /dist;
        mgr.scaleWithTrackerFixed(delta,((hand==0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT));
    }
    int getHand() const {
        return hand;
    }

private:
    TransformManager &mgr;
    const int hand;
};

static const char ZOOM_OPERATION_FUNCTION_NAME[] = "ZOO0M";
  
void zoom(SketchBio::Project *project, int hand, bool wasPressed)
  {
    ZoomOperationState *zState = dynamic_cast<ZoomOperationState*>(
                project->getOperationState(ZOOM_OPERATION_FUNCTION_NAME));
    if (wasPressed)  // button pressed
    {
      if (project->isShowingAnimation() || zState != NULL)
      {
        return;
      }
      project->setOperationState(
                  ZOOM_OPERATION_FUNCTION_NAME,
                  new ZoomOperationState(project->getTransformManager(),hand));
    } else {
        if (zState != NULL && zState->getHand() == hand) {
            project->clearOperationState(ZOOM_OPERATION_FUNCTION_NAME);
        }
    } 
    
}

// ===== END UTILITY FUNCTIONS =====
  
// ===== BEGIN NON CONTROL FUNCTIONS (but still useful) =====

void addUndoState(SketchBio::Project *project)
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

// ===== END NON CONTROL FUNCTIONS =====
  
// ===== BEGIN ANALOG FUNCTIONS =====

// value passed is in the interval [0,1]  
  
void rotateCameraPitch(SketchBio::Project *project, int hand, double value)
{
  if (project->isShowingAnimation())
  {
    return;
  }
  if (project->getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME) == NULL)
  {
    project->setOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME, new RotateCameraOperationState(project));
  }
  
  value = (value-0.5)*2;
  
  RotateCameraOperationState *rotCam = dynamic_cast< RotateCameraOperationState * >(
    project->getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME));
  rotCam->setdX(value);
  
  return;
}
  
void rotateCameraYaw(SketchBio::Project *project, int hand, double value)
{
  if (project->isShowingAnimation())
  {
    return;
  }
  if (project->getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME) == NULL)
  {
    project->setOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME, new RotateCameraOperationState(project));
  }
  
  value = (value-0.5)*2;
  
  RotateCameraOperationState *rotCam = dynamic_cast< RotateCameraOperationState * >(
    project->getOperationState(ROTATE_CAMERA_OPERATION_FUNC_NAME));
  rotCam->setdY(value);
  return;
}
  
void setCrystalByExampleCopies(SketchBio::Project *project, int hand, double value)
{

  SketchBio::Hand &handObj = project->getHand(
                                              (hand == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
  double hDist = handObj.getNearestObjectDistance();
  SketchObject *grp = handObj.getNearestObject();
  
  if (hDist < DISTANCE_THRESHOLD)
  {
    QList <StructureReplicator*> SRs = project->getCrystalByExamples();
    StructureReplicator* activeSR;
  
    bool found = false;
    for (int i = 0; i < SRs.count(); i++)
    {
      activeSR = SRs.at(i);
      if (activeSR->getReplicaGroup() ==grp)
      {
        found = true;
        break;
      }
    }
    
    if (found)
    {
      int nCopies = min(floor(pow(2.0, value / .125)), 64);
      activeSR->setNumShown(nCopies);
    }
  }

  return;
}
 
  class TimelineOperationState : public SketchBio::OperationState
  {
  public:
    TimelineOperationState(SketchBio::Project *project)
    : proj(project), dTime(0), currTime(0)
    {
    }
    virtual void doFrameUpdates()
    {
        if (Q_ABS(dTime) < Q_EPSILON) {
            return;
        }
      currTime = max(currTime + dTime,0);
      proj->setViewTime(currTime);
    }
    void setDTime(double timeAdd)
    {
      dTime= timeAdd;
    }
  private:
    SketchBio::Project *proj;
    double dTime;
    double currTime;
  };
  
  static const char TIMELINE_OPERATION_FUNC_NAME[30] = "timeline";
  
void moveAlongTimeline(SketchBio::Project *project, int hand, double value)
{
  if (project->isShowingAnimation())
  {
    return;
  }
  
  double dTime;
  
  value = (value-0.5)*2;
  int sign = (value >= 0) ? 1 : -1;
  if (Q_ABS(value) > .8)
  {
    dTime = 1 * sign;
  }
  else if (Q_ABS(value) > .4)
  {
    dTime = 0.1 * sign;
  }
  else //if (Q_ABS(value) > .2)
  {
    dTime = 0;
  }


  if (project->getOperationState(TIMELINE_OPERATION_FUNC_NAME) == NULL)
  {
    project->setOperationState(TIMELINE_OPERATION_FUNC_NAME, new TimelineOperationState(project));
  }
    
  TimelineOperationState *timeline = dynamic_cast< TimelineOperationState * >(
    project->getOperationState(TIMELINE_OPERATION_FUNC_NAME));
  timeline->setDTime(dTime);
}
  
// ===== END ANALOG FUNCTIONS =====
  
// ===== END NON CONTROL FUNCTIONS (but still useful) =====
  
}
