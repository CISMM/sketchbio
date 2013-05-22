#ifndef SimpleViewUI_H
#define SimpleViewUI_H

#define SIMPLEVIEW_NUM_ACTORS 4
 
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkRenderer.h>

#include <quat.h>

#include "transformmanager.h"
#include "modelmanager.h"
#include "worldmanager.h"
#include "structurereplicator.h"
#include "sketchioconstants.h"
#include "sketchproject.h"
#include "transformequals.h"
#include "hydrainputmanager.h"

#include <QMainWindow>
#include <QProgressDialog>
#include <QActionGroup>
#include <QTimer>
#include <QString>
#include <QDebug>
#include <QVector>
 
// Forward Qt class declarations
class Ui_SimpleView;
class vtkTextMapper;

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
  // Run an external pymol script to download and surface the model
  // Put the resulting model in the models/ directory.
  void importPDBId();

  // Export an animation to blender
  // currently writes the test file so I can see if it is correct
  void exportBlenderAnimation();

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
  void toggleWorldSpringsEnabled();
  void setCollisionTestsOn(bool on);
  void toggleWorldCollisionTestsOn();

  // Load a project
  void loadProject();

  virtual void slotExit();
  void slot_frameLoop();

  void setTextMapperString(QString str);
  void updateStatusText();

protected:
 
protected slots:
 
private:

  // Methods
  void addActor(vtkActor *actor, q_vec_type position, q_type orientation);

  // Fields
  Ui_SimpleView *ui;
  QTimer *timer;
  QActionGroup *collisionModeGroup;
  vtkSmartPointer<vtkRenderer> renderer;
  vtkSmartPointer<vtkTextMapper> directionsTextMapper;
  vtkSmartPointer<vtkActor2D> directionsTextActor;
  vtkSmartPointer<vtkTextMapper> statusTextMapper;
  vtkSmartPointer<vtkActor2D> statusTextActor;
  SketchProject *project;
  HydraInputManager *inputManager;
};


// A dialog that sends signals correctly so that I can delete it after the animation is
// finished/cancelled.
class MyDialog : public QProgressDialog {
    Q_OBJECT
public:
    explicit MyDialog(const QString &a, const QString &b, int min, int max, QWidget *parent = 0) :
        QProgressDialog(a,b,min,max,parent)
    {
        setModal(true);
    }
    virtual ~MyDialog() {
        qDebug() << "Deleting dialog";
    }
public slots:
    void resetAndSignal() {
        QProgressDialog::reset();
        emit deleteMe();
    }
signals:
    void deleteMe();
};

#endif // SimpleViewUI_H
