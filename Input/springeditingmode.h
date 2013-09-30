#ifndef SPRINGEDITINGMODE_H
#define SPRINGEDITINGMODE_H

#include "hydrainputmode.h"

class Connector;

class SpringEditingMode : public HydraInputMode
{
    Q_OBJECT
public:
    SpringEditingMode(SketchProject *proj, bool const *buttonState, double const *analogState);
    virtual ~SpringEditingMode();
    // Called anytime a button is pressed
    virtual void buttonPressed(int vrpn_ButtonNum);
    // Called anytime a button is released
    virtual void buttonReleased(int vrpn_ButtonNum);
    // Called when the analogs status have been updated
    virtual void analogsUpdated();
    // Called in each frame to handle the input for that frame
    virtual void doUpdatesForFrame();
    // TEMPLATE METHOD
    // this will be called from within setProject after the new project is set
    // it should refresh the status and delete all references to the old project
    virtual void clearStatus();
private:
    int grabbedWorld;
    Connector* lSpring, * rSpring;
    double lSpringDist, rSpringDist;
    bool lAtEnd1, rAtEnd1;
    bool leftGrabbedSpring, rightGrabbedSpring;
};

#endif // SPRINGEDITINGMODE_H
