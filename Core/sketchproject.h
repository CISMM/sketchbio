#ifndef SKETCHPROJECT_H
#define SKETCHPROJECT_H

// VRPN/quatlib dependencies
#include <quat.h>

// VTK dependencies
#include <vtkSmartPointer.h>
class vtkRenderer;
class vtkPlaneSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkCamera;
class vtkMatrix4x4;

// QT dependencies
#include <QVector>
#include <QPair>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QWeakPointer>
#include <QList>
#include <QHash>
#include <QString>
#include <QTime>
class QDir;

// SketchBio dependencies
class SketchModel;
class ModelManager;
class TransformManager;
class SketchObject;
class Connector;
class WorldManager;
class StructureReplicator;
class TransformEquals;
#include "physicsstrategyfactory.h"
class UndoState;

// constants to use with the setOutlineXXX and isOutlineVisible functions
// for the value of the side variable
#define LEFT_SIDE_OUTLINE 0
#define RIGHT_SIDE_OUTLINE 1

/*
 *
 * This class encapsulates all the data about what the user has done.  This allows these objects to be
 * swapped out at runtime so that projects can be saved and loaded.  Also, all data that is saved as part
 * of the project state must be in this class.  Each project has a directory where it copies all model
 * files that are added to it.
 *
 */
class SketchProject
{
public:
    SketchProject(vtkRenderer* r, const QString& projDir);
    ~SketchProject();
    // setters
    // sets the directory path for this project (should be an absolute path)
    bool setProjectDir(const QString& dir);
    // sets the left hand position/orientation from vrpn
    void setLeftHandPos(q_xyz_quat_type* loc);
    // sets the right hand position/orientation from vrpn
    void setRightHandPos(q_xyz_quat_type* loc);
    // sets the viewpoint
    void setViewpoint(vtkMatrix4x4* worldToRoom, vtkMatrix4x4* roomToEyes);
    // sets the collision mode
    void setCollisionMode(PhysicsMode::Type mode);
    // toggle settings in the physics
    void setCollisionTestsOn(bool on);
    void setWorldSpringsEnabled(bool enabled);
    // animation
    void startAnimation();
    void stopAnimation();
    // returns true if the animation preview is being played
    bool isShowingAnimation();
    // a setter for world->setAnimationTime.  Sets the project state correctly
    // if it is not set yet
    // Returns true if the animation is done by the time given
    bool goToAnimationTime(double time);
    // turning on and off shadows
    bool isShowingShadows() const;
    void setShowShadows(bool show);
    void setShadowsOn();
    void setShadowsOff();

    // clears everything that the user has done in the project in preparation
    // for loading in an undo state
    void clearProject();

    // functions for undo and redo
    // Note: these two will return NULL if there is no state to return
    UndoState* getLastUndoState();
    UndoState* getNextRedoState();
    // pops the last undo state off the stack entirely and deletes it
    void popUndoState();
    // clears the redo operation stack in addition to adding an undo state
    void addUndoState(UndoState* state);
    void applyUndo();
    void applyRedo();

    // returns the current time in the animation that is being viewed
    double getViewTime() const;
    // sets the current time in the animation that is being viewed
    void setViewTime(double time);

    // physics/time related functions - timestep either updates physics or moves the animation one time frame
    void timestep(double dt);

    // get model manager
    const ModelManager* getModelManager() const;
    ModelManager* getModelManager();
    // get transform manager
    const TransformManager* getTransformManager() const;
    TransformManager* getTransformManager();
    // get world manager
    const WorldManager* getWorldManager() const;
    WorldManager* getWorldManager();
    // get objects representing trackers ('hands')
    SketchObject* getLeftHandObject();
    SketchObject* getRightHandObject();
    // get replicas
    const QList< StructureReplicator* >* getReplicas() const;
    // get transform equals objects (or more things added later, not sure)
    const QVector< QSharedPointer< TransformEquals > >* getTransformOps() const;
    // get cameras hash
    const QHash< SketchObject* , vtkSmartPointer< vtkCamera > >* getCameras() const;
    // number of replicas
    int getNumberOfReplications() const;
    // number of transform equals (or more stuff... see comment on getTransformOps())
    int getNumberOfTransformOps() const;
    // gets the directory path for this project (absolute path)
    QString getProjectDir() const;
    // gets the file in the project directory that matches the given file
    // this will copy the file to the project directory if it can, and if
    // that fails will return the file as-is if it is outside the project directory
    QString getFileInProjDir(const QString& filename);
    // gets the camera model
    SketchModel* getCameraModel();

    // update locations of tracker objects
    void updateTrackerPositions();
    // update the object used for the outlines
    void setOutlineObject(int outlineIdx, SketchObject* obj);
    // update the spring used for the outlines
    void setOutlineSpring(int outlineIdx, Connector* conn, bool end1Large);
    // show/hide the outlines of objects
    void setOutlineVisible(int outlineIdx, bool visible);
    // tell if the outlines for a particular side are visible
    bool isOutlineVisible(int outlineIdx);

