#include "sketchproject.h"

#include <limits>

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkOBJReader.h>
#include <vtkPlaneSource.h>
#include <vtkSphereSource.h>
#include <vtkConeSource.h>
#include <vtkExtractEdges.h>
#include <vtkArrayCalculator.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourFilter.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <QDebug>
#include <QDir>

#include "sketchioconstants.h"
#include "transformmanager.h"
#include "modelmanager.h"
#include "springconnection.h"
#include "structurereplicator.h"
#include "transformequals.h"

#define NUM_COLORS (6)
static double COLORS[][3] =
  {  { 1.0, 0.7, 0.7 },
     { 0.7, 1.0, 0.8 },
     { 0.7, 0.7, 1.0 },
     { 1.0, 1.0, 0.7 },
     { 1.0, 0.7, 1.0 },
     { 0.7, 1.0, 1.0 } };

class TrackerObject : public SketchObject {
public:
    TrackerObject() : SketchObject(), actor(vtkSmartPointer<vtkActor>::New()) {
        vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
        sphereSource->SetRadius(4 * TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*SCALE_DOWN_FACTOR);
        sphereSource->Update();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphereSource->GetOutputPort());
        mapper->Update();
        actor->SetMapper(mapper);
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->Identity();
        actor->SetUserTransform(transform);
        setLocalTransformPrecomputed(true);
        setLocalTransformDefiningPosition(false);
    }

    // I'm using disabling localTransform, so I need to reimplement these
    virtual void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint, q_vec_type worldCoordsOut) const {
        q_vec_type pos; q_type orient;
        getPosition(pos);
        getOrientation(orient);
        q_xform(worldCoordsOut,orient,modelPoint);
        q_vec_add(worldCoordsOut,pos,worldCoordsOut);
    }
    virtual void getWorldSpacePointInModelCoordinates(const q_vec_type worldPoint, q_vec_type modelCoordsOut) const {
        q_vec_type pos;
        q_type orient;
        getPosition(pos);
        getOrientation(orient);
        q_invert(orient,orient);
        q_vec_invert(pos,pos);
        q_vec_add(modelCoordsOut,pos,worldPoint);
        q_xform(modelCoordsOut,orient,modelCoordsOut);
    }
    virtual void getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const {
        q_type orient;
        getOrientation(orient);
        q_type orientInv;
        q_invert(orientInv,orient);
        q_xform(modelVecOut,orientInv,worldVec);

    }
    virtual void getModelVectorInWorldSpace(const q_vec_type modelVec, q_vec_type worldVecOut) const {
        q_type orient;
        getOrientation(orient);
        q_xform(worldVecOut,orient,modelVec);
    }

    virtual int numInstances() const { return 0; }
    virtual vtkActor *getActor() { return actor; }
    virtual vtkPolyDataAlgorithm *getTransformedGeometry() { return NULL; }
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags) { return false;}
    virtual void getBoundingBox(double bb[]) {}
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes() { return NULL;}
    virtual SketchObject *deepCopy() { return NULL; }

private:
    vtkSmartPointer<vtkActor> actor;
};

inline SketchObject *addTracker(vtkRenderer *r) {
    vtkSmartPointer<vtkActor> actor;
    SketchObject *tracker = new TrackerObject();
    actor = tracker->getActor();
    r->AddActor(actor);
    return tracker;
}

