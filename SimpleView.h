#ifndef SimpleViewUI_H
#define SimpleViewUI_H

#define SIMPLEVIEW_NUM_ACTORS 4
 
#include <vtkSmartPointer.h>
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include "transformmanager.h"
#include <quat.h>
#include <QMainWindow>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkRenderer.h>
#include <QTimer>
#include "modelmanager.h"
#include "worldmanager.h"
#include "structurereplicator.h"

 
// Forward Qt class declarations
class Ui_SimpleView;

#define NUM_HYDRA_BUTTONS 16
#define NUM_HYDRA_ANALOGS 6

// VRPN callback functions
void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b);
void VRPN_CALLBACK handle_analogs(void *userdata, const vrpn_ANALOGCB a);
 
// SimpleView class
class SimpleView : public QMainWindow
{
  Q_OBJECT
public:
 
  // Constructor/Destructor
  SimpleView(); 
  ~SimpleView();
  void setLeftPos(q_xyz_quat_type *newPos);
  void setRightPos(q_xyz_quat_type *newPos);
  void setButtonState(int buttonNum, bool buttonPressed);
  void setAnalogStates(const double state[]);
 
public slots:
 
  virtual void slotExit();
  void slot_frameLoop();
 
protected:
 
protected slots:
 
private:

  // Methods
  void addActor(vtkActor *actor, q_vec_type position, q_type orientation);

  // Fields
  Ui_SimpleView *ui;
  vrpn_Tracker_Remote tracker;
  vrpn_Button_Remote buttons;
  vrpn_Analog_Remote analogRemote;
  bool buttonDown[NUM_HYDRA_BUTTONS]; // number of buttons, each w/ 2 states pressed & not
                       // 1 = pressed, 0 = not pressed.
  double analog[NUM_HYDRA_ANALOGS]; // number of analogs for hyrda
  QTimer *timer;
  vtkSmartPointer<vtkRenderer> renderer;
  ModelManager models;
  WorldManager world;
  TransformManager transforms;
  StructureReplicator *copies;
  vtkSmartPointer<vtkTransform> left;
  vtkSmartPointer<vtkTransform> right;
};

 
#endif // SimpleViewUI_H
