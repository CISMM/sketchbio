#ifndef SimpleViewUI_H
#define SimpleViewUI_H
 
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vrpn_Tracker.h"
#include <QMainWindow>
#include <QTimer>

#include <quat.h>
 
// Forward Qt class declarations
class Ui_SimpleView;
void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t);
 
class SimpleView : public QMainWindow
{
  Q_OBJECT
public:
 
  // Constructor/Destructor
  SimpleView(); 
  ~SimpleView();
  void setLeftPos(q_xyz_quat_type *xyzQuat);
  void setRightPos(q_xyz_quat_type *xyzQuat);
 
public slots:
 
  virtual void slotExit();
  void slot_vrpnLoop();
 
protected:
 
protected slots:
 
private:
 
  // Designer form
  Ui_SimpleView *ui;
  vrpn_Tracker_Remote tracker;
  QTimer *timer;
  vtkSmartPointer<vtkTransform> left;
  vtkSmartPointer<vtkTransform> lInv;
  vtkSmartPointer<vtkTransform> right;
  vtkSmartPointer<vtkTransform> master;
};
 
#endif // SimpleViewUI_H
