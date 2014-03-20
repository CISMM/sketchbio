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

inline void makeFloorAndLines(vtkPlaneSource*  shadowFloorSource,
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

#define OUTLINES_COLOR 0.7,0.7,0.7
SketchProject::SketchProject(vtkRenderer* r, const QString& projDir) :
    renderer(r),
	time(),
    models(new ModelManager()),
    transforms(new TransformManager()),
    world(new WorldManager(r)),
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
    outlineType(SketchProject::OUTLINE_OBJECTS),
    viewTime(0.0)
{
    hand[SketchBioHandId::LEFT].init(transforms.data(),world.data(),SketchBioHandId::LEFT);
    hand[SketchBioHandId::RIGHT].init(transforms.data(),world.data(),SketchBioHandId::RIGHT);
    renderer->AddActor(hand[SketchBioHandId::LEFT].getHandActor());
    renderer->AddActor(hand[SketchBioHandId::RIGHT].getHandActor());
    QDir dir(projectDirName);
    if (!dir.exists())
    {
        QDir::current().mkpath(projectDirName);
    }
    // set up initial transforms
    transforms->scaleWorldRelativeToRoom(SCALE_DOWN_FACTOR);
    // set up initial camera
    renderer->SetActiveCamera(transforms->getGlobalCamera());
    // connect outlines mapper & actor
    for (int i = 0; i < 2; i++)
    {
        QPair< vtkSmartPointer< vtkPolyDataMapper >, vtkSmartPointer< vtkActor > >
                pair(vtkSmartPointer< vtkPolyDataMapper >::New(),
                     vtkSmartPointer< vtkActor >::New());
        pair.second->SetMapper(pair.first);
        pair.second->GetProperty()->SetLighting(false);
        pair.second->GetProperty()->SetColor(OUTLINES_COLOR);
        outlines.append(pair);
    }

    // make the floor and lines
    makeFloorAndLines(shadowFloorSource,shadowFloorActor,floorLinesActor,
                      transforms.data());
    // add the floor and lines
    renderer->AddActor(floorLinesActor);
    renderer->AddActor(shadowFloorActor);
    renderer->AddActor(hand[SketchBioHandId::LEFT].getShadowActor());
    renderer->AddActor(hand[SketchBioHandId::RIGHT].getShadowActor());
}

SketchProject::~SketchProject()
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

bool SketchProject::setProjectDir(const QString &dir)
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

void SketchProject::setViewpoint(vtkMatrix4x4* worldToRoom, vtkMatrix4x4* roomToEyes)
{
    transforms->setWorldToRoomMatrix(worldToRoom);
    transforms->setRoomToEyeMatrix(roomToEyes);
}

void SketchProject::setCollisionMode(PhysicsMode::Type mode)
{
    world->setCollisionMode(mode);
}

void SketchProject::setCollisionTestsOn(bool on)
{
    world->setCollisionCheckOn(on);
}

void SketchProject::setWorldSpringsEnabled(bool enabled)
{
    world->setPhysicsSpringsOn(enabled);
}

QString SketchProject::getProjectDir() const
{
  return projectDirName;
}

bool SketchProject::getFileInProjDir(const QString& filename, QString& newName)
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

void SketchProject::clearProject()
{
    if (isDoingAnimation)
    {
        stopAnimation();
    }
    transformOps.clear();
    cameras.clear();
    qDeleteAll(replicas);
    replicas.clear();
    world->clearObjects();
    world->clearConnectors();
    setOutlineVisible(LEFT_SIDE_OUTLINE,false);
    setOutlineVisible(RIGHT_SIDE_OUTLINE,false);
    hand[SketchBioHandId::LEFT].clearState();
    hand[SketchBioHandId::RIGHT].clearState();
}

UndoState* SketchProject::getLastUndoState()
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

UndoState* SketchProject::getNextRedoState()
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

void SketchProject::popUndoState()
{
    UndoState* state = undoStack.back();
    undoStack.pop_back();
    delete state;
}

void SketchProject::addUndoState(UndoState* state)
{
    if (state == NULL || &state->getProject() != this)
        return;
    undoStack.push_back(state);
    qDeleteAll(redoStack);
    redoStack.clear();
}

void SketchProject::applyUndo()
{
    if (undoStack.empty())
        return;
    UndoState* state = undoStack.back();
    undoStack.pop_back();
    state->undo();
    redoStack.push_back(state);
}

void SketchProject::applyRedo()
{
    if (redoStack.empty())
        return;
    UndoState* state = redoStack.back();
    redoStack.pop_back();
    state->redo();
    undoStack.push_back(state);
}

void SketchProject::startAnimation()
{
    isDoingAnimation = true;
    timeInAnimation = 0.0;
    renderer->RemoveActor(hand[SketchBioHandId::LEFT].getHandActor());
    renderer->RemoveActor(hand[SketchBioHandId::RIGHT].getHandActor());
    for (int i = 0; i < outlines.size(); i++)
    {
        if (renderer->HasViewProp(outlines[i].second))
        {
            renderer->RemoveViewProp(outlines[i].second);
        }
    }
    hand[SketchBioHandId::LEFT].clearState();
    hand[SketchBioHandId::RIGHT].clearState();
    world->hideInvisibleObjects();
    setShowShadows(false);

	QListIterator<SketchObject *> it = world->getObjectIterator();
	while(it.hasNext()) {
		SketchObject* obj = it.next();
		obj->computeSplines();
	}
	time.start();
}

void SketchProject::stopAnimation()
{
    isDoingAnimation = false;
    renderer->AddActor(hand[SketchBioHandId::LEFT].getHandActor());
    renderer->AddActor(hand[SketchBioHandId::RIGHT].getHandActor());
    // distances and outlines actors will refresh themselves
    // handed springs and world grab should refresh themselves too
    world->showInvisibleObjects();
    renderer->SetActiveCamera(transforms->getGlobalCamera());
    world->setAnimationTime(viewTime);
    setShowShadows(true);
}

bool SketchProject::isShowingAnimation()
{
    return isDoingAnimation;
}

bool SketchProject::goToAnimationTime(double time)
{
	if (!isDoingAnimation) {
        startAnimation();
    }

    timeInAnimation = time;
    return world->setAnimationTime(time);
}

bool SketchProject::isShowingShadows() const
{
    return showingShadows;
}

void SketchProject::setShowShadows(bool show)
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
    world->setShowShadows(show);
}

