#include "sketchproject.h"

#include <limits>

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkOBJReader.h>
#include <vtkPlaneSource.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkConeSource.h>
#include <vtkExtractEdges.h>
#include <vtkArrayCalculator.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkNew.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourFilter.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>

#include <QDebug>
#include <QDir>
#include <QPair>

#include <vtkProjectToPlane.h>

#include "sketchioconstants.h"
#include "transformmanager.h"
#include "colormaptype.h"
#include "modelmanager.h"
#include "sketchobject.h"
#include "worldmanager.h"
#include "objectchangeobserver.h"
#include "springconnection.h"
#include "structurereplicator.h"
#include "transformequals.h"
#include "undostate.h"
#include "hand.h"

#define NUM_COLORS (6)
/*
static double COLORS[][3] =
  {  { 1.0, 0.7, 0.7 },
     { 0.7, 1.0, 0.8 },
     { 0.7, 0.7, 1.0 },
     { 1.0, 1.0, 0.7 },
     { 1.0, 0.7, 1.0 },
     { 0.7, 1.0, 1.0 } };
     */
static ColorMapType::Type COLORS[] =
{
    ColorMapType::SOLID_COLOR_RED,
    ColorMapType::SOLID_COLOR_GREEN,
    ColorMapType::SOLID_COLOR_BLUE,
    ColorMapType::SOLID_COLOR_YELLOW,
    ColorMapType::SOLID_COLOR_PURPLE,
    ColorMapType::SOLID_COLOR_CYAN,
};

//###############################################################
// Code for rmDir and cpDir taken from mosg's StackOverflow answer
// to this question:
// http://stackoverflow.com/questions/2536524/copy-directory-using-qt

// This function implementes the shell command 'rm -r'
static bool rmDir(const QString &dirPath)
{
  QDir dir(dirPath);
  if (!dir.exists())
    return true;
  foreach(const QFileInfo &info, dir.entryInfoList(QDir::Dirs | QDir::Files
                                                   | QDir::NoDotAndDotDot)) {
    if (info.isDir()) {
      if (!rmDir(info.filePath()))
        return false;
    } else {
      if (!dir.remove(info.fileName()))
        return false;
    }
  }
  QDir parentDir(QFileInfo(dirPath).path());
  return parentDir.rmdir(QFileInfo(dirPath).fileName());
}

// This function implements the shell command 'cp -r' except that it runs
// 'rm -r' on the destination folder first
static bool cpDir(const QString& srcPath, const QString &dstPath)
{
  rmDir(dstPath);
  QDir parentDstDir(QFileInfo(dstPath).path());
  if (!parentDstDir.mkpath(QFileInfo(dstPath).fileName()))
    return false;

  QDir srcDir(srcPath);
  foreach(const QFileInfo &info, srcDir.entryInfoList(QDir::Dirs | QDir::Files
                                                      | QDir::NoDotAndDotDot)) {
    QString srcItemPath = srcDir.absoluteFilePath(info.fileName());
    QString dstItemPath = dstPath + "/" + info.fileName();
    if (info.isDir()) {
      if (!cpDir(srcItemPath, dstItemPath)) {
        return false;
      }
    } else if (info.isFile()) {
      if (!QFile::copy(srcItemPath, dstItemPath)) {
        return false;
      }
    } else {
      qDebug() << "Unhandled item " << info.filePath() << " in cpDir";
    }
  }
  return true;
}
// end code taken from StackOverflow
//###############################################################

// helper function to compute position and orientation from vtkCamera
// on return position and orientation will be in the quatlib types passed in
static inline void getCameraPositionAndOrientation(vtkCamera *cam,
                                                   q_vec_type pos,
                                                   q_type orient)
{
    q_vec_type forward,up,right,focalPoint;

    // position is easy
    cam->GetPosition(pos);
    cam->GetFocalPoint(focalPoint);
    cam->GetViewUp(up);
    q_vec_subtract(forward,pos,focalPoint);
    q_vec_normalize(forward,forward);
    q_vec_cross_product(right,forward,up);

    vtkNew< vtkTransform > transform;
    vtkSmartPointer< vtkMatrix4x4 > mat =
            vtkSmartPointer< vtkMatrix4x4 >::New();
    mat->Identity();
    mat->SetElement(0,0,right[0]);
    mat->SetElement(1,0,right[1]);
    mat->SetElement(2,0,right[2]);
    mat->SetElement(0,1,up[0]);
    mat->SetElement(1,1,up[1]);
    mat->SetElement(2,1,up[2]);
    mat->SetElement(0,2,forward[0]);
    mat->SetElement(1,2,forward[1]);
    mat->SetElement(2,2,forward[2]);
    transform->SetMatrix(mat);

    q_vec_type junk;
    // finally get orientation... note that position could
    // also be computed, but the position is not added to the
    // matrix and thus the output should be the null vector
    SketchObject::getPositionAndOrientationFromTransform(
                transform.GetPointer(),junk,orient);
}

