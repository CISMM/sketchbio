#ifndef COLOREDITINGMODE_H
#define COLOREDITINGMODE_H

#include "objectgrabmode.h"

class Connector;

class ColorEditingMode : public ObjectGrabMode
{
  Q_OBJECT
 public:
  ColorEditingMode(SketchBio::Project *proj, bool const *const b,
                   double const *const a);
  virtual ~ColorEditingMode();
  virtual void buttonPressed(int vrpn_ButtonNum);
  virtual void buttonReleased(int vrpn_ButtonNum);
  virtual void analogsUpdated();
  void doUpdatesForFrame();
};

#endif  // COLOREDITINGMODE_H
