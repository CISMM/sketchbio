#include "hydrainputmanager.h"

#include <QDebug>
#include <QSettings>

#include <sketchproject.h>
#include <worldmanager.h>
#include <transformmanager.h>
#include <sketchtests.h>

#include "transformeditingmode.h"
#include "springeditingmode.h"
#include "animationmode.h"
#include "coloreditingmode.h"
#include "groupeditingmode.h"

inline int scale_button_idx() {
    return BUTTON_LEFT(FOUR_BUTTON_IDX);
}
inline int spring_disable_button_idx() {
    return BUTTON_LEFT(THREE_BUTTON_IDX);
}
inline int collision_disable_button_idx() {
    return BUTTON_LEFT(TWO_BUTTON_IDX);
}
inline int change_modes_button_idx() {
    return BUTTON_LEFT(ONE_BUTTON_IDX);
}
inline int undo_button_idx() {
    return BUTTON_RIGHT(OBLONG_BUTTON_IDX);
}
inline int redo_button_idx() {
    return BUTTON_LEFT(OBLONG_BUTTON_IDX);
}

#define NO_OPERATION                    0
#define DUPLICATE_OBJECT_PENDING        1
#define REPLICATE_OBJECT_PENDING        2
#define ADD_SPRING_PENDING              3
#define ADD_TRANSFORM_EQUALS_PENDING    4


HydraInputManager::HydraInputManager(SketchBio::Project *proj) :
    project(proj),
    tracker(VRPN_ONE_EURO_FILTER_DEVICE_STRING),
    buttons(VRPN_RAZER_HYDRA_DEVICE_STRING),
    analogRemote(VRPN_RAZER_HYDRA_DEVICE_STRING),
    modeList(),
    modeIndex(0),
    activeMode(NULL)
{
    // clear button and analog statuses
    for (int i = 0; i < NUM_HYDRA_BUTTONS; i++) {
        buttonsDown[i] = false;
    }
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = 0.0;
    }

    // set up modes
    modeList.append(QSharedPointer< HydraInputMode >(
                        new TransformEditingMode(project,buttonsDown,analogStatus)));
    modeList.append(QSharedPointer< HydraInputMode >(
                        new SpringEditingMode(project,buttonsDown,analogStatus)));
    modeList.append(QSharedPointer< HydraInputMode >(
                        new AnimationMode(project,buttonsDown,analogStatus)));
    modeList.append(QSharedPointer< HydraInputMode >(
                        new ColorEditingMode(project,buttonsDown,analogStatus)));
    modeList.append(QSharedPointer< HydraInputMode >(
                        new GroupEditingMode(project,buttonsDown,analogStatus)));
    activeMode = modeList[modeIndex];

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
    buttons.register_change_handler((void *) this, handle_button);
    analogRemote.register_change_handler((void *) this, handle_analogs);
}

HydraInputManager::~HydraInputManager()
{
}

QString HydraInputManager::getModeName()
{
    switch (modeIndex)
    {
    case 0:
        return QString("Edit Objects");
    case 1:
        return QString("Edit Springs");
    case 2:
        return QString("Animation");
    case 3:
        return QString("Edit Colors");
    case 4:
        return QString("Edit Groups");
    default:
        return QString("Unknown");
    }
}

void HydraInputManager::setProject(SketchBio::Project *proj) {
    project = proj;
    for (int i = 0; i < modeList.size(); i++)
        modeList[i]->setProject(proj);
}

void HydraInputManager::handleCurrentInput() {
    // get new data from hydra
    tracker.mainloop();
    buttons.mainloop();
    analogRemote.mainloop();

    if (buttonsDown[scale_button_idx()]) {
        activeMode->scaleWithLeftFixed();
    }
    activeMode->useRightJoystickToChangeViewTime();

    // handle input
    activeMode->doUpdatesForFrame();

    // set tracker locations
    project->updateTrackerPositions();

    // update the main camera
    // this has to be last since updateTrackerPositions performs
    // grab world
    project->getTransformManager().updateCameraForFrame();
}


void HydraInputManager::setHandPos(q_xyz_quat_type *newPos, SketchBioHandId::Type side) {
  project->getTransformManager().setHandTransform(newPos,side);
}


void HydraInputManager::setButtonState(int buttonNum, bool buttonPressed) {
    if (buttonPressed) {
        // events on press
        if (buttonNum == spring_disable_button_idx()) {
            project->getWorldManager().setPhysicsSpringsOn(
                        project->getWorldManager().areSpringsEnabled());
        } else if (buttonNum == collision_disable_button_idx()) {
            project->getWorldManager().setCollisionCheckOn(
                        project->getWorldManager().isCollisionTestingOn());
        } else if (buttonNum == change_modes_button_idx()) {
            activeMode->clearStatus();
            modeIndex = (modeIndex + 1 ) % modeList.size();
            activeMode = modeList[modeIndex];
            activeMode->clearStatus();
            emit changedModes();
            project->setDirections(" ");
        } else if (buttonNum == undo_button_idx()) {
            project->applyUndo();
            activeMode->clearStatus();
        } else if (buttonNum == redo_button_idx()) {
            project->applyRedo();
            activeMode->clearStatus();
        } else {
            activeMode->buttonPressed(buttonNum);
        } // TODO - change modes button
    } else {
        activeMode->buttonReleased(buttonNum);
    }
    buttonsDown[buttonNum] = buttonPressed;
}

void HydraInputManager::setAnalogStates(const double state[]) {
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = state[i];
    }
    activeMode->analogsUpdated();
}

HydraInputMode *HydraInputManager::getActiveMode()
{
    return activeMode.data();
}

void HydraInputManager::addUndoState()
{
    activeMode->addXMLUndoState();
}

//####################################################################################
// VRPN callback functions
//####################################################################################


void VRPN_CALLBACK HydraInputManager::handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    q_xyz_quat_type data;
    // TransformManager handles difference between coordinates of trackers and world
    // no need to do math here
    q_vec_copy(data.xyz,t.pos);
    q_copy(data.quat,t.quat);
    // set the correct hand's position
    mgr->setHandPos(&data,(t.sensor == 0) ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
}

void VRPN_CALLBACK HydraInputManager::handle_button(void *userdata, const vrpn_BUTTONCB b) {
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    mgr->setButtonState(b.button,b.state);
}

void VRPN_CALLBACK HydraInputManager::handle_analogs(void *userdata, const vrpn_ANALOGCB a) {
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    if (a.num_channel != NUM_HYDRA_ANALOGS) {
        qDebug() << "Wrong number of analogs!";
    }
    mgr->setAnalogStates(a.channel);
}