static inline void makeFloorAndLines(vtkPlaneSource*  shadowFloorSource,
                              vtkActor* shadowFloorActor,
                              vtkActor* floorLinesActor,
                              TransformManager* transforms)
{
// the half x-length of the plane in room units
#define PLANE_HALF_LENGTH 30
#define PLANE_Y PLANE_ROOM_Y
// the depth of the plane in z away from the camera
#define PLANE_DEPTH PLANE_HALF_LENGTH*3

// These have to do with how vtk defines a plane.  It is defined as
// an origin and two points and the result is a parallelogram that
// has those three as one corner (the origin) and the two nearest corners
// (the other points)
// the "origin" for the plane
#define PLANE_ORIGIN -PLANE_HALF_LENGTH,PLANE_Y,PLANE_HALF_LENGTH
// the first point to define the plane
#define PLANE_POINT1  PLANE_HALF_LENGTH,PLANE_Y,PLANE_HALF_LENGTH
// the second point to define the plane
#define PLANE_POINT2 -PLANE_HALF_LENGTH,PLANE_Y,-PLANE_DEPTH

// array names for the calculator/contour filters
#define X_ARRAY_NAME "X"
#define Z_ARRAY_NAME "Z"

// the number of lines across in X
#define NUM_LINES_IN_X 9

// The color of the lines over the plane (R, G, and B components all set by this)
#define LINES_COLOR 0.3,0.3,0.3
// The color of the plane itself (R, G, and B components all set by this)
#define PLANE_COLOR 0.45,0.45,0.45

    shadowFloorSource->SetOrigin(PLANE_ORIGIN);
    shadowFloorSource->SetPoint1(PLANE_POINT1);
    shadowFloorSource->SetPoint2(PLANE_POINT2);
    shadowFloorSource->SetResolution(1,1);
    shadowFloorSource->Update();
    // create contour lines
    vtkSmartPointer< vtkArrayCalculator > xCalc =
            vtkSmartPointer< vtkArrayCalculator >::New();
    xCalc->SetInputConnection(shadowFloorSource->GetOutputPort());
    xCalc->SetResultArrayName(X_ARRAY_NAME);
    xCalc->AddCoordinateScalarVariable(X_ARRAY_NAME,0);
    xCalc->SetFunction(X_ARRAY_NAME);
    xCalc->Update();
    vtkSmartPointer< vtkArrayCalculator > zCalc =
            vtkSmartPointer< vtkArrayCalculator >::New();
    zCalc->SetInputConnection(shadowFloorSource->GetOutputPort());
    zCalc->SetResultArrayName(Z_ARRAY_NAME);
    zCalc->AddCoordinateScalarVariable(Z_ARRAY_NAME,2);
    zCalc->SetFunction(Z_ARRAY_NAME);
    zCalc->Update();
    vtkSmartPointer< vtkContourFilter > xLines =
            vtkSmartPointer< vtkContourFilter >::New();
    xLines->SetInputConnection(xCalc->GetOutputPort());
    xLines->GenerateValues(NUM_LINES_IN_X,-PLANE_HALF_LENGTH+1,PLANE_HALF_LENGTH-1);
    xLines->Update();
    vtkSmartPointer< vtkContourFilter > zLines =
            vtkSmartPointer< vtkContourFilter >::New();
    zLines->SetInputConnection(zCalc->GetOutputPort());
    zLines->GenerateValues(NUM_LINES_IN_X*(PLANE_DEPTH/PLANE_HALF_LENGTH +1),
                           -PLANE_HALF_LENGTH+1,
                           PLANE_HALF_LENGTH+PLANE_DEPTH-1);
    zLines->Update();
    vtkSmartPointer< vtkAppendPolyData > appendPolyData =
            vtkSmartPointer< vtkAppendPolyData >::New();
    appendPolyData->AddInputConnection(xLines->GetOutputPort());
    appendPolyData->AddInputConnection(zLines->GetOutputPort());
    appendPolyData->Update();
    vtkSmartPointer< vtkTransformPolyDataFilter > transformPD =
            vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    transformPD->SetInputConnection(appendPolyData->GetOutputPort());
    vtkSmartPointer< vtkTransform > transform =
            vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    transform->Translate(0,LINE_FROM_PLANE_OFFSET,0);
    transformPD->SetTransform(transform);
    transformPD->Update();
    vtkSmartPointer< vtkPolyDataMapper > linesMapper =
            vtkSmartPointer< vtkPolyDataMapper >::New();
    linesMapper->SetInputConnection(transformPD->GetOutputPort());
    vtkSmartPointer< vtkColorTransferFunction > colors =
            vtkSmartPointer< vtkColorTransferFunction >::New();
    colors->SetColorSpaceToLab();
    colors->AddRGBPoint(0,LINES_COLOR);
    linesMapper->SetLookupTable(colors);
    linesMapper->Update();
    // set the actor for the floor lines
    floorLinesActor->SetMapper(linesMapper);
    floorLinesActor->GetProperty()->LightingOff();
    floorLinesActor->SetUserTransform(transforms->getRoomToWorldTransform());
    // connect shadow floor source to its actor
    vtkSmartPointer< vtkPolyDataMapper > floorMapper =
            vtkSmartPointer< vtkPolyDataMapper >::New();
    floorMapper->SetInputConnection(shadowFloorSource->GetOutputPort());
    floorMapper->Update();
    shadowFloorActor->SetMapper(floorMapper);
    // set up the actor and add it to the renderer
    shadowFloorActor->GetProperty()->SetColor(PLANE_COLOR);
    shadowFloorActor->GetProperty()->LightingOff();
    shadowFloorActor->SetUserTransform(transforms->getRoomToWorldTransform());

#undef Z_ARRAY_NAME
#undef X_ARRAY_NAME

#undef PLANE_POINT2
#undef PLANE_POINT1
#undef PLANE_ORIGIN
#undef PLANE_DEPTH
#undef PLANE_HALF_LENGTH
}

