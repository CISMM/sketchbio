#ifndef SPRINGEDITINGMODE_H
#define SPRINGEDITINGMODE_H

#include "objectgrabmode.h"

class Connector;

class SpringEditingMode : public ObjectGrabMode
{
  Q_OBJECT
 public:
  SpringEditingMode(SketchBio::Project *proj, bool const *buttonState,
                    double const *analogState);
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
  bool snapMode;
};

#endif  // SPRINGEDITINGMODE_H