inline void makeFloorAndLines(vtkPlaneSource * shadowFloorSource,
                              vtkActor *shadowFloorActor,
                              vtkActor *floorLinesActor,
                              TransformManager *transforms)
{
// the half x-length of the plane in room units
#define PLANE_HALF_LENGTH 30
// the y height of the plane in room units
#define PLANE_Y -6
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
// the offset in room space of the lines above the plane to prevent OpenGL
// depth problems
#define LINE_FROM_PLANE_OFFSET 0.3

// The color of the lines over the plane (R, G, and B components all set to this)
#define LINES_COLOR 0.3
// The color of the plane itself (R, G, and B components all set to this)
#define PLANE_COLOR 0.45

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
    colors->AddRGBPoint(0,LINES_COLOR,LINES_COLOR,LINES_COLOR);
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
    shadowFloorActor->GetProperty()->SetColor(PLANE_COLOR,PLANE_COLOR,PLANE_COLOR);
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

SketchProject::SketchProject(vtkRenderer *r) :
    renderer(r),
    models(new ModelManager()),
    transforms(new TransformManager()),
    world(new WorldManager(r)),
    replicas(),
    cameras(),
    transformOps(),
    projectDir(NULL),
    leftHand(addTracker(r)),
    rightHand(addTracker(r)),
    leftOutlinesActor(vtkSmartPointer<vtkActor>::New()),
    rightOutlinesActor(vtkSmartPointer<vtkActor>::New()),
    leftOutlinesMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    rightOutlinesMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    shadowFloorSource(vtkSmartPointer< vtkPlaneSource >::New()),
    shadowFloorActor(vtkSmartPointer< vtkActor >::New()),
    floorLinesActor(vtkSmartPointer< vtkActor >::New()),
    isDoingAnimation(false),
    timeInAnimation(0.0),
    viewTime(0.0)
{
    // set up initial transforms
    transforms->scaleWorldRelativeToRoom(SCALE_DOWN_FACTOR);
    // set up initial camera
    renderer->SetActiveCamera(transforms->getGlobalCamera());
    // connect outlines mapper & actor
    leftOutlinesActor->SetMapper(leftOutlinesMapper);
    leftOutlinesActor->GetProperty()->SetColor(0.7,0.7,0.7);
    leftOutlinesActor->GetProperty()->SetLighting(false);
    rightOutlinesActor->SetMapper(rightOutlinesMapper);
    rightOutlinesActor->GetProperty()->SetColor(0.7,0.7,0.7);
    rightOutlinesActor->GetProperty()->SetLighting(false);

    // make the floor and lines
    makeFloorAndLines(shadowFloorSource,shadowFloorActor,floorLinesActor,
                      transforms.data());
    // add the floor and lines
    renderer->AddActor(floorLinesActor);
    renderer->AddActor(shadowFloorActor);
}

SketchProject::~SketchProject() {
    cameras.clear();
    qDeleteAll(replicas);
    replicas.clear();
    delete leftHand;
    leftHand = NULL;
    delete rightHand;
    rightHand = NULL;
    if (projectDir != NULL) {
        delete projectDir;
    }
}

bool SketchProject::setProjectDir(QString dir) {
    QDir tmp = QDir(dir);
    if (tmp.isRelative()) {
        QString abs = QDir::current().absoluteFilePath(dir);
        tmp = QDir(abs);
    }
    bool exists;
    if (!(exists = tmp.exists())) {
        tmp.mkpath(".");
        exists = tmp.exists();
    }
    if (projectDir != NULL) {
        delete projectDir;
    }
    projectDir = new QDir(tmp.absolutePath());
    return exists;
}

void SketchProject::setLeftHandPos(q_xyz_quat_type *loc) {
    transforms->setLeftHandTransform(loc);
}

void SketchProject::setRightHandPos(q_xyz_quat_type *loc) {
    transforms->setRightHandTransform(loc);
}

void SketchProject::setViewpoint(vtkMatrix4x4 *worldToRoom, vtkMatrix4x4 *roomToEyes) {
    transforms->setWorldToRoomMatrix(worldToRoom);
    transforms->setRoomToEyeMatrix(roomToEyes);
}

void SketchProject::setCollisionMode(CollisionMode::Type mode) {
    world->setCollisionMode(mode);
}

void SketchProject::setCollisionTestsOn(bool on) {
    world->setCollisionCheckOn(on);
}

void SketchProject::setWorldSpringsEnabled(bool enabled) {
    world->setPhysicsSpringsOn(enabled);
}

QString SketchProject::getProjectDir() const {
    return projectDir->absolutePath();
}

void SketchProject::startAnimation() {
    isDoingAnimation = true;
    timeInAnimation = 0.0;
    renderer->RemoveActor(leftHand->getActor());
    renderer->RemoveActor(rightHand->getActor());
    if (renderer->HasViewProp(leftOutlinesActor)) {
        renderer->RemoveActor(leftOutlinesActor);
    }
    if (renderer->HasViewProp(rightOutlinesActor)) {
        renderer->RemoveActor(rightOutlinesActor);
    }
    world->clearLeftHandSprings();
    world->clearRightHandSprings();
    world->hideInvisibleObjects();
}

void SketchProject::stopAnimation() {
    isDoingAnimation = false;
    renderer->AddActor(leftHand->getActor());
    renderer->AddActor(rightHand->getActor());
    // distances and outlines actors will refresh themselves
    // handed springs and world grab should refresh themselves too
    world->showInvisibleObjects();
    renderer->SetActiveCamera(transforms->getGlobalCamera());
    world->setAnimationTime(viewTime);
}

bool SketchProject::goToAnimationTime(double time) {
    if (!isDoingAnimation) {
        startAnimation();
    }
    timeInAnimation = time;
    return world->setAnimationTime(time);
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
        if (world->setAnimationTime(timeInAnimation)) {
            // if setAnimationTime returns true, then we are done
            stopAnimation();
        } else {
            for (QHashIterator<SketchObject *, vtkSmartPointer<vtkCamera> > it(cameras); it.hasNext(); ) {
                SketchObject *cam = it.peekNext().key();
                if (cam->isActive()) {
                    vtkSmartPointer<vtkCamera> vCam = it.next().value();
                    setUpVtkCamera(cam,vCam);
                    renderer->SetActiveCamera(vCam);
                }
            }
        }
        timeInAnimation += dt;
    }
}

