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
    virtual void setWireFrame() {}
    virtual void setSolid() {}
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags) { return false;}
    virtual void getAABoundingBox(double bb[]) {}

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

SketchProject::SketchProject(vtkRenderer *r,
                             const bool *buttonStates, const double *analogStates) :
    models(new ModelManager()),
    transforms(new TransformManager()),
    world(new WorldManager(r,transforms->getWorldToEyeTransform())),
    replicas(),
    projectDir(NULL),
    buttonDown(buttonStates),
    analog(analogStates)
{
    transforms->scaleWorldRelativeToRoom(SCALE_DOWN_FACTOR);
    grabbedWorld = WORLD_NOT_GRABBED;
    leftHand = addTracker(r);
//    leftHand->setDoPhysics(false);
    rightHand = addTracker(r);
//    rightHand->setDoPhysics(false);
    lObj = rObj = (SketchObject *) NULL;
    lDist = rDist = std::numeric_limits<double>::max();
}

SketchProject::~SketchProject() {
    for (QMutableListIterator<StructureReplicator *> it(replicas); it.hasNext();) {
        StructureReplicator *rep = it.next();
        it.setValue((StructureReplicator *) NULL);
        delete rep;
    }
    replicas.clear();
    delete leftHand;
    leftHand = NULL;
    delete rightHand;
    rightHand = NULL;
    delete world;
    world = NULL;
    delete transforms;
    transforms = NULL;
    delete models;
    models = NULL;
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

void SketchProject::timestep(double dt) {
    handleInput();
    world->stepPhysics(dt);
    transforms->copyCurrentHandTransformsToOld();
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
    return world->addObject(object);
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

SpringConnection *SketchProject::addSpring(SketchObject *o1, SketchObject *o2, double minRest, double maxRest,
                                  double stiffness, q_vec_type o1Pos, q_vec_type o2Pos) {
    return world->addSpring(o1,o2,o1Pos,o2Pos,false,stiffness,minRest,maxRest);
}

SpringConnection *SketchProject::addSpring(SpringConnection *spring) {
    return world->addSpring(spring);
}

StructureReplicator *SketchProject::addReplication(SketchObject *o1, SketchObject *o2, int numCopies) {
    StructureReplicator *rep = new StructureReplicator(o1,o2,world,transforms->getWorldToEyeTransform());
    replicas.append(rep);
    rep->setNumShown(numCopies);
    return rep;
}

void SketchProject::handleInput() {
    q_vec_type beforeDVect, afterDVect, beforeLPos, beforeRPos, afterLPos, afterRPos;
    q_type beforeLOr, afterLOr, beforeROr, afterROr;

    // get former measurements
    double dist = transforms->getOldWorldDistanceBetweenHands();
    transforms->getOldLeftTrackerPosInWorldCoords(beforeLPos);
    transforms->getOldRightTrackerPosInWorldCoords(beforeRPos);
    transforms->getOldLeftTrackerOrientInWorldCoords(beforeLOr);
    transforms->getOldRightTrackerOrientInWorldCoords(beforeROr);
    transforms->getOldLeftToRightHandWorldVector(beforeDVect);

    // get current measurements
    double delta = transforms->getWorldDistanceBetweenHands() / dist;
    transforms->getLeftToRightHandWorldVector(afterDVect);
    transforms->getLeftTrackerPosInWorldCoords(afterLPos);
    transforms->getRightTrackerPosInWorldCoords(afterRPos);
    transforms->getLeftTrackerOrientInWorldCoords(afterLOr);
    transforms->getRightTrackerOrientInWorldCoords(afterROr);

    // possibly scale the world
    if (buttonDown[SCALE_BUTTON]) {
        transforms->scaleWithLeftTrackerFixed(delta);
    }
    // possibly rotate the world
    if (buttonDown[ROTATE_BUTTON]) {
        q_type rotation;
        q_normalize(afterDVect,afterDVect);
        q_normalize(beforeDVect,beforeDVect);
        q_from_two_vecs(rotation,beforeDVect,afterDVect);
        transforms->rotateWorldRelativeToRoomAboutLeftTracker(rotation);
    }
    // if the world is grabbed, translate/rotate it
    if (grabbedWorld == LEFT_GRABBED_WORLD) {
        // translate
        q_vec_type delta;
        q_vec_subtract(delta,afterLPos,beforeLPos);
        transforms->translateWorldRelativeToRoom(delta);
        // rotate
        q_type inv, rotation;
        q_invert(inv,beforeLOr);
        q_mult(rotation,afterLOr,inv);
        transforms->rotateWorldRelativeToRoomAboutLeftTracker(rotation);
    } else if (grabbedWorld == RIGHT_GRABBED_WORLD) {
        // translate
        q_vec_type delta;
        q_vec_subtract(delta,afterRPos,beforeRPos);
        transforms->translateWorldRelativeToRoom(delta);
        // rotate
        q_type inv, rotation;
        q_invert(inv,beforeROr);
        q_mult(rotation,afterROr,inv);
        transforms->rotateWorldRelativeToRoomAboutRightTracker(rotation);
    }
    // move fibers
    updateTrackerObjectConnections();

    if (world->getNumberOfObjects() > 0) {
        bool oldExists = false;
        bool leftWired = false;
        bool rightGrabbed = world->getRightSprings()->size() > 0;

        SketchObject *closest = NULL;

        if (world->getLeftSprings()->size() == 0 ) {
            oldExists = lDist != std::numeric_limits<double>::max();
            closest = world->getClosestObject(leftHand,&lDist);
            if (lObj == closest) {
                if (lDist < DISTANCE_THRESHOLD) {
                    closest->setWireFrame();
                    leftWired = true;
                } else if (!rightGrabbed || closest != rObj) {
                    closest->setSolid();
                }
            } else {
                if (oldExists && (!rightGrabbed || lObj != rObj)) {
                    lObj->setSolid();
                }
                lObj = closest;
                if (lDist < DISTANCE_THRESHOLD) {
                    lObj->setWireFrame();
                    leftWired = true;
                }
            }
        } else {
            leftWired = true;
        }

        if (!rightGrabbed) {
            oldExists = rDist != std::numeric_limits<double>::max();
            closest = world->getClosestObject(rightHand,&rDist);

            if (rObj == closest) {
                if (rDist < DISTANCE_THRESHOLD) {
                    closest->setWireFrame();
                } else if (!leftWired || lObj != closest) {
                    closest->setSolid();
                }
            } else {
                if (oldExists && (!leftWired || lObj != rObj)) {
                    rObj->setSolid();
                }
                rObj = closest;
                if (rDist < DISTANCE_THRESHOLD) {
                    rObj->setWireFrame();
            }
            }
        }
    }

    // set tracker locations
    updateTrackerPositions();
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

/*
 * This method updates the springs connecting the trackers and the objects in the world->..
 */
void SketchProject::updateTrackerObjectConnections() {
    if (world->getNumberOfObjects() == 0)
        return;
    for (int i = 0; i < 2; i++) {
        int analogIdx;
        int worldGrabConstant;
        SketchObject *objectToGrab;
        SketchObject *tracker;
        QList<SpringConnection *> *springs;
        double dist;
        // select left or right
        if (i == 0) {
            analogIdx = HYDRA_LEFT_TRIGGER;
            worldGrabConstant = LEFT_GRABBED_WORLD;
            tracker = leftHand;
            springs = world->getLeftSprings();
            objectToGrab = lObj;
            dist = lDist;
        } else if (i == 1) {
            analogIdx = HYDRA_RIGHT_TRIGGER;
            worldGrabConstant = RIGHT_GRABBED_WORLD;
            tracker = rightHand;
            springs = world->getRightSprings();
            objectToGrab = rObj;
            dist = rDist;
        }
        // if they are gripping the trigger
        if (analog[analogIdx] > .1) {
            // if we do not have springs yet add them
            if (springs->size() == 0 && grabbedWorld != worldGrabConstant) { // add springs
                if (dist > DISTANCE_THRESHOLD) {
                    if (grabbedWorld == WORLD_NOT_GRABBED) {
                        grabbedWorld = worldGrabConstant;
                    }
                    // allow grabbing world & moving something with other hand...
                    // discouraged, but allowed -> the results are not guaranteed.
                } else {
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
                                                           analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    if (i == 0) {
                        world->addLeftHandSpring(spring);
                    } else if (i == 1) {
                        world->addRightHandSpring(spring);
                    }
                    // second spring --defined as rotated 120 degrees about "z" axis.
                    // coordinates in terms of x and y: (-1/2x, sqrt(3)/2y)
                    q_vec_scale(tPer1,-.5,tPer1);
                    q_vec_scale(tPer2,sqrt(3.0)/2,tPer2);
                    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                    q_vec_add(wPos2,wPos2,tPer2); // + sqrt(3)/2 y
                    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                                           analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    if (i == 0) {
                        world->addLeftHandSpring(spring);
                    } else if (i == 1) {
                        world->addRightHandSpring(spring);
                    }
                    // third spring --defined as rotated 240 degrees about "z" axis.
                    // coordinates in terms of x and y: (-1/2x, -sqrt(3)/2y)
                    q_vec_invert(tPer2,tPer2);
                    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                    q_vec_add(wPos2,wPos2,tPer2); // - sqrt(3)/2 y
                    spring = InterObjectSpring::makeSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                                           analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    if (i == 0) {
                        world->addLeftHandSpring(spring);
                    } else if (i == 1) {
                        world->addRightHandSpring(spring);
                    }
                }
            } else { // update springs stiffness if they are already there
                for (QListIterator<SpringConnection *> it(*springs); it.hasNext();) {
                    it.next()->setStiffness(analog[analogIdx]);
                }
            }
        } else {
            if (!springs->empty()) { // if we have springs and they are no longer gripping the trigger
                // remove springs between model & tracker, set grabbed objects solid
                if (i == 0) {
                    SketchObject *obj = world->getLeftSprings()->first()->getObject1();
                    if (obj != leftHand) {
                        obj->setSolid();
                    } else {
                        (dynamic_cast<InterObjectSpring *>(world->getLeftSprings()->first()))->getObject2()->setSolid();
                    }
                    world->clearLeftHandSprings();
                } else if (i == 1) {
                    SketchObject *obj = world->getRightSprings()->first()->getObject1();
                    if (obj != rightHand) {
                        obj->setSolid();
                    } else {
                        (dynamic_cast<InterObjectSpring *>(world->getRightSprings()->first()))->getObject2()->setSolid();
                    }
                    world->clearRightHandSprings();
                }
            }
            if (grabbedWorld == worldGrabConstant) {
                grabbedWorld = WORLD_NOT_GRABBED;
            }
        }
    }
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
