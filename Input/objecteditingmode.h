#ifndef OBJECTEDITINGMODE_H
#define OBJECTEDITINGMODE_H

#include "hydrainputmode.h"
#include <QVector>

class SketchObject;

class ObjectEditingMode : public HydraInputMode
{
    Q_OBJECT
public:
    ObjectEditingMode(SketchProject *proj, const bool *b, const double *a);
    virtual ~ObjectEditingMode();
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
    // helper method
    void updateTrackerObjectConnections();

    int grabbedWorld; // state of world grabbing
    double lDist, rDist; // the distance to the closest object to the (left/right) hand
    SketchObject *lObj, *rObj; // the objects in the world that are closest to the left
                                // and right hands respectively
    int operationState;
    QVector<SketchObject *> objectsSelected;
    QVector<double> positionsSelected; // have to put in coords... can't have a vector of arrays
};

#endif // OBJECTEDITINGMODE_H
