#ifndef SimpleViewUI_H
#define SimpleViewUI_H

#define SIMPLEVIEW_NUM_ACTORS 4

#include <quat.h>
 
#include <vtkSmartPointer.h>
class vtkActor2D;
class vtkTextMapper;
class vtkRenderer;

#include <QMainWindow>
class QTimer;
class QThread;
class QActionGroup;
#include <QString>

class SketchObject;
class SketchProject;

class vrpnServer;
class HydraInputManager;

class SubprocessRunner;
 
// Forward Qt class declaration (the view)
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
  SimpleView(QString projDir, bool load_example = false);
  virtual ~SimpleView();

  // Add an object (or objects) to be displayed.
  SketchObject *addObject(QString name);
  bool addObjects(QVector<QString> names);

  // Simplify an external object based on the root file name.
  void simplifyObjectByName(const QString name);
 
public slots:

  // Throw a dialog box to browse for an OBJ file to load
  void openOBJFile();

  // Throw a dialog box to type in the name of a PDB ID to import
  // and the chains to remove
  // Run an external Chimera script to download and surface the model
  // Put the resulting model in the project directory.
  void importPDBId();
  // Use a file dialog to select a local PDB file to open and import
  // Also gives a dialog for the chains to remove
  // as a model.  Runs an external Chimera script to surface the model
  // and puts the result in the project folder
  void openPDBFile();

  // Export an animation to blender
  // currently writes the test file so I can see if it is correct
  void exportBlenderAnimation();

  // Throw a dialog box to browse for an OBJ file to
  // simplify.  Produce multiple simplifications, with smaller
  // fractional polygon counts.
  void simplifyOBJFile();

  // Restarts the internal vrpn server if there is one.
  void restartVRPNServer();

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
  void toggleWorldSpringsEnabled();
  void setCollisionTestsOn(bool on);
  void toggleWorldCollisionTestsOn();

  // Load a project
  void loadProject();

  virtual void closeEvent(QCloseEvent *);
  virtual void slotExit();
  void slot_frameLoop();

  void setTextMapperString(QString str);
  void updateStatusText();
  void updateViewTime(double time);
  void goToViewTime();

  void createCameraForViewpoint();
  void setCameraToViewpoint();

protected:
 
protected slots:
  void addUndoStateIfSuccess(bool success);
 
private:

  // Methods
  void runSubprocessAndFreezeGUI(SubprocessRunner *runner,
                                 bool needsUndoState = false);

  // Fields
  Ui_SimpleView *ui;
  QTimer *timer;
  vrpnServer *server;
  QThread *serverThread;
  QActionGroup *collisionModeGroup;
  vtkSmartPointer<vtkRenderer> renderer;
  vtkSmartPointer<vtkTextMapper> directionsTextMapper;
  vtkSmartPointer<vtkActor2D> directionsTextActor;
  vtkSmartPointer<vtkTextMapper> statusTextMapper;
  vtkSmartPointer<vtkActor2D> statusTextActor;
  vtkSmartPointer<vtkTextMapper> timeTextMapper;
  vtkSmartPointer<vtkActor2D> timeTextActor;
  SketchProject *project;
  HydraInputManager *inputManager;
};


#endif // SimpleViewUI_H
