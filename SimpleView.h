#ifndef SimpleViewUI_H
#define SimpleViewUI_H

#define SIMPLEVIEW_NUM_ACTORS 4
 
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkRenderer.h>

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <quat.h>

#include "transformmanager.h"
#include "modelmanager.h"
#include "worldmanager.h"
#include "structurereplicator.h"
#include "sketchioconstants.h"
#include "sketchproject.h"

#include <QMainWindow>
#include <QActionGroup>
#include <QTimer>
#include <QString>
#include <QVector>
 
// Forward Qt class declarations
class Ui_SimpleView;

/*
 * SimpleView is the main GUI class in the SketchBio project.  It also manages
 * the vrpn input to the program.
 */
class SimpleView : public QMainWindow
{
  Q_OBJECT
public:
 
  // Constructor/Destructor
  SimpleView(QString projDir, bool load_fibrin = true, bool fibrin_springs = true,
	     bool do_replicate = true); 
  ~SimpleView();
  void setLeftPos(q_xyz_quat_type *newPos);
  void setRightPos(q_xyz_quat_type *newPos);
  void setButtonState(int buttonNum, bool buttonPressed);
  void setAnalogStates(const double state[]);

  // Add an object (or objects) to be displayed.
  SketchObject *addObject(QString name);
  bool addObjects(QVector<QString> names);

  // Simplify an external object based on the root file name.
  bool simplifyObjectByName(const QString name);
 
public slots:

  // Throw a dialog box to browse for an OBJ file to load
  void openOBJFile();

  // Throw a dialog box to type in the name of a PDB ID to import
  // Run an external pymol script to download and surface the model
  // Put the resulting model in the models/ directory.
  void importPDBId();

  // Throw a dialog box to browse for an OBJ file to
  // simplify.  Produce multiple simplifications, with smaller
  // fractional polygon counts.
  void simplifyOBJFile();

  // Save the current project
  void saveProjectAs();
  void saveProject();

  // Collision modes (for testing)
  void oldCollisionMode();
  void poseModeTry1();
  void binaryCollisionSearch();
  void poseModePCA();

  // Physics settings
  void setWorldSpringsEnabled(bool enabled);
  void setCollisionTestsOn(bool on);

  // Load a project
  void loadProject();

  virtual void slotExit();
  void slot_frameLoop();
 
protected:
 
protected slots:
 
private:

  // VRPN callback functions
  static void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t);
  static void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b);
  static void VRPN_CALLBACK handle_analogs(void *userdata, const vrpn_ANALOGCB a);

  // Methods
  void addActor(vtkActor *actor, q_vec_type position, q_type orientation);
  void handleInput();

  // Fields
  Ui_SimpleView *ui;
  vrpn_Tracker_Remote tracker;
  vrpn_Button_Remote buttons;
  vrpn_Analog_Remote analogRemote;
  bool buttonDown[NUM_HYDRA_BUTTONS]; // number of buttons, each w/ 2 states pressed & not
                       // 1 = pressed, 0 = not pressed.
  double analog[NUM_HYDRA_ANALOGS]; // number of analogs for hyrda
  QTimer *timer;
  QActionGroup *collisionModeGroup;
  vtkSmartPointer<vtkRenderer> renderer;
  SketchProject *project;
};

 
#endif // SimpleViewUI_H
