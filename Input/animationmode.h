#ifndef ANIMATIONMODE_H
#define ANIMATIONMODE_H

#include "objectgrabmode.h"

class SketchObject;

class AnimationMode : public ObjectGrabMode
{
    Q_OBJECT
public:
    AnimationMode(SketchBio::Project *proj, const bool *const b, const double *const a);
    virtual ~AnimationMode();

    virtual void buttonPressed(int vrpn_ButtonNum);
    virtual void buttonReleased(int vrpn_ButtonNum);
    virtual void doUpdatesForFrame();
    virtual void analogsUpdated();
    virtual void clearStatus();
};

#endif // ANIMATIONMODE_H
