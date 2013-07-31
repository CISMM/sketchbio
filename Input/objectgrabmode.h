#ifndef OBJECTGRABMODE_H
#define OBJECTGRABMODE_H

#include "hydrainputmode.h"

class SketchObject;

/*
 * This class is a superclass that provides the functionality of grabbing
 * objects or the world with the bumper buttons.  To add a mode based on this,
 * override the methods you need but call the superclass methods in the overriden
 * methods
 */
class ObjectGrabMode : public HydraInputMode
{
    Q_OBJECT
public:
    ObjectGrabMode(SketchProject *proj, bool const * const b,
                   double const * const a);
    virtual ~ObjectGrabMode();
    
    virtual void buttonPressed(int vrpn_ButtonNum);
    virtual void buttonReleased(int vrpn_ButtonNum);
    virtual void doUpdatesForFrame();
    virtual void clearStatus();
    virtual void analogsUpdated();

protected:
    int worldGrabbed; // state of world grabbing
    double lDist, rDist; // the distance to the closest object to the (left/right) hand
    SketchObject *lObj, *rObj; // the objects in the world that are closest to the left
                                // and right hands respectively
    SketchObject *lBase, *rBase;
    int leftLevel, rightLevel;
    bool bumpLevels;

};

#endif // OBJECTGRABMODE_H
