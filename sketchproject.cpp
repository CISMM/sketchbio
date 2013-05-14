#include "sketchproject.h"

#include <vtkOBJReader.h>
#include <vtkSphereSource.h>
#include <vtkProperty.h>
#include "sketchioconstants.h"
#include <limits>
#include <QDebug>

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
    virtual void getWorldVectorInModelSpace(const q_vec_type worldVec, q_vec_type modelVecOut) const {
        q_type orient;
        getOrientation(orient);
        q_type orientInv;
        q_invert(orientInv,orient);
        q_xform(modelVecOut,orientInv,worldVec);

    }

    virtual int numInstances() const { return 0; }
    virtual vtkActor *getActor() { return actor; }
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags) { return false;}
    virtual void getAABoundingBox(double bb[]) {}
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes() { return NULL;}

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

SketchProject::SketchProject(vtkRenderer *r) :
    renderer(r),
    models(new ModelManager()),
    transforms(new TransformManager()),
    world(new WorldManager(r,transforms->getWorldToEyeTransform())),
    replicas(),
    cameras(),
    transformOps(),
    projectDir(NULL),
    leftOutlinesActor(vtkSmartPointer<vtkActor>::New()),
    rightOutlinesActor(vtkSmartPointer<vtkActor>::New()),
    leftOutlinesMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    rightOutlinesMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    isDoingAnimation(false),
    timeInAnimation(0.0)
{
    transforms->scaleWorldRelativeToRoom(SCALE_DOWN_FACTOR);
    renderer->SetActiveCamera(transforms->getGlobalCamera());
    leftHand = addTracker(r);
    rightHand = addTracker(r);
    leftOutlinesActor->SetMapper(leftOutlinesMapper);
    leftOutlinesActor->GetProperty()->SetColor(0.7,0.7,0.7);
    leftOutlinesActor->GetProperty()->SetLighting(false);
    rightOutlinesActor->SetMapper(rightOutlinesMapper);
    rightOutlinesActor->GetProperty()->SetColor(0.7,0.7,0.7);
    rightOutlinesActor->GetProperty()->SetLighting(false);
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
}

bool SketchProject::goToAnimationTime(double time) {
    if (!isDoingAnimation) {
        startAnimation();
    }
    timeInAnimation = time;
    return world->setAnimationTime(time);
}

void SketchProject::timestep(double dt) {
    if (!isDoingAnimation) {
        //handleInput();
        world->stepPhysics(dt);
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

SketchModel *SketchProject::addModelFromFile(QString fileName, double iMass, double iMoment, double scale) {
    QFile file(fileName);
//    qDebug() << filename;
    QString localname = fileName.mid(fileName.lastIndexOf("/") +1).toLower();
    QString fullpath = projectDir->absoluteFilePath(localname);
//    qDebug() << fullpath;
    if (projectDir->entryList().contains(localname, Qt::CaseInsensitive) || file.copy(fileName,fullpath)) {
        SketchModel *model = NULL;
        if (fileName.endsWith("obj", Qt::CaseInsensitive)) {
            model = models->modelForOBJSource(fullpath,iMass,iMoment,scale);
        }
        return model;
    } else {
        throw "Cannot create local copy of model file";
    }
}

SketchObject *SketchProject::addObject(SketchModel *model, const q_vec_type pos, const q_type orient) {
    int myIdx = world->getNumberOfObjects();
    SketchObject *object = world->addObject(model,pos,orient);
    object->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
    return object;
}

SketchObject *SketchProject::addObject(QString filename) {
    QFile file(filename);
//    qDebug() << filename;
    QString localname = filename.mid(filename.lastIndexOf("/") +1).toLower();
    QString fullpath = projectDir->absoluteFilePath(localname);
    QFile localfile(fullpath);
//    qDebug() << fullpath;
    if (localfile.exists() || file.copy(filename,fullpath)) {
        SketchModel *model = models->modelForOBJSource(fullpath);

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
    if (object->getModel() == models->getCameraModel()) {
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
        addObject(filenames[i]);
    }

    return true;
}

SketchObject *SketchProject::addCamera(const q_vec_type pos, const q_type orient) {
    SketchModel *model = models->getCameraModel();
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

void SketchProject::setRightOutlineObject(SketchObject *obj) {
    rightOutlinesMapper->SetInputConnection(obj->getOrientedBoundingBoxes()->GetOutputPort());
    rightOutlinesMapper->Update();
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
    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                           OBJECT_GRAB_SPRING_CONST,abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
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
    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                           OBJECT_GRAB_SPRING_CONST,abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
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
    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                           OBJECT_GRAB_SPRING_CONST,abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
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
}