namespace SketchBio {

//########################################################################
//########################################################################
// ProjectImpl internal class
//########################################################################
//########################################################################

class Project::ProjectImpl : public WorldObserver {
public:
    // Constructor/Destructor
    ProjectImpl(vtkRenderer* r, const QString& projDir);
    ~ProjectImpl();
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
    const QList< StructureReplicator *> &getCrystalByExamples() const;
    int getNumberOfCrystalByExamples() const;
    // the list of transform operations (lock transforms for now)
    const QVector< QSharedPointer< TransformEquals > > &getTransformOps() const;
    int getNumberOfTransformOps() const;
    // the hash of camera objects and their corresponding vtkCamera objects
    const QHash< SketchObject*, vtkSmartPointer< vtkCamera > > &getCameras() const;
    // the project directory
    QString getProjectDir() const;
    // ##################################################################
    // Outline functions:
    // update the object used for the outlines
    void setOutlineObject(int outlineIdx, SketchObject* obj);
    // update the spring used for the outlines
    void setOutlineSpring(int outlineIdx, Connector* conn, bool end1Large);
    // show/hide the outlines of objects
    void setOutlineVisible(int outlineIdx, bool visible);
    // tell if the outlines for a particular side are visible
    bool isOutlineVisible(int outlineIdx);
    // set if outlining springs or objects
    void setOutlineType(OutlineType type);
    // ###################################################################
    // Frame update functions:
    // functions to update things every frame
    // update locations of tracker objects
    void updateTrackerPositions();
    // physics/time related functions - timestep either updates physics or moves the animation one time frame
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
    OperationState *getOperationState();
    void setOperationState(OperationState *state);
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
    // that fails will return the file as-is if it is outside the project directory
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
    // Given the vtk camera position, creates a camera object that has that position
    // and view
    SketchObject* addCameraObjectFromCameraPosition(vtkCamera* cam);
    void setCameraToVTKCameraPosition(SketchObject* cam, vtkCamera* vcam);
    // ###################################################################
    // Adding things functions
    // for structure replication chains
    StructureReplicator* addReplication(SketchObject* o1, SketchObject* o2, int numCopies);
    void addReplication(StructureReplicator* rep);
    // for transform equals
    QWeakPointer<TransformEquals> addTransformEquals(SketchObject* o1, SketchObject* o2);
public:
    // ###################################################################
    // overrides from WorldObserver to get camera added/removed events
    virtual void objectAdded(SketchObject *o);
    virtual void objectRemoved(SketchObject *o);
private:
    // ###################################################################
    // helper functions
    void updateOutlines(SketchBioHandId::Type side);
    // ###################################################################
    // Fields

    vtkSmartPointer< vtkRenderer > renderer;
    QTime time;
    // managers -- these are owned by the project, raw pointers may be passed to other places, but
    //              will be invalid once the project is deleted
    ModelManager models;
    TransformManager transforms;
    WorldManager world;

    // operators on objects -- these are owned here.
    QList< StructureReplicator* > replicas;
    QHash< SketchObject* , vtkSmartPointer< vtkCamera > > cameras;
    QVector< QSharedPointer< TransformEquals > > transformOps;

    // project dir
    QString projectDirName;

    // undo states:
    QList< UndoState* > undoStack, redoStack;

    // other ui stuff
    // the objects that represent the user's hands
    Hand hand[2];
    // outline actors are added to the renderer when the object is close enough to
    // interact with.  The outline mappers are updated whenever the closest object
    // changes (unless another one is currently grabbed). Note: left is 0, right is 1
    QPair< vtkSmartPointer< vtkPolyDataMapper >,
        vtkSmartPointer< vtkActor > > outlines[2];

