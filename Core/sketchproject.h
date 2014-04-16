#ifndef SKETCHPROJECT_H
#define SKETCHPROJECT_H

// VRPN/quatlib dependencies
#include <quat.h>

// VTK dependencies
#include <vtkSmartPointer.h>
class vtkRenderer;
class vtkPlaneSource;
class vtkLinearTransform;
class vtkActor;
class vtkPolyDataMapper;
class vtkCamera;
class vtkMatrix4x4;

// QT dependencies
#include <QVector>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QList>
#include <QHash>
#include <QString>
#include <QTime>

// SketchBio dependencies
#include "sketchioconstants.h"
class SketchModel;
class ModelManager;
class TransformManager;
class SketchObject;
class Connector;
class WorldManager;
class StructureReplicator;
class TransformEquals;
class UndoState;

namespace SketchBio
{
// forward declare here so it matches with actual definition namespace
class Hand;

class OperationState
{
   public:
    OperationState() {}
    virtual ~OperationState() {}
    virtual void doFrameUpdates() {}
};

class ProjectObserver
{
   public:
    virtual ~ProjectObserver() {}
    // Called whenever an operation calls setDirections, passing on the
    // given string.  When clearDirections is called, it is passed the
    // empty string.
    virtual void newDirections(const QString &string) {}
    // Called when the view time of the project is changed
    virtual void viewTimeChanged(double newTime) {}
};

/*
 *
 * This class encapsulates all the data about what the user has done.  This
 *allows these objects to be
 * swapped out at runtime so that projects can be saved and loaded.  Also, all
 *data that is saved as part
 * of the project state must be in this class.  Each project has a directory
 *where it copies all model
 * files that are added to it.
 *
 */
class Project
{
   public:
    Project(vtkRenderer* r, const QString& projDir);
    ~Project();
    // #################################################################
    // Accessors:
    // methods for getting member variables and information about them
    // the model manager (used to add and remove models)
    ModelManager& getModelManager();
    const ModelManager& getModelManager() const;
    // the world manager (used to add and remove objects and connectors
    // and change physics settings)
    WorldManager& getWorldManager();
    const WorldManager& getWorldManager() const;
    // the transform manager (used to set relative locations of things and
    // positions and orientations of the trackers)
    TransformManager& getTransformManager();
    const TransformManager& getTransformManager() const;
    // the hand, knows position of the trackers and how to grab things
    Hand& getHand(SketchBioHandId::Type side);
    // the list of crystal-by-example structures
    const QList< StructureReplicator* >& getCrystalByExamples() const;
    int getNumberOfCrystalByExamples() const;
    // the list of transform operations (lock transforms for now)
    const QVector< QSharedPointer< TransformEquals > >& getTransformOps() const;
    int getNumberOfTransformOps() const;
    // the hash of camera objects and their corresponding vtkCamera objects
    const QHash< SketchObject*, vtkSmartPointer< vtkCamera > >& getCameras()
        const;
    // the project directory
    QString getProjectDir() const;
    // ###################################################################
    // Frame update functions:
    // functions to update things every frame
    // update locations of tracker objects
    void updateTrackerPositions();
    // physics/time related functions - timestep either updates physics or
    // moves the animation one time frame
    void timestep(double dt);
    // ###################################################################
    // Animation and time functions:
    // animation
    void startAnimation();
    void stopAnimation();
    // returns true if the animation preview is being played
    bool isShowingAnimation();
    // a setter for world->setAnimationTime.  Sets the project state correctly
    // if it is not set yet
    // Returns true if the animation is done by the time given
    bool goToAnimationTime(double time);
    // returns the current time in the animation that is being viewed
    double getViewTime() const;
    // sets the current time in the animation that is being viewed
    void setViewTime(double time);
    // ###################################################################
    // Undo and Redo functions:
    // Note: these two will return NULL if there is no state to return
    UndoState* getLastUndoState();
    UndoState* getNextRedoState();
    // pops the last undo state off the stack entirely and deletes it
    void popUndoState();
    // clears the redo operation stack in addition to adding an undo state
    void addUndoState(UndoState* state);
    void applyUndo();
    void applyRedo();
    // ###################################################################
    // User operation state functions:
    // get and set operation state for user operations with persistent state
    OperationState* getOperationState();
    void setOperationState(OperationState* state);
    // ###################################################################
    // clears everything that the user has done in the project in preparation
    // for loading in an undo state
    void clearProject();
    // ###################################################################
    // Project directory functions
    // sets the directory path for this project (should be an absolute path)
    // note, the folder will get deleted and replaced with the contents of the
    // current project folder.
    bool setProjectDir(const QString& dir);
    // gets the file in the project directory that matches the given file
    // this will copy the file to the project directory if it can, and if
    // that fails will return the file as-is if it is outside the project
    // directory
    // the new filename is returned in the newName parameter and the bool return
    // value indicates that the project directory now contains the given file
    bool getFileInProjDir(const QString& filename, QString& newName);
    // ###################################################################
    // Shadow functions:
    // turning on and off shadows
    bool isShowingShadows() const;
    void setShowShadows(bool show);
    void setShadowsOn();
    void setShadowsOff();
    // ###################################################################
    // Camera functions:
    SketchModel* getCameraModel();
    SketchObject* addCamera(const q_vec_type pos, const q_type orient);
    // Given the vtk camera position, creates a camera object that has that
    // position
    // and view
    SketchObject* addCameraObjectFromCameraPosition(vtkCamera* cam);
    void setCameraToVTKCameraPosition(SketchObject* cam, vtkCamera* vcam);
    // ###################################################################
    // Adding things functions
    // for structure replication chains
    StructureReplicator* addReplication(SketchObject* o1, SketchObject* o2,
                                        int numCopies);
    void addReplication(StructureReplicator* rep);
    // for transform equals
    QWeakPointer< TransformEquals > addTransformEquals(SketchObject* o1,
                                                       SketchObject* o2);
    // ###################################################################
    // ProjectObserver
    // these allow external notification of the directions that operations
    // will send as alerts
    void setDirections(const QString& directions);
    void clearDirections();
    void addProjectObserver(ProjectObserver* p);
    void removeProjectObserver(ProjectObserver* p);

   public:
    // sets up the vtkCamera object to the position and orientation of the given
    // SketchObject.  Should only pass cameras for the SketchObject...
    static void setUpVtkCamera(SketchObject* cam, vtkCamera* vCam);

   private:
    class ProjectImpl;
    ProjectImpl* impl;
};
}

#endif  // SKETCHPROJECT_H
