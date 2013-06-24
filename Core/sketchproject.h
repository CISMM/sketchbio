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
#include <QSharedPointer>
#include <QScopedPointer>
#include <QWeakPointer>
#include <QList>
#include <QHash>
#include <QString>
class QDir;

// SketchBio dependencies
class SketchModel;
class ModelManager;
class TransformManager;
class SketchObject;
class SpringConnection;
class WorldManager;
class StructureReplicator;
class TransformEquals;
#include "physicsstrategyfactory.h"

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
    SketchProject(vtkRenderer *r);
    ~SketchProject();
    // setters
    // sets the directory path for this project (should be an absolute path)
    bool setProjectDir(QString dir);
    // sets the left hand position/orientation
    void setLeftHandPos(q_xyz_quat_type *loc);
    // sets the right hand position/orientation
    void setRightHandPos(q_xyz_quat_type *loc);
    // sets the viewpoint
    void setViewpoint(vtkMatrix4x4 *worldToRoom, vtkMatrix4x4 *roomToEyes);
    // sets the collision mode (testing only)
    void setCollisionMode(PhysicsMode::Type mode);
    // toggle settings in the physics
    void setCollisionTestsOn(bool on);
    void setWorldSpringsEnabled(bool enabled);
    // animation
    void startAnimation();
    void stopAnimation();
    // an setter for world->setAnimationTime.  Sets the project state correctly
    // if it is not set yet
    // Returns true if the animation is done by the time given
    bool goToAnimationTime(double time);
    // turning on and off shadows
    bool isShowingShadows() const;
    void setShowShadows(bool show);
    void setShadowsOn();
    void setShadowsOff();

    // returns the current time in the animation that is being viewed
    double getViewTime() const;
    // sets the current time in the animation that is being viewed
    void setViewTime(double time);

    // physics/time related functions - timestep either updates physics or moves the animation one time frame
    void timestep(double dt);

    // get model manager
    const ModelManager *getModelManager() const;
    // get transform manager
    const TransformManager *getTransformManager() const;
    TransformManager *getTransformManager();
    // get world manager
    const WorldManager *getWorldManager() const;
    WorldManager *getWorldManager();
    // get objects representing trackers ('hands')
    SketchObject *getLeftHandObject();
    SketchObject *getRightHandObject();
    // get replicas
    const QList<StructureReplicator *> *getReplicas() const;
    // get transform equals objects (or more things added later, not sure)
    const QVector<QSharedPointer<TransformEquals> > *getTransformOps() const;
    // get cameras hash
    const QHash<SketchObject *, vtkSmartPointer<vtkCamera> > *getCameras() const;
    // number of replicas
    int getNumberOfReplications() const;
    // number of transform equals (or more stuff... see comment on getTransformOps())
    int getNumberOfTransformOps() const;
    // gets the directory path for this project (absolute path)
    QString getProjectDir() const;
    // gets the camera model
    SketchModel *getCameraModel();

    // update locations of tracker objects
    void updateTrackerPositions();
    // update the object used for the outlines
    void setLeftOutlineObject(SketchObject *obj);
    void setLeftOutlineSpring(SpringConnection *conn, bool end1Large);
    void setRightOutlineObject(SketchObject *obj);
    void setRightOutlineSpring(SpringConnection *conn, bool end1Large);
    // show/hide the outlines of objects
    void setLeftOutlinesVisible(bool visible);
    void setRightOutlinesVisible(bool visible);
    // tell if the outlines for a particular side are visible
    bool isLeftOutlinesVisible();
    bool isRightOutlinesVisible();

    // causes the given object to be connected to one of the tracker objects with springs to allow
    // the user to manipulate its position and orientation.
    // the first parameter is the object to attach
    // the second parameter should be true if the object should be attached to the left tracker
    //          and false if the object should be attached to the right tracker
    void grabObject(SketchObject *objectToGrab, bool grabWithLeft);

    // adding things functions
    // for models
    SketchModel *addModel(SketchModel *model);
    SketchModel *addModelFromFile(QString source, QString filename, double imass, double imoment);
    // for objects
    SketchObject *addObject(SketchModel *model, const q_vec_type pos, const q_type orient);
    SketchObject *addObject(QString source, QString filename);
    SketchObject *addObject(SketchObject *obj);
    SketchObject *addCamera(const q_vec_type pos, const q_type orient);
    bool addObjects(QVector<QString> filenames);
    // for springs between objects (object-tracker springs managed internally)
    SpringConnection *addSpring(SketchObject *o1, SketchObject *o2, double minRest, double maxRest,
                       double stiffness, q_vec_type o1Pos, q_vec_type o2Pos);
    // for other springs
    SpringConnection *addSpring(SpringConnection *spring);
    // for structure replication chains
    StructureReplicator *addReplication(SketchObject *o1, SketchObject *o2, int numCopies);
    // for transform equals
    QWeakPointer<TransformEquals> addTransformEquals(SketchObject *o1, SketchObject *o2);