    vtkSmartPointer< vtkPlaneSource > shadowFloorSource;
    vtkSmartPointer< vtkActor > shadowFloorActor, floorLinesActor;
    // animation stuff
    bool isDoingAnimation; // true if the animation is happenning
    bool showingShadows; // true if shadows are currently being shown
    double timeInAnimation; // the animation time starting at 0
    Project::OutlineType outlineType;
    double viewTime;
    // State from user operations that require persistent state
    OperationState *opState;
};

//########################################################################
// Constructor/Destructor
static const float OUTLINES_COLOR_VAL = 0.7;
Project::ProjectImpl::ProjectImpl(vtkRenderer* r, const QString& projDir) :
    renderer(r),
    time(),
    models(),
    transforms(),
    world(r),
    replicas(),
    cameras(),
    transformOps(),
    projectDirName(projDir),
    undoStack(),
    redoStack(),
    outlines(),
    shadowFloorSource(vtkSmartPointer< vtkPlaneSource >::New()),
    shadowFloorActor(vtkSmartPointer< vtkActor >::New()),
    floorLinesActor(vtkSmartPointer< vtkActor >::New()),
    isDoingAnimation(false),
    showingShadows(true),
    timeInAnimation(0.0),
    outlineType(OUTLINE_OBJECTS),
    viewTime(0.0),
    opState(NULL)
{
    world.addObserver(this);
    hand[SketchBioHandId::LEFT].init(&transforms,&world,SketchBioHandId::LEFT);
    hand[SketchBioHandId::RIGHT].init(&transforms,&world,SketchBioHandId::RIGHT);
    renderer->AddActor(hand[SketchBioHandId::LEFT].getHandActor());
    renderer->AddActor(hand[SketchBioHandId::RIGHT].getHandActor());
    QDir dir(projectDirName);
    if (!dir.exists())
    {
        QDir::current().mkpath(projectDirName);
    }
    // set up initial transforms
    transforms.scaleWorldRelativeToRoom(SCALE_DOWN_FACTOR);
    // set up initial camera
    renderer->SetActiveCamera(transforms.getGlobalCamera());
    // connect outlines mapper & actor
    for (int i = 0; i < 2; i++)
    {
        outlines[i].first = vtkSmartPointer< vtkPolyDataMapper >::New();
        outlines[i].second = vtkSmartPointer< vtkActor >::New();
        outlines[i].second->SetMapper(outlines[i].first);
        outlines[i].second->GetProperty()->SetLighting(false);
        outlines[i].second->GetProperty()->SetColor(OUTLINES_COLOR_VAL,
                                             OUTLINES_COLOR_VAL,
                                             OUTLINES_COLOR_VAL);
    }
    // make the floor and lines
    makeFloorAndLines(shadowFloorSource,shadowFloorActor,floorLinesActor,
                      &transforms);
    // add the floor and lines
    renderer->AddActor(floorLinesActor);
    renderer->AddActor(shadowFloorActor);
    renderer->AddActor(hand[SketchBioHandId::LEFT].getShadowActor());
    renderer->AddActor(hand[SketchBioHandId::RIGHT].getShadowActor());
}

Project::ProjectImpl::~ProjectImpl()
{
    qDeleteAll(redoStack);
    redoStack.clear();
    qDeleteAll(undoStack);
    undoStack.clear();
    cameras.clear();
    // these destructors call methods on the objects... make sure they are called
    // before the objects are deleted.
    transformOps.clear();
    qDeleteAll(replicas);
    replicas.clear();
}
//########################################################################
// Accessor fuctions
ModelManager &Project::ProjectImpl::getModelManager()
{
    return models;
}
const ModelManager &Project::ProjectImpl::getModelManager() const
{
    return models;
}
WorldManager &Project::ProjectImpl::getWorldManager()
{
    return world;
}
const WorldManager &Project::ProjectImpl::getWorldManager() const
{
    return world;
}
TransformManager &Project::ProjectImpl::getTransformManager()
{
    return transforms;
}
const TransformManager &Project::ProjectImpl::getTransformManager() const
{
    return transforms;
}
Hand &Project::ProjectImpl::getHand(SketchBioHandId::Type side)
{
    return hand[side];
}
const QList< StructureReplicator* > &
        Project::ProjectImpl::getCrystalByExamples() const
{
    return replicas;
}
int Project::ProjectImpl::getNumberOfCrystalByExamples() const
{
    return replicas.size();
}
const QVector< QSharedPointer< TransformEquals > > &
        Project::ProjectImpl::getTransformOps() const
{
    return transformOps;
}
int Project::ProjectImpl::getNumberOfTransformOps() const
{
    return transformOps.size();
}
const QHash< SketchObject*, vtkSmartPointer< vtkCamera > > &
        Project::ProjectImpl::getCameras() const
{
    return cameras;
}
QString Project::ProjectImpl::getProjectDir() const {
    return projectDirName;
}
//########################################################################
// Outline functions
void Project::ProjectImpl::setOutlineObject(int outlineIdx, SketchObject* obj)
{
    vtkPolyDataMapper* mapper = outlines[outlineIdx].first;
    vtkActor* actor = outlines[outlineIdx].second;
    mapper->SetInputConnection(obj->getOrientedBoundingBoxes()->GetOutputPort());
    mapper->Update();
    // TODO - fix this
    if (obj->getParent() != NULL)
    {
        actor->GetProperty()->SetColor(1,0,0);
    }
    else
    {
        actor->GetProperty()->SetColor(OUTLINES_COLOR_VAL,OUTLINES_COLOR_VAL,
                                       OUTLINES_COLOR_VAL);
    }
}

void Project::ProjectImpl::setOutlineSpring(int outlineIdx, Connector* conn, bool end1Large)
{
    vtkPolyDataMapper* mapper = outlines[outlineIdx].first;
    vtkActor* actor = outlines[outlineIdx].second;
    q_vec_type end1, end2, midpoint, direction;
    conn->getEnd1WorldPosition(end1);
    conn->getEnd2WorldPosition(end2);
    q_vec_add(midpoint,end1,end2);
    q_vec_scale(midpoint, .5, midpoint);
    q_vec_subtract(direction,end2,end1);
    double length = q_vec_magnitude(direction);
    if (!end1Large) {
        q_vec_invert(direction,direction);
    }
    vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
    cone->SetCenter(midpoint);
    cone->SetDirection(direction);
    cone->SetHeight(length);
    cone->SetRadius(50);
    cone->SetResolution(4);
    cone->Update();

    vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
    edges->SetInputConnection(cone->GetOutputPort());
    edges->Update();

    mapper->SetInputConnection(edges->GetOutputPort());
    mapper->Update();

    actor->GetProperty()->SetColor(OUTLINES_COLOR_VAL,OUTLINES_COLOR_VAL,
                                   OUTLINES_COLOR_VAL);
}

void Project::ProjectImpl::setOutlineVisible(int outlineIdx, bool visible)
{
    vtkActor* actor = outlines[outlineIdx].second;
    if (visible)
    {
        renderer->AddActor(actor);
    }
    else
    {
        renderer->RemoveActor(actor);
    }
}

bool Project::ProjectImpl::isOutlineVisible(int outlineIdx)
{
    return renderer->HasViewProp(outlines[outlineIdx].second);
}

void Project::ProjectImpl::setOutlineType(OutlineType type)
{
  outlineType = type;
  updateOutlines(SketchBioHandId::LEFT);
  updateOutlines(SketchBioHandId::RIGHT);
}

void Project::ProjectImpl::updateOutlines(SketchBioHandId::Type side)
{
  bool outlinesShowing = isOutlineVisible(side);
  bool shouldOutlinesShow;
  double dist;
  switch(outlineType) {
  case OUTLINE_OBJECTS:
    dist = hand[side].getNearestObjectDistance();
    shouldOutlinesShow = dist < DISTANCE_THRESHOLD;
    break;
  case OUTLINE_CONNECTORS:
    dist = hand[side].getNearestConnectorDistance();
    shouldOutlinesShow = dist < SPRING_DISTANCE_THRESHOLD;
    break;
  }
  // don't show outlines during animation
  if (isShowingAnimation()) {
    shouldOutlinesShow = false;
  }
  if (shouldOutlinesShow != outlinesShowing) {
    setOutlineVisible(side,shouldOutlinesShow);
  }
}
//########################################################################
// Frame update functions
void Project::ProjectImpl::updateTrackerPositions()
{
  for (int i = 0; i < 2; ++i) {
    SketchObject *oldNearest = hand[i].getNearestObject();
    bool oldAtEnd1;
    Connector *oldConnector = hand[i].getNearestConnector(&oldAtEnd1);
    hand[i].updatePositionAndOrientation();
    hand[i].updateGrabbed();
    hand[i].computeNearestObjectAndConnector();
    SketchObject *nearest = hand[i].getNearestObject();
    bool atEnd1;
    Connector *connector = hand[i].getNearestConnector(&atEnd1);
    if (oldNearest != nearest && outlineType == OUTLINE_OBJECTS) {
      setOutlineObject(i,nearest);
    }
    if (((oldConnector != connector) || (oldAtEnd1 != atEnd1)) &&
        outlineType == OUTLINE_CONNECTORS) {
      setOutlineSpring(i,connector,atEnd1);
    }
    updateOutlines(i ? SketchBioHandId::LEFT : SketchBioHandId::RIGHT);
  }
}
void Project::ProjectImpl::timestep(double dt) {
    if (!isDoingAnimation) {
        //handleInput();
        world.stepPhysics(dt);
        q_vec_type point = { 0, PLANE_Y, 0}, vector = {0, 1, 0};
        vtkLinearTransform *rTW = transforms.getRoomToWorldTransform();
        rTW->TransformPoint(point,point);
        rTW->TransformVector(vector,vector);
        q_vec_normalize(vector,vector);
        q_vec_add(point,vector,point);
        world.setShadowPlane(point,vector);
        transforms.copyCurrentHandTransformsToOld();
    } else {
        double elapsed_time = time.restart();
        if (world.setAnimationTime(timeInAnimation)) {
            // if setAnimationTime returns true, then we are done
            stopAnimation();
        } else {
            for (QHashIterator< SketchObject* , vtkSmartPointer< vtkCamera > > it(cameras);
                 it.hasNext(); ) {
                SketchObject* cam = it.peekNext().key();
                vtkSmartPointer< vtkCamera > vCam = it.next().value();
                if (cam->isActive()) {
                    setUpVtkCamera(cam,vCam);
                    renderer->SetActiveCamera(vCam);
                }
            }
        }
        timeInAnimation += (elapsed_time/1000);
    }
}
//########################################################################
// Animation and time functions
void Project::ProjectImpl::startAnimation()
{
    isDoingAnimation = true;
    timeInAnimation = 0.0;
    renderer->RemoveActor(hand[SketchBioHandId::LEFT].getHandActor());
    renderer->RemoveActor(hand[SketchBioHandId::RIGHT].getHandActor());
    for (int i = 0; i < 2; i++)
    {
        if (renderer->HasViewProp(outlines[i].second))
        {
            renderer->RemoveViewProp(outlines[i].second);
        }
    }
    hand[SketchBioHandId::LEFT].clearState();
    hand[SketchBioHandId::RIGHT].clearState();
    world.hideInvisibleObjects();
    setShowShadows(false);

    QListIterator<SketchObject *> it = world.getObjectIterator();
    while(it.hasNext()) {
        SketchObject* obj = it.next();
        obj->computeSplines();
    }
    time.start();
}
void Project::ProjectImpl::stopAnimation()
{
    isDoingAnimation = false;
    renderer->AddActor(hand[SketchBioHandId::LEFT].getHandActor());
    renderer->AddActor(hand[SketchBioHandId::RIGHT].getHandActor());
    // distances and outlines actors will refresh themselves
    // handed springs and world grab should refresh themselves too
    world.showInvisibleObjects();
    renderer->SetActiveCamera(transforms.getGlobalCamera());
    world.setAnimationTime(viewTime);
    setShowShadows(true);
}
bool Project::ProjectImpl::isShowingAnimation()
{
    return isDoingAnimation;
}
bool Project::ProjectImpl::goToAnimationTime(double time)
{
    if (!isDoingAnimation) {
        startAnimation();
    }

    timeInAnimation = time;
    return world.setAnimationTime(time);
}
double Project::ProjectImpl::getViewTime() const
{
    return viewTime;
}

void Project::ProjectImpl::setViewTime(double time)
{
    viewTime = time;
    if (!isDoingAnimation)
        world.setAnimationTime(time);
}
//########################################################################
// Undo and redo functions
UndoState* Project::ProjectImpl::getLastUndoState()
{
    if (undoStack.empty())
    {
        return NULL;
    }
    else
    {
        return undoStack.back();
    }
}
UndoState* Project::ProjectImpl::getNextRedoState()
{
    if (redoStack.empty())
    {
        return NULL;
    }
    else
    {
        return redoStack.back();
    }
}
void Project::ProjectImpl::popUndoState()
{
    UndoState* state = undoStack.back();
    undoStack.pop_back();
    delete state;
}
void Project::ProjectImpl::addUndoState(UndoState* state)
{
    if (state == NULL || state->getProject().impl != this)
        return;
    undoStack.push_back(state);
    qDeleteAll(redoStack);
    redoStack.clear();
}
void Project::ProjectImpl::applyUndo()
{
    if (undoStack.empty())
        return;
    UndoState* state = undoStack.back();
    undoStack.pop_back();
    state->undo();
    redoStack.push_back(state);
}
void Project::ProjectImpl::applyRedo()
{
    if (redoStack.empty())
        return;
    UndoState* state = redoStack.back();
    redoStack.pop_back();
    state->redo();
    undoStack.push_back(state);
}
//########################################################################
// User operation state functions
OperationState *Project::ProjectImpl::getOperationState()
{
    return opState;
}
void Project::ProjectImpl::setOperationState(OperationState *newState)
{
    opState = newState;
}
//########################################################################
void Project::ProjectImpl::clearProject()
{
    if (isDoingAnimation)
    {
        stopAnimation();
    }
    transformOps.clear();
    cameras.clear();
    qDeleteAll(replicas);
    replicas.clear();
    world.clearObjects();
    world.clearConnectors();
    setOutlineVisible(SketchBioHandId::LEFT,false);
    setOutlineVisible(SketchBioHandId::RIGHT,false);
    hand[SketchBioHandId::LEFT].clearState();
    hand[SketchBioHandId::RIGHT].clearState();
}
//########################################################################
// Project directory functions
bool Project::ProjectImpl::setProjectDir(const QString &dir)
{
if (dir == projectDirName)
  return true;
  QDir tmp = QDir(dir);
  if (tmp.isRelative())
  {
    QString abs = QDir::current().absoluteFilePath(dir);
    if (abs == projectDirName)
      return true;
  }
  if (!cpDir(projectDirName,dir))
    return false;
  projectDirName = dir;
  return true;
}
bool Project::ProjectImpl::getFileInProjDir(const QString& filename, QString& newName)
{
    QDir projectDir(projectDirName);
    QString localname = filename.mid(filename.lastIndexOf("/") +1);
    QString fullpath = projectDir.absoluteFilePath(localname);
    QFile file(fullpath);
    if ( file.exists() || file.copy(filename,fullpath))
    {
      newName = fullpath;
      return true;
    } else {
      newName = filename;
      return false;
    }
}
//########################################################################
// Shadow functions
bool Project::ProjectImpl::isShowingShadows() const
{
    return showingShadows;
}
void Project::ProjectImpl::setShowShadows(bool show)
{
    if (show == showingShadows)
        return;
    showingShadows = show;
    if (show)
    {
        renderer->AddActor(shadowFloorActor);
        renderer->AddActor(floorLinesActor);
        renderer->AddActor(hand[SketchBioHandId::LEFT].getShadowActor());
        renderer->AddActor(hand[SketchBioHandId::RIGHT].getShadowActor());
    }
    else
    {
        renderer->RemoveActor(shadowFloorActor);
        renderer->RemoveActor(floorLinesActor);
        renderer->RemoveActor(hand[SketchBioHandId::LEFT].getShadowActor());
        renderer->RemoveActor(hand[SketchBioHandId::RIGHT].getShadowActor());
    }
    world.setShowShadows(show);
}
//########################################################################
// Camera functions

SketchModel* Project::ProjectImpl::getCameraModel() {
  QDir projectDir(projectDirName);
  return models.getCameraModel(projectDir);
}

SketchObject* Project::ProjectImpl::addCamera(const q_vec_type pos, const q_type orient)
{
    QDir projectDir(projectDirName);
    SketchModel* model = models.getCameraModel(projectDir);
    SketchObject* obj = world.addObject(model,pos,orient);
    // cameras are invisible (from the animation's standpoint)
    // this is now taken care of by the callback from world's addObject observer
    // adding to the cameras set is also done in that callback
//    obj->setIsVisible(false);
    // if this is the first camera added, make it active
    if (cameras.size() == 1 && cameras.contains(obj)) {
        obj->setActive(true);
    }
    return obj;
}
SketchObject* Project::ProjectImpl::addCameraObjectFromCameraPosition(vtkCamera* cam)
{
    q_vec_type pos;
    q_type orient;

    getCameraPositionAndOrientation(cam,pos,orient);

    SketchObject* obj = addCamera(pos,orient);
    return obj;
}
void Project::ProjectImpl::setCameraToVTKCameraPosition(SketchObject* cam, vtkCamera* vcam)
{
    q_vec_type pos;
    q_type orient;

    getCameraPositionAndOrientation(vcam,pos,orient);

    cam->setPosAndOrient(pos,orient);
}
//########################################################################
// Adding things functions
StructureReplicator* Project::ProjectImpl::addReplication(SketchObject* o1, SketchObject* o2,
                                                   int numCopies)
{
    StructureReplicator* rep = new StructureReplicator(o1,o2,&world);
    replicas.append(rep);
    rep->setNumShown(numCopies);
    return rep;
}
void Project::ProjectImpl::addReplication(StructureReplicator* rep)
{
    replicas.append(rep);
}
QWeakPointer<TransformEquals> Project::ProjectImpl::addTransformEquals(
        SketchObject* o1, SketchObject* o2)
{
    QSharedPointer<TransformEquals> trans(new TransformEquals(o1,o2,&world));
    transformOps.append(trans);
    return trans.toWeakRef();
}
//########################################################################
// World observer functions
void Project::ProjectImpl::objectAdded(SketchObject *o)
{
    if (o->numInstances() == 1) {
        if (o->getModel() == getCameraModel()) {
            cameras.insert(o,vtkSmartPointer<vtkCamera>::New());
            // cameras are not visible! make sure they are not.
            if (o->isVisible()) {
                o->setIsVisible(false);
            }
        }
    } else {
        foreach (SketchObject *child, *o->getSubObjects()) {
            objectAdded(child);
        }
    }
}

void Project::ProjectImpl::objectRemoved(SketchObject *o)
{
    if (o->numInstances() == 1) {
        if (o->getModel() == getCameraModel()) {
            cameras.remove(o);
        }
    } else {
        foreach (SketchObject *child, *o->getSubObjects()) {
            objectRemoved(child);
        }
    }
}


//########################################################################
//########################################################################
// Project class - forward to ProjectImpl
//########################################################################
//########################################################################
// most of these definitions are pretty boring, they just forward to the
// implementation class
Project::Project(vtkRenderer *r, const QString &projDir)
    : impl(new ProjectImpl(r,projDir)) {}

Project::~Project() {
    delete impl;
}
//########################################################################
// Accessor functions
ModelManager &Project::getModelManager() { return impl->getModelManager(); }
const ModelManager &Project::getModelManager() const
{
    return impl->getModelManager();
}
WorldManager &Project::getWorldManager() { return impl->getWorldManager(); }
const WorldManager &Project::getWorldManager() const
{
    return impl->getWorldManager();
}
TransformManager &Project::getTransformManager()
{
    return impl->getTransformManager();
}
const TransformManager &Project::getTransformManager() const
{
    return impl->getTransformManager();
}
Hand &Project::getHand(SketchBioHandId::Type side)
{
    return impl->getHand(side);
}
const QList< StructureReplicator* > &Project::getCrystalByExamples() const
{
    return impl->getCrystalByExamples();
}
int Project::getNumberOfCrystalByExamples() const
{
    return impl->getNumberOfCrystalByExamples();
}
const QVector< QSharedPointer< TransformEquals > > &Project::getTransformOps() const
{
    return impl->getTransformOps();
}
int Project::getNumberOfTransformOps() const
{
    return impl->getNumberOfTransformOps();
}
const QHash< SketchObject*, vtkSmartPointer< vtkCamera > > &Project::getCameras() const
{
    return impl->getCameras();
}
QString Project::getProjectDir() const { return impl->getProjectDir(); }
//########################################################################
// Outline functions
void Project::setOutlineObject(SketchBioHandId::Type side, SketchObject *obj)
{
    impl->setOutlineObject(side, obj);
}
void Project::setOutlineSpring(SketchBioHandId::Type side, Connector *conn, bool end1Large)
{
    impl->setOutlineSpring(side,conn,end1Large);
}
void Project::setOutlineVisible(SketchBioHandId::Type side, bool visible)
{
    impl->setOutlineVisible(side,visible);
}
bool Project::isOutlineVisible(SketchBioHandId::Type side)
{
    return impl->isOutlineVisible(side);
}
void Project::setOutlineType(OutlineType type)
{
    impl->setOutlineType(type);
}
//########################################################################
// Frame update functions
void Project::updateTrackerPositions() { impl->updateTrackerPositions(); }
void Project::timestep(double dt) { impl->timestep(dt); }
//########################################################################
// Animation and time functions
void Project::startAnimation() { impl->startAnimation(); }
void Project::stopAnimation() { impl->stopAnimation(); }
bool Project::isShowingAnimation() { return impl->isShowingAnimation(); }
bool Project::goToAnimationTime(double time)
{
    return impl->goToAnimationTime(time);
}
double Project::getViewTime() const { return impl->getViewTime(); }
void Project::setViewTime(double time) { impl->setViewTime(time); }
//########################################################################
// Undo and Redo functions
UndoState *Project::getLastUndoState()
{
    return impl->getLastUndoState();
}
UndoState *Project::getNextRedoState()
{
    return impl->getNextRedoState();
}
void Project::popUndoState() { impl->popUndoState(); }
void Project::addUndoState(UndoState *state) { impl->addUndoState(state); }
void Project::applyUndo() { impl->applyUndo(); }
void Project::applyRedo() { impl->applyRedo(); }
//########################################################################
// User operation state functions
OperationState *Project::getOperationState()
{
    return impl->getOperationState();
}
void Project::setOperationState(OperationState *state)
{
    impl->setOperationState(state);
}
//########################################################################
void Project::clearProject() { impl->clearProject(); }
//########################################################################
// Project directory functions
bool Project::setProjectDir(const QString &dir)
{
    return impl->setProjectDir(dir);
}
bool Project::getFileInProjDir(const QString &filename, QString &newName)
{
    return impl->getFileInProjDir(filename,newName);
}
//########################################################################
// Shadow functions
bool Project::isShowingShadows() const { return impl->isShowingShadows(); }
void Project::setShowShadows(bool show) { impl->setShowShadows(show); }
void Project::setShadowsOn() { impl->setShowShadows(true); }
void Project::setShadowsOff() { impl->setShowShadows(false); }
//########################################################################
// Camera functions
SketchModel *Project::getCameraModel()
{
    return impl->getCameraModel();
}
SketchObject *Project::addCamera(const q_vec_type pos, const q_type orient)
{
    return impl->addCamera(pos,orient);
}
SketchObject *Project::addCameraObjectFromCameraPosition(vtkCamera *cam)
{
    return impl->addCameraObjectFromCameraPosition(cam);
}
void Project::setCameraToVTKCameraPosition(SketchObject *cam, vtkCamera *vcam)
{
    impl->setCameraToVTKCameraPosition(cam,vcam);
}
//########################################################################
// Adding things functions
StructureReplicator *Project::addReplication(SketchObject *o1,
                                             SketchObject *o2, int numCopies)
{
    return impl->addReplication(o1,o2,numCopies);
}
void Project::addReplication(StructureReplicator *rep)
{
    impl->addReplication(rep);
}
QWeakPointer< TransformEquals > Project::addTransformEquals(SketchObject *o1,
                                                            SketchObject *o2)
{
    return impl->addTransformEquals(o1,o2);
}
//########################################################################
// Static functions
void Project::setUpVtkCamera(SketchObject *cam, vtkCamera *vCam)
{
    vtkSmartPointer<vtkTransform> trans = cam->getLocalTransform();
    // not actually the inverse... I guess I goofed up... but it works, so leaving it for now.
    vtkSmartPointer<vtkMatrix4x4> invMat = trans->GetMatrix();
    // note that the object's localTransform should not be scaled
    q_vec_type up, forward, pos, fPoint;
    // the first column is the 'right' vector ... unused
    // the second column of that matrix is the up vector
    up[0] = invMat->GetElement(0,1);
    up[1] = invMat->GetElement(1,1);
    up[2] = invMat->GetElement(2,1);
    // the third column is the front vector
    forward[0] = invMat->GetElement(0,2);
    forward[1] = invMat->GetElement(1,2);
    forward[2] = invMat->GetElement(2,2);
    q_vec_normalize(forward,forward);
    // the fourth column of the matrix is the position of the camera as a point
    pos[0] = invMat->GetElement(0,3);
    pos[1] = invMat->GetElement(1,3);
    pos[2] = invMat->GetElement(2,3);
    // compute the focal point from the position and the front vector
    q_vec_scale(fPoint,40,forward);
    q_vec_add(fPoint,fPoint,pos);
    // set camera parameters
    vCam->SetPosition(pos);
    vCam->SetFocalPoint(fPoint);
    vCam->SetViewUp(up);
    vCam->SetClippingRange(20,5000);
}

}
//void SketchProject::setViewpoint(vtkMatrix4x4* worldToRoom, vtkMatrix4x4* roomToEyes)
//{
//    transforms->setWorldToRoomMatrix(worldToRoom);
//    transforms->setRoomToEyeMatrix(roomToEyes);
//}