void SketchProject::setShadowsOn()
{
    setShowShadows(true);
}

void SketchProject::setShadowsOff()
{
    setShowShadows(false);
}

double SketchProject::getViewTime() const
{
    return viewTime;
}

void SketchProject::setViewTime(double time)
{
    viewTime = time;
    if (!isDoingAnimation)
        world->setAnimationTime(time);
}

void SketchProject::timestep(double dt) {
    if (!isDoingAnimation) {
        //handleInput();
        world->stepPhysics(dt);
        q_vec_type point = { 0, PLANE_Y, 0}, vector = {0, 1, 0};
        vtkLinearTransform *rTW = transforms->getRoomToWorldTransform();
        rTW->TransformPoint(point,point);
        rTW->TransformVector(vector,vector);
        q_vec_normalize(vector,vector);
        q_vec_add(point,vector,point);
        world->setShadowPlane(point,vector);
        transforms->copyCurrentHandTransformsToOld();
    } else {
		double elapsed_time = time.restart();
        if (world->setAnimationTime(timeInAnimation)) {
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

SketchModel* SketchProject::addModel(SketchModel* model)
{
    model = models->addModel(model);
    return model;
}


SketchModel* SketchProject::addModelFromFile(const QString& source, const QString& fileName,
                                             double iMass, double iMoment)
{
    QString newFileName;
    if (getFileInProjDir(fileName,newFileName))
    {
      return models->makeModel(source,newFileName,iMass,iMoment);
    } else {
      // Can't throw, called from Qt slot
      qDebug() << "Failed to copy file to project directory: " << fileName;
      return NULL;
    }
}

SketchObject* SketchProject::addObject(SketchModel* model, const q_vec_type pos,
                                       const q_type orient)
{
    int myIdx = world->getNumberOfObjects();
    SketchObject* object = world->addObject(model,pos,orient);
    //object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
    if (object->numInstances() == 1)
    {
        object->setColorMapType(COLORS[myIdx%NUM_COLORS]);
    }
    return object;
}

SketchObject* SketchProject::addObject(const QString& source,const QString& filename)
{
    SketchModel* model = NULL;
    model = models->getModel(source);
    if (model == NULL)
    {
        model = addModelFromFile(source,filename,DEFAULT_INVERSE_MASS,
                                 DEFAULT_INVERSE_MOMENT);
        if (model == NULL)
            return NULL;
    }

    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    pos[Q_Y] = 2 * world->getNumberOfObjects() / transforms->getWorldToRoomScale();
    return addObject(model,pos,orient);
}

SketchObject* SketchProject::addObject(SketchObject* object) {
    int myIdx = world->getNumberOfObjects();
    if (object->numInstances() == 1) {
    //    object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
        object->setColorMapType(COLORS[myIdx%NUM_COLORS]);
    }
    world->addObject(object);
    // this is for when the object is read in from a file, this method is called
    // with the objects instead of addCamera.  So this needs to recognize cameras
    QDir projectDir(projectDirName);
    if (object->getModel() == models->getCameraModel(projectDir)) {
        cameras.insert(object,vtkSmartPointer<vtkCamera>::New());
        // cameras are not visible! make sure they are not.
        if (object->isVisible()) {
            object->setIsVisible(false);
        }
    }
    return object;
}

bool SketchProject::addObjects(const QVector<QString>& filenames)
{

    int i;
    for (i = 0; i < filenames.size(); i++) {
      // XXX Check for error here and return false if one found.
        // notes: no good cross-platform way to check if file exists
        // VTK prints out errors but does not throw anything, so we must
        // do error checking before calling this
        addObject(filenames[i],filenames[i]);
    }

    return true;
}

SketchObject* SketchProject::addCamera(const q_vec_type pos, const q_type orient)
{
    QDir projectDir(projectDirName);
    SketchModel* model = models->getCameraModel(projectDir);
    SketchObject* obj = addObject(model,pos,orient);
    // cameras are invisible (from the animation's standpoint)
    obj->setIsVisible(false);
    // if this is the first camera added, make it active
    if (cameras.empty()) {
        obj->setActive(true);
    }
    cameras.insert(obj,vtkSmartPointer<vtkCamera>::New());
    return obj;
}

Connector* SketchProject::addSpring(SketchObject* o1, SketchObject* o2, double minRest, double maxRest,
                                  double stiffness, q_vec_type o1Pos, q_vec_type o2Pos)
{
    return world->addSpring(o1,o2,o1Pos,o2Pos,false,stiffness,minRest,maxRest);
}

Connector* SketchProject::addConnector(Connector* spring)
{
    return world->addConnector(spring);
}

StructureReplicator* SketchProject::addReplication(SketchObject* o1, SketchObject* o2,
                                                   int numCopies)
{
    StructureReplicator* rep = new StructureReplicator(o1,o2,world.data());
    replicas.append(rep);
    rep->setNumShown(numCopies);
    return rep;
}

void SketchProject::addReplication(StructureReplicator* rep)
{
    replicas.append(rep);
}

QWeakPointer<TransformEquals> SketchProject::addTransformEquals(
        SketchObject* o1, SketchObject* o2)
{
    QSharedPointer<TransformEquals> trans(new TransformEquals(o1,o2,world.data()));
    transformOps.append(trans);
    return trans.toWeakRef();
}

/*
 * Updates the positions and transformations of the tracker objects.
 */
void SketchProject::updateTrackerPositions()
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

void SketchProject::setOutlineObject(int outlineIdx, SketchObject* obj)
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
        actor->GetProperty()->SetColor(OUTLINES_COLOR);
    }
}

void SketchProject::setOutlineSpring(int outlineIdx, Connector* conn, bool end1Large)
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

    actor->GetProperty()->SetColor(OUTLINES_COLOR);
}