private:
    // helper functions
    // input related functions
    void handleInput();
    void updateTrackerObjectConnections();
    static void setUpVtkCamera(SketchObject *cam, vtkCamera *vCam);

    // fields
    vtkSmartPointer< vtkRenderer > renderer;
    // managers -- these are owned by the project, raw pointers may be passed to other places, but
    //              will be invalid once the project is deleted
    QScopedPointer< ModelManager > models;
    QScopedPointer< TransformManager > transforms;
    QScopedPointer< WorldManager > world;
    // operators on objects -- these are owned here... but not sure if I can do lists of ScopedPointers
    QList< StructureReplicator * > replicas;
    QHash< SketchObject *, vtkSmartPointer< vtkCamera > > cameras;
    QVector< QSharedPointer< TransformEquals > > transformOps;

    // project dir
    QDir *projectDir;

    // other ui stuff
    SketchObject *leftHand, *rightHand; // the objects for the left and right hand trackers
    // the left and right trackers' shadows
    vtkSmartPointer< vtkActor > leftShadowActor, rightShadowActor;
    // outline actors are added to the renderer when the object is close enough to
    // interact with.  The outline mappers are updated whenever the closest object
    // changes (unless another one is currently grabbed).
    vtkSmartPointer< vtkActor > leftOutlinesActor, rightOutlinesActor;
    vtkSmartPointer< vtkPolyDataMapper > leftOutlinesMapper, rightOutlinesMapper;
    vtkSmartPointer< vtkPlaneSource > shadowFloorSource;
    vtkSmartPointer< vtkActor > shadowFloorActor, floorLinesActor;
    // animation stuff
    bool isDoingAnimation; // true if the animation is happenning
    bool showingShadows; // true if shadows are currently being shown
    double timeInAnimation; // the animation time starting at 0
    double viewTime;
};

inline const ModelManager *SketchProject::getModelManager() const {
    return models.data();
}

inline const TransformManager *SketchProject::getTransformManager() const {
    return transforms.data();
}

inline TransformManager *SketchProject::getTransformManager() {
    return transforms.data();
}

inline const WorldManager *SketchProject::getWorldManager() const {
    return world.data();
}

inline WorldManager *SketchProject::getWorldManager() {
    return world.data();
}

inline SketchObject *SketchProject::getLeftHandObject() {
    return leftHand;
}

inline SketchObject *SketchProject::getRightHandObject() {
    return rightHand;
}

inline const QList<StructureReplicator *> *SketchProject::getReplicas() const {
    return &replicas;
}

inline int SketchProject::getNumberOfReplications() const {
    return replicas.size();
}

inline const QVector<QSharedPointer<TransformEquals> > *SketchProject::getTransformOps() const {
    return &transformOps;
}

inline int SketchProject::getNumberOfTransformOps() const {
    return transformOps.size();
}

inline const QHash<SketchObject *, vtkSmartPointer<vtkCamera> > *SketchProject::getCameras() const {
    return &cameras;
}


#endif // SKETCHPROJECT_H
