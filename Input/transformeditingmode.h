#ifndef OBJECTEDITINGMODE_H
#define OBJECTEDITINGMODE_H

#include "objectgrabmode.h"
#include <QVector>

class SketchObject;

class TransformEditingMode : public ObjectGrabMode
{
  Q_OBJECT
 public:
  TransformEditingMode(SketchProject *proj, const bool *b, const double *a);
  virtual ~TransformEditingMode();
  // Called anytime a button is pressed
  virtual void buttonPressed(int vrpn_ButtonNum);
  // Called anytime a button is released
  virtual void buttonReleased(int vrpn_ButtonNum);
  // Called when the analogs status have been updated
  virtual void analogsUpdated();
  // TEMPLATE METHOD
  // this will be called from within setProject after the new project is set
  // it should refresh the status and delete all references to the old project
  virtual void clearStatus();
  virtual void doUpdatesForFrame();
 private slots:
  void setTransformBetweenActiveObjects(double translateX, double translateY,
                                        double translateZ, double rotateX,
                                        double rotateY, double rotateZ);
  void cancelSetTransforms();

 private:
  int operationState;
  QVector< SketchObject * > objectsSelected;
};

#endif  // OBJECTEDITINGMODE_H