SketchModel *SketchProject::addModel(SketchModel *model)
{
    models->addModel(model);
    return model;
}


SketchModel *SketchProject::addModelFromFile(QString source, QString fileName,
                                             double iMass, double iMoment) {
    QFile file(fileName);
//    qDebug() << filename;
    QString localname = fileName.mid(fileName.lastIndexOf("/") +1);
    QString fullpath = projectDir->absoluteFilePath(localname);
//    qDebug() << fullpath;
    if (projectDir->entryList().contains(localname, Qt::CaseInsensitive)
            || file.copy(fileName,fullpath)) {
        SketchModel *model = models->makeModel(source,fullpath,iMass, iMoment);
        return model;
    } else {
        throw "Cannot create local copy of model file";
    }
}

SketchObject *SketchProject::addObject(SketchModel *model, const q_vec_type pos,
                                       const q_type orient) {
    int myIdx = world->getNumberOfObjects();
    SketchObject *object = world->addObject(model,pos,orient);
    object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
    return object;
}

SketchObject *SketchProject::addObject(QString source,QString filename) {
    QFile file(filename);
//    qDebug() << filename;
    QString localname = filename.mid(filename.lastIndexOf("/") +1);
    QString fullpath = projectDir->absoluteFilePath(localname);
    QFile localfile(fullpath);
//    qDebug() << fullpath;
    if (localfile.exists() || file.copy(filename,fullpath)) {
        SketchModel *model = models->makeModel(source,filename,DEFAULT_INVERSE_MASS,
                                               DEFAULT_INVERSE_MOMENT);

        q_vec_type pos = Q_NULL_VECTOR;
        q_type orient = Q_ID_QUAT;
        pos[Q_Y] = 2 * world->getNumberOfObjects() / transforms->getWorldToRoomScale();
        return addObject(model,pos,orient);
    } else {
        if (!localfile.exists()) {
            qDebug() << "File is not there" << endl;
            if (!file.copy(filename,fullpath)) {
                qDebug() << "Cannot create copy" << endl;
            }
        }
        throw "Could not create local copy of model file";
    }
}