//void SketchProject::setCollisionMode(PhysicsMode::Type mode)
//{
//    world->setCollisionMode(mode);
//}

//void SketchProject::setCollisionTestsOn(bool on)
//{
//    world->setCollisionCheckOn(on);
//}

//void SketchProject::setWorldSpringsEnabled(bool enabled)
//{
//    world->setPhysicsSpringsOn(enabled);
//}

//SketchModel* SketchProject::addModel(SketchModel* model)
//{
//    model = models->addModel(model);
//    return model;
//}

//SketchModel* SketchProject::addModelFromFile(const QString& source, const QString& fileName,
//                                             double iMass, double iMoment)
//{
//    QString newFileName;
//    if (getFileInProjDir(fileName,newFileName))
//    {
//      return models->makeModel(source,newFileName,iMass,iMoment);
//    } else {
//      // Can't throw, called from Qt slot
//      qDebug() << "Failed to copy file to project directory: " << fileName;
//      return NULL;
//    }
//}

//SketchObject* SketchProject::addObject(SketchModel* model, const q_vec_type pos,
//                                       const q_type orient)
//{
//    int myIdx = world->getNumberOfObjects();
//    SketchObject* object = world->addObject(model,pos,orient);
//    //object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
//    if (object->numInstances() == 1)
//    {
//        object->setColorMapType(COLORS[myIdx%NUM_COLORS]);
//    }
//    return object;
//}

