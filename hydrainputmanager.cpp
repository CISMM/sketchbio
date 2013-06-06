#include "hydrainputmanager.h"
#include "sketchproject.h"
#include "objecteditingmode.h"
#include "springeditingmode.h"
#include "sketchtests.h"
#include <QDebug>
#include <QSettings>

#define NO_OPERATION                    0
#define DUPLICATE_OBJECT_PENDING        1
#define REPLICATE_OBJECT_PENDING        2
#define ADD_SPRING_PENDING              3
#define ADD_TRANSFORM_EQUALS_PENDING    4


HydraInputManager::HydraInputManager(SketchProject *proj) :
    project(proj),
    tracker(VRPN_RAZER_HYDRA_DEVICE_STRING),
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
                        new ObjectEditingMode(project,buttonsDown,analogStatus)));
    modeList.append(QSharedPointer< HydraInputMode >(
                        new SpringEditingMode(project,buttonsDown,analogStatus)));
    activeMode = modeList[modeIndex];

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
    buttons.register_change_handler((void *) this, handle_button);
    analogRemote.register_change_handler((void *) this, handle_analogs);

    for (int i = 0; i < modeList.size(); i++)
    {
    connect(this->modeList[i].data(), SIGNAL(newDirectionsString(QString)),
            this, SLOT(setNewDirectionsString(QString)));
    }

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
    default:
        return QString("");
    }
}

void HydraInputManager::setProject(SketchProject *proj) {
    project = proj;
    for (int i = 0; i < modeList.size(); i++)
        modeList[i]->setProject(proj);
}

void HydraInputManager::handleCurrentInput() {
    // get new data from hydra
    tracker.mainloop();
    buttons.mainloop();
    analogRemote.mainloop();

    if (buttonsDown[HydraButtonMapping::scale_button_idx()]) {
        activeMode->scaleWithLeftFixed();
    }
    activeMode->useRightJoystickToChangeViewTime();

    // handle input
    activeMode->doUpdatesForFrame();

    // update the main camera
    project->getTransformManager()->updateCameraForFrame();

    // set tracker locations
    project->updateTrackerPositions();
}


void HydraInputManager::setLeftPos(q_xyz_quat_type *newPos) {
    project->setLeftHandPos(newPos);
}

void HydraInputManager::setRightPos(q_xyz_quat_type *newPos) {
    project->setRightHandPos(newPos);
}

void HydraInputManager::setButtonState(int buttonNum, bool buttonPressed) {
    if (buttonPressed) {
        // events on press
        if (buttonNum == HydraButtonMapping::spring_disable_button_idx()) {
            emit toggleWorldSpringsEnabled();
        } else if (buttonNum == HydraButtonMapping::collision_disable_button_idx()) {
            emit toggleWorldCollisionsEnabled();
        } else if (buttonNum == HydraButtonMapping::change_modes_button_idx()) {
            modeIndex = (modeIndex + 1 ) % modeList.size();
            activeMode = modeList[modeIndex];
            activeMode->clearStatus();
            emit changedModes(getModeName());
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

void HydraInputManager::setNewDirectionsString(QString str) {
    emit newDirectionsString(str);
}

//####################################################################################
// VRPN callback functions
//####################################################################################


void VRPN_CALLBACK HydraInputManager::handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
    HydraInputManager *mgr = (HydraInputManager *) userdata;
    q_xyz_quat_type data;
    // changes coordinates to OpenGL coords, switching y & z and negating y
    data.xyz[Q_X] = t.pos[Q_X];
    data.xyz[Q_Y] = -t.pos[Q_Z];
    data.xyz[Q_Z] = t.pos[Q_Y];
    data.quat[Q_X] = t.quat[Q_X];
    data.quat[Q_Y] = -t.quat[Q_Z];
    data.quat[Q_Z] = t.quat[Q_Y];
    data.quat[Q_W] = t.quat[Q_W];
    // set the correct hand's position
    if (t.sensor == 0) {
        mgr->setLeftPos(&data);
    } else {
        mgr->setRightPos(&data);
    }
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

