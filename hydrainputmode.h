#ifndef HYDRAINPUTMODE_H
#define HYDRAINPUTMODE_H

#include <QObject>

class SketchProject;

/*
 * This class is the base for the strategy pattern for input modes.  Classes implementing
 * the virtual methods defined here will be used as 'modes' for interpreting the controller
 * input.
 *
 * Note: the input manager still handles all operations that will be similar across all modes
 * such as most of the left (non-dominant) hand controls (toggling springs/collisions,
 * scaling the world, changing modes). For now, the option to use the left joystick to rotate
 * the view is left up to the implementing subclass, but this option may be removed.
 */
class HydraInputMode : public QObject
{
    Q_OBJECT
public:
    // b and a will be stored in buttonStatus and analogStatus.  They
    // will be externally updated and kept current, so there is no need
    // to reimplement this functionality in subclasses of HydraInputMode
    HydraInputMode(SketchProject *proj, bool const * const b, double const * const a);
    virtual ~HydraInputMode();
    // sets this mode to operate on a new project
    void setProject(SketchProject *proj);
    // Called anytime a button is pressed
    virtual void buttonPressed(int vrpn_ButtonNum) = 0;
    // Called anytime a button is released
    virtual void buttonReleased(int vrpn_ButtonNum) = 0;
    // Called when the analogs status have been updated
    virtual void analogsUpdated() = 0;
    // Called in each frame to handle the input for that frame
    virtual void doUpdatesForFrame() = 0;
    // TEMPLATE METHOD
    // this will be called from within setProject after the new project is set
    // it should refresh the status and delete all references to the old project
    // also should be called on mode change to prevent contaminated status when
    // switching to a mode that has some undefined status
    virtual void clearStatus() = 0;

    // USEFUL FUNCTIONS FOR ALL MODES
    // uses the left joystick position to apply a rotation to the camera independent
    // of the world to tracker transformation
    void useLeftJoystickToRotateViewPoint();
    // uses the right joystick left/right to move forward/backward in animation time
    // this internally emits viewTimeChanged so there is no need to re-emit it after
    // the call to this
    void useRightJoystickToChangeViewTime();
    // scales based on the change in distance between trackers with the left assumed fixed
    void scaleWithLeftFixed();
    // scales based on the change in distance between trackers with the right assumed fixed
    void scaleWithRightFixed();
    // does a the 'grab world with left' changes for the current frame
    void grabWorldWithLeft();
    // does a the 'grab world with right' changes for the current frame
    void grabWorldWithRight();
signals:
    void newDirectionsString(QString str);
    // should be emitted whenever the setViewTime() method is called on the project
    void viewTimeChanged(double time);
    // Maybe others?
protected:

    // fields
    bool const * const isButtonDown;
    double const * const analogStatus;
    SketchProject *project;
};

#endif // HYDRAINPUTMODE_H