void SketchProject::setOutlineVisible(int outlineIdx, bool visible)
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

bool SketchProject::isOutlineVisible(int outlineIdx)
{
    return renderer->HasViewProp(outlines[outlineIdx].second);
}

void SketchProject::setOutlineType(OutlineType type)
{
  outlineType = type;
  updateOutlines(SketchBioHandId::LEFT);
  updateOutlines(SketchBioHandId::RIGHT);
}

void SketchProject::updateOutlines(SketchBioHandId::Type side)
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

SketchModel* SketchProject::getCameraModel() {
  QDir projectDir(projectDirName);
  return models->getCameraModel(projectDir);
}

/*
 * This function takes a q_vec_type and returns the index of
 * the component with the minimum magnitude.
 */
inline int getMinIdx(const q_vec_type vec) {
    if (Q_ABS(vec[Q_X]) < Q_ABS(vec[Q_Y])) {
        if (Q_ABS(vec[Q_X]) < Q_ABS(vec[Q_Z])) {
            return Q_X;
        } else {
            return Q_Z;
        }
    } else {
        if (Q_ABS(vec[Q_Y]) < Q_ABS(vec[Q_Z])) {
            return Q_Y;
        } else {
            return Q_Z;
        }
    }
}


void SketchProject::setUpVtkCamera(SketchObject* cam, vtkCamera* vCam) {
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

SketchObject* SketchProject::addCameraObjectFromCameraPosition(vtkCamera* cam)
{
    q_vec_type pos;
    q_type orient;

    getCameraPositionAndOrientation(cam,pos,orient);

    SketchObject* obj = addCamera(pos,orient);
    return obj;
}

void SketchProject::setCameraToVTKCameraPosition(SketchObject* cam, vtkCamera* vcam)
{
    q_vec_type pos;
    q_type orient;

    getCameraPositionAndOrientation(vcam,pos,orient);

    cam->setPosAndOrient(pos,orient);
}
