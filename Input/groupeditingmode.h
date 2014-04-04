#ifndef GROUPEDITINGMODE_H
#define GROUPEDITINGMODE_H

#include "objectgrabmode.h"

class GroupEditingMode : public ObjectGrabMode
{
    Q_OBJECT
public:
    GroupEditingMode(SketchBio::Project *proj, bool const * const b,
                     double const * const a);
    ~GroupEditingMode();
    
    virtual void buttonPressed(int vrpn_ButtonNum);
    virtual void buttonReleased(int vrpn_ButtonNum);
    virtual void analogsUpdated();
    virtual void doUpdatesForFrame();
};

#endif // GROUPEDITINGMODE_H
