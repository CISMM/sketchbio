#ifndef ANIMATIONMODE_H
#define ANIMATIONMODE_H

#include "hydrainputmode.h"

class SketchObject;

class AnimationMode : public HydraInputMode
{
    Q_OBJECT
public:
    AnimationMode(SketchProject *proj, const bool *const b, const double *const a);
    virtual ~AnimationMode();

    virtual void buttonPressed(int vrpn_ButtonNum);
    virtual void buttonReleased(int vrpn_ButtonNum);
    virtual void analogsUpdated();
    virtual void doUpdatesForFrame();
    virtual void clearStatus();

signals:
    
public slots:

private:
    int worldGrabbed;
    
    double lDist, rDist; // the distance to the closest object to the (left/right) hand
    SketchObject *lObj, *rObj; // the objects in the world that are closest to the left
                                // and right hands respectively
};

#endif // ANIMATIONMODE_H
