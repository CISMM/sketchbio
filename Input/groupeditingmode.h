#ifndef GROUPEDITINGMODE_H
#define GROUPEDITINGMODE_H

#include "objectgrabmode.h"

class GroupEditingMode : public ObjectGrabMode
{
    Q_OBJECT
public:
    GroupEditingMode(SketchProject *proj, bool const * const b,
                     double const * const a);
    ~GroupEditingMode();
    
    virtual void buttonPressed(int vrpn_ButtonNum);
    virtual void buttonReleased(int vrpn_ButtonNum);
    virtual void analogsUpdated();
};

#endif // GROUPEDITINGMODE_H
