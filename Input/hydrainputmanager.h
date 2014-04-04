#ifndef HYDRAINPUTMANAGER_H
#define HYDRAINPUTMANAGER_H

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <quat.h>

#include <QObject>
#include <QVector>
#include <QSharedPointer>

#include "sketchioconstants.h"
namespace SketchBio {
class Project;
}

class HydraInputMode;

class HydraInputManager : public QObject
{
    Q_OBJECT
public:
    HydraInputManager(SketchBio::Project *proj);
    virtual ~HydraInputManager();
    void setProject(SketchBio::Project *proj);
    void handleCurrentInput();
    void updateTrackerObjectConnections();
    QString getModeName();

    // called by the callback functions
    void setHandPos(q_xyz_quat_type *newPos,SketchBioHandId::Type side);
    void setButtonState(int buttonNum, bool buttonPressed);
    void setAnalogStates(const double state[]);

    // mostly for testing
    HydraInputMode *getActiveMode();

public slots:
    // This slot adds an undo state to the project.  An undo
    // state is a saved state that can be gotten back to by
    // pressing undo
    void addUndoState();

signals:
    void toggleWorldSpringsEnabled();
    void toggleWorldCollisionsEnabled();
    void newDirectionsString(QString str);
    void changedModes(QString newModeName);
    void viewTimeChanged(double time);

private slots:
    // connected to all modes so that the signal they emit will be 'caught'
    // here and re-emitted to listeners on this object
    void setNewDirectionsString(QString str);
    void newViewTime(double time);
private:

    // VRPN callback functions
    static void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t);
    static void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b);
    static void VRPN_CALLBACK handle_analogs(void *userdata, const vrpn_ANALOGCB a);

    SketchBio::Project *project;
    vrpn_Tracker_Remote tracker;
    vrpn_Button_Remote buttons;
    vrpn_Analog_Remote analogRemote;
    bool buttonsDown[NUM_HYDRA_BUTTONS]; // number of buttons, each with 2 states
                                    // 1 = pressed, 0 = not pressed
    double analogStatus[NUM_HYDRA_ANALOGS]; // number of analogs and their current values
    QVector < QSharedPointer < HydraInputMode > > modeList;
    int modeIndex;
    QSharedPointer < HydraInputMode > activeMode;
};


#endif // HYDRAINPUTMANAGER_H
