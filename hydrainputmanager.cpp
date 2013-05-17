#include "hydrainputmanager.h"
#include "objecteditingmode.h"
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
    mode(new ObjectEditingMode(project,buttonsDown,analogStatus))
{
    for (int i = 0; i < NUM_HYDRA_BUTTONS; i++) {
        buttonsDown[i] = false;
    }
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = 0.0;
    }

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
    buttons.register_change_handler((void *) this, handle_button);
    analogRemote.register_change_handler((void *) this, handle_analogs);

    connect(this->mode, SIGNAL(newDirectionsString(QString)),
            this, SLOT(setNewDirectionsString(QString)));

}

HydraInputManager::~HydraInputManager()
{
    delete mode;
}

void HydraInputManager::setProject(SketchProject *proj) {
    project = proj;
    mode->setProject(proj);
}

void HydraInputManager::handleCurrentInput() {
    // get new data from hydra
    tracker.mainloop();
    buttons.mainloop();
    analogRemote.mainloop();

    if (buttonsDown[HydraButtonMapping::scale_button_idx()]) {
        mode->scaleWithLeftFixed();
    }

    // handle input
    mode->doUpdatesForFrame();

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
        } else {
            mode->buttonPressed(buttonNum);
        } // TODO - change modes button
    } else {
        mode->buttonReleased(buttonNum);
    }
    buttonsDown[buttonNum] = buttonPressed;
}

void HydraInputManager::setAnalogStates(const double state[]) {
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analogStatus[i] = state[i];
    }
    mode->analogsUpdated();
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

