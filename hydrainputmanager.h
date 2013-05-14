#ifndef HYDRAINPUTMANAGER_H
#define HYDRAINPUTMANAGER_H

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <quat.h>

#include "sketchioconstants.h"
#include "sketchproject.h"

class HydraInputManager : public QObject
{
    Q_OBJECT
public:
    HydraInputManager(SketchProject *proj);
    virtual ~HydraInputManager();
    void setProject(SketchProject *proj);
    void handleCurrentInput();
    void updateTrackerObjectConnections();

    // called by the callback functions
    void setLeftPos(q_xyz_quat_type *newPos);
    void setRightPos(q_xyz_quat_type *newPos);
    void setButtonState(int buttonNum, bool buttonPressed);
    void setAnalogStates(const double state[]);

signals:
    void toggleWorldSpringsEnabled();
private:

    // VRPN callback functions
    static void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t);
    static void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b);
    static void VRPN_CALLBACK handle_analogs(void *userdata, const vrpn_ANALOGCB a);

    SketchProject *project;
    vrpn_Tracker_Remote tracker;
    vrpn_Button_Remote buttons;
    vrpn_Analog_Remote analogRemote;
    bool buttonsDown[NUM_HYDRA_BUTTONS]; // number of buttons, each with 2 states
                                    // 1 = pressed, 0 = not pressed
    double analogStatus[NUM_HYDRA_ANALOGS]; // number of analogs and their current values
    int grabbedWorld; // state of world grabbing
    double lDist, rDist; // the distance to the closest object to the (left/right) hand
    SketchObject *lObj, *rObj; // the objects in the world that are closest to the left
                                // and right hands respectively
    bool rightHandDominant;
    QVector<SketchObject *> objectsSelected;
    QVector<double> positionsSelected; // have to put in coords... can't have a vector of arrays
};

#endif // HYDRAINPUTMANAGER_H