SketchObject *SketchProject::addObject(SketchObject *object) {
    int myIdx = world->getNumberOfObjects();
    if (object->numInstances() == 1) {
        object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
    }
    world->addObject(object);
    // this is for when the object is read in from a file, this method is called
    // with the objects instead of addCamera.  So this needs to recognize cameras
    if (object->getModel() == models->getCameraModel(*projectDir)) {
        cameras.insert(object,vtkSmartPointer<vtkCamera>::New());
        // cameras are not visible! make sure they are not.
        if (object->isVisible()) {
            object->setIsVisible(false);
        }
    }
    return object;
}

bool SketchProject::addObjects(QVector<QString> filenames) {

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

SketchObject *SketchProject::addCamera(const q_vec_type pos, const q_type orient) {
    SketchModel *model = models->getCameraModel(*projectDir);
    SketchObject *obj = addObject(model,pos,orient);
    // cameras are invisible (from the animation's standpoint)
    obj->setIsVisible(false);
    // if this is the first camera added, make it active
    if (cameras.empty()) {
        obj->setActive(true);
    }
    cameras.insert(obj,vtkSmartPointer<vtkCamera>::New());
    return obj;
}

SpringConnection *SketchProject::addSpring(SketchObject *o1, SketchObject *o2, double minRest, double maxRest,
                                  double stiffness, q_vec_type o1Pos, q_vec_type o2Pos) {
    return world->addSpring(o1,o2,o1Pos,o2Pos,false,stiffness,minRest,maxRest);
}

SpringConnection *SketchProject::addSpring(SpringConnection *spring) {
    return world->addSpring(spring);
}

StructureReplicator *SketchProject::addReplication(SketchObject *o1, SketchObject *o2, int numCopies) {
    StructureReplicator *rep = new StructureReplicator(o1,o2,world.data());
    replicas.append(rep);
    rep->setNumShown(numCopies);
    return rep;
}

QWeakPointer<TransformEquals> SketchProject::addTransformEquals(SketchObject *o1, SketchObject *o2) {
    QSharedPointer<TransformEquals> trans(new TransformEquals(o1,o2,world.data()));
    transformOps.append(trans);
    return trans.toWeakRef();
}

/*
 * Updates the positions and transformations of the tracker objects.
 */
void SketchProject::updateTrackerPositions() {
    q_vec_type pos;
    q_type orient;
    transforms->getLeftTrackerPosInWorldCoords(pos);
    transforms->getLeftTrackerOrientInWorldCoords(orient);
    leftHand->setPosAndOrient(pos,orient);
    transforms->getLeftTrackerTransformInEyeCoords((vtkTransform*)leftHand->getActor()->GetUserTransform());
    transforms->getRightTrackerPosInWorldCoords(pos);
    transforms->getRightTrackerOrientInWorldCoords(orient);
    rightHand->setPosAndOrient(pos,orient);
    transforms->getRightTrackerTransformInEyeCoords((vtkTransform*)rightHand->getActor()->GetUserTransform());
}

void SketchProject::setLeftOutlineObject(SketchObject *obj) {
    leftOutlinesMapper->SetInputConnection(obj->getOrientedBoundingBoxes()->GetOutputPort());
    leftOutlinesMapper->Update();
}

void SketchProject::setLeftOutlineSpring(SpringConnection *conn, bool end1Large) {
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

    leftOutlinesMapper->SetInputConnection(edges->GetOutputPort());
}

void SketchProject::setRightOutlineObject(SketchObject *obj) {
    rightOutlinesMapper->SetInputConnection(obj->getOrientedBoundingBoxes()->GetOutputPort());
    rightOutlinesMapper->Update();
}

void SketchProject::setRightOutlineSpring(SpringConnection *conn, bool end1Large) {
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

    rightOutlinesMapper->SetInputConnection(edges->GetOutputPort());
}


void SketchProject::setLeftOutlinesVisible(bool visible) {
    if (visible)
        renderer->AddActor(leftOutlinesActor);
    else
        renderer->RemoveActor(leftOutlinesActor);
}

void SketchProject::setRightOutlinesVisible(bool visible) {
    if (visible)
        renderer->AddActor(rightOutlinesActor);
    else
        renderer->RemoveActor(rightOutlinesActor);
}

SketchModel *SketchProject::getCameraModel() {
    return models->getCameraModel(*projectDir);
}

bool SketchProject::isLeftOutlinesVisible() {
    return renderer->HasViewProp(leftOutlinesActor);
}

bool SketchProject::isRightOutlinesVisible() {
    return renderer->HasViewProp(rightOutlinesActor);
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

void SketchProject::grabObject(SketchObject *objectToGrab, bool grabWithLeft) {
    SketchObject *tracker = NULL;
    if (grabWithLeft) {
        tracker = leftHand;
    } else {
        tracker = rightHand;
    }

    q_vec_type oPos, tPos, vec;
    objectToGrab->getPosition(oPos);
    tracker->getPosition(tPos);
    q_vec_subtract(vec,tPos,oPos);
    q_vec_normalize(vec,vec);
    q_vec_type axis = Q_NULL_VECTOR;
    axis[getMinIdx(vec)] = 1; // this gives an axis that is guaranteed not to be
                        // parallel to vec
    q_vec_type per1, per2; // create two perpendicular unit vectors
    q_vec_cross_product(per1,axis,vec);
    q_vec_normalize(per1,per1);
    q_vec_cross_product(per2,per1,vec); // should already be length 1
    // create scaled perpendicular vectors
    q_vec_type tPer1, tPer2;
    q_vec_scale(tPer1,TRACKER_SIDE_LEN,per1);
    q_vec_scale(tPer2,TRACKER_SIDE_LEN,per2);
    q_vec_type /*wPos1,*/ wPos2;
    // create springs and add them
    // first spring --defined along the "x" axis (per1)
    SpringConnection *spring;
    q_vec_add(wPos2,tPos,tPer1);
    spring = SpringConnection::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                          OBJECT_GRAB_SPRING_CONST,
                                          abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
    if (grabWithLeft) {
        world->addLeftHandSpring(spring);
    } else {
        world->addRightHandSpring(spring);
    }
    // second spring --defined as rotated 120 degrees about "z" axis.
    // coordinates in terms of x and y: (-1/2x, sqrt(3)/2y)
    q_vec_scale(tPer1,-.5,tPer1);
    q_vec_scale(tPer2,sqrt(3.0)/2,tPer2);
    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
    q_vec_add(wPos2,wPos2,tPer2); // + sqrt(3)/2 y
    spring = SpringConnection::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                          OBJECT_GRAB_SPRING_CONST,
                                          abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
    if (grabWithLeft) {
        world->addLeftHandSpring(spring);
    } else {
        world->addRightHandSpring(spring);
    }
    // third spring --defined as rotated 240 degrees about "z" axis.
    // coordinates in terms of x and y: (-1/2x, -sqrt(3)/2y)
    q_vec_invert(tPer2,tPer2);
    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
    q_vec_add(wPos2,wPos2,tPer2); // - sqrt(3)/2 y
    spring = SpringConnection::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                          OBJECT_GRAB_SPRING_CONST,
                                          abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
    if (grabWithLeft) {
        world->addLeftHandSpring(spring);
    } else {
        world->addRightHandSpring(spring);
    }
}

void SketchProject::setUpVtkCamera(SketchObject *cam, vtkCamera *vCam) {
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
    vCam->SetClippingRange(20,2000);
}