    // causes the given object to be connected to one of the tracker objects with springs to allow
    // the user to manipulate its position and orientation.
    // the first parameter is the object to attach
    // the second parameter should be true if the object should be attached to the left tracker
    //          and false if the object should be attached to the right tracker
    void grabObject(SketchObject *objectToGrab, bool grabWithLeft);

    // adding things functions
    // for models
    SketchModel* addModel(SketchModel* model);
    SketchModel* addModelFromFile(const QString& source, const QString& filename,
                                  double imass, double imoment);
    // for objects
    SketchObject* addObject(SketchModel* model, const q_vec_type pos, const q_type orient);
    SketchObject* addObject(const QString& source, const QString& filename);
    SketchObject* addObject(SketchObject* obj);
    SketchObject* addCamera(const q_vec_type pos, const q_type orient);
    // Given the vtk camera position, creates a camera object that has that position
    // and view
    SketchObject* addCameraObjectFromCameraPosition(vtkCamera* cam);
    void setCameraToVTKCameraPosition(SketchObject* cam, vtkCamera* vcam);
    // adds objects for the given list of filenames
    bool addObjects(const QVector<QString>& filenames);
    // for springs between objects (object-tracker springs managed internally)
    Connector *addSpring(SketchObject* o1, SketchObject* o2, double minRest, double maxRest,
                       double stiffness, q_vec_type o1Pos, q_vec_type o2Pos);
    // for other springs
    Connector* addConnector(Connector* spring);
    // for structure replication chains
    StructureReplicator* addReplication(SketchObject* o1, SketchObject* o2, int numCopies);
    void addReplication(StructureReplicator* rep);
    // for transform equals
    QWeakPointer<TransformEquals> addTransformEquals(SketchObject* o1, SketchObject* o2);
public:
    // sets up the vtkCamera object to the position and orientation of the given
    // SketchObject.  Should only pass cameras for the SketchObject...
    static void setUpVtkCamera(SketchObject* cam, vtkCamera* vCam);
private:
    // helper functions
    // input related functions
    void handleInput();
    void updateTrackerObjectConnections();

    // fields
    vtkSmartPointer< vtkRenderer > renderer;
	QTime time;
    // managers -- these are owned by the project, raw pointers may be passed to other places, but
    //              will be invalid once the project is deleted
    QScopedPointer< ModelManager > models;
    QScopedPointer< TransformManager > transforms;
    QScopedPointer< WorldManager > world;

    // operators on objects -- these are owned here.
    QList< StructureReplicator* > replicas;
    QHash< SketchObject* , vtkSmartPointer< vtkCamera > > cameras;
    QVector< QSharedPointer< TransformEquals > > transformOps;

    // project dir
    QDir *projectDir;

    // undo states:
    QList< UndoState* > undoStack, redoStack;

    // other ui stuff
    SketchObject* leftHand, * rightHand; // the objects for the left and right hand trackers
    // the left and right trackers' shadows
    vtkSmartPointer< vtkActor > leftShadowActor, rightShadowActor;
    // outline actors are added to the renderer when the object is close enough to
    // interact with.  The outline mappers are updated whenever the closest object
    // changes (unless another one is currently grabbed). Note: left is 0, right is 1
    QVector< QPair< vtkSmartPointer< vtkPolyDataMapper >,
        vtkSmartPointer< vtkActor > > > outlines;

    vtkSmartPointer< vtkPlaneSource > shadowFloorSource;
    vtkSmartPointer< vtkActor > shadowFloorActor, floorLinesActor;
    // animation stuff
    bool isDoingAnimation; // true if the animation is happenning
    bool showingShadows; // true if shadows are currently being shown
    double timeInAnimation; // the animation time starting at 0
    double viewTime;
};

inline const ModelManager* SketchProject::getModelManager() const {
    return models.data();
}

inline ModelManager* SketchProject::getModelManager()
{
    return models.data();
}

inline const TransformManager* SketchProject::getTransformManager() const
{
    return transforms.data();
}

inline TransformManager* SketchProject::getTransformManager()
{
    return transforms.data();
}

inline const WorldManager* SketchProject::getWorldManager() const
{
    return world.data();
}

inline WorldManager* SketchProject::getWorldManager()
{
    return world.data();
}

inline SketchObject* SketchProject::getLeftHandObject()
{
    return leftHand;
}

inline SketchObject* SketchProject::getRightHandObject()
{
    return rightHand;
}

inline const QList< StructureReplicator* >* SketchProject::getReplicas() const
{
    return &replicas;
}

inline int SketchProject::getNumberOfReplications() const
{
    return replicas.size();
}

inline const QVector< QSharedPointer< TransformEquals > >*
        SketchProject::getTransformOps() const
{
    return &transformOps;
}

inline int SketchProject::getNumberOfTransformOps() const
{
    return transformOps.size();
}

inline const QHash< SketchObject* , vtkSmartPointer< vtkCamera > >*
        SketchProject::getCameras() const
{
    return &cameras;
}


#endif // SKETCHPROJECT_H