//SketchObject* SketchProject::addObject(const QString& source,const QString& filename)
//{
//    SketchModel* model = NULL;
//    model = models->getModel(source);
//    if (model == NULL)
//    {
//        model = addModelFromFile(source,filename,DEFAULT_INVERSE_MASS,
//                                 DEFAULT_INVERSE_MOMENT);
//        if (model == NULL)
//            return NULL;
//    }

//    q_vec_type pos = Q_NULL_VECTOR;
//    q_type orient = Q_ID_QUAT;
//    pos[Q_Y] = 2 * world->getNumberOfObjects() / transforms->getWorldToRoomScale();
//    return addObject(model,pos,orient);
//}

//SketchObject* SketchProject::addObject(SketchObject* object) {
//    int myIdx = world->getNumberOfObjects();
//    if (object->numInstances() == 1) {
//    //    object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
//        object->setColorMapType(COLORS[myIdx%NUM_COLORS]);
//    }
//    world->addObject(object);
//    // this is for when the object is read in from a file, this method is called
//    // with the objects instead of addCamera.  So this needs to recognize cameras
//    QDir projectDir(projectDirName);
//    if (object->getModel() == models->getCameraModel(projectDir)) {
//        cameras.insert(object,vtkSmartPointer<vtkCamera>::New());
//        // cameras are not visible! make sure they are not.
//        if (object->isVisible()) {
//            object->setIsVisible(false);
//        }
//    }
//    return object;
//}

//bool SketchProject::addObjects(const QVector<QString>& filenames)
//{

//    int i;
//    for (i = 0; i < filenames.size(); i++) {
//      // XXX Check for error here and return false if one found.
//        // notes: no good cross-platform way to check if file exists
//        // VTK prints out errors but does not throw anything, so we must
//        // do error checking before calling this
//        addObject(filenames[i],filenames[i]);
//    }

//    return true;
//}
//Connector* SketchProject::addSpring(SketchObject* o1, SketchObject* o2, double minRest, double maxRest,
//                                  double stiffness, q_vec_type o1Pos, q_vec_type o2Pos)
//{
//    return world->addSpring(o1,o2,o1Pos,o2Pos,false,stiffness,minRest,maxRest);
//}

//Connector* SketchProject::addConnector(Connector* spring)
//{
//    return world->addConnector(spring);
//}
