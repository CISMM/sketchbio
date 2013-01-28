#include "sketchproject.h"

#include <vtkOBJReader.h>
#include <vtkProperty.h>
#include "sketchioconstants.h"
#include <limits>

#define NUM_COLORS (6)
static double COLORS[][3] =
  {  { 1.0, 0.7, 0.7 },
     { 0.7, 1.0, 0.8 },
     { 0.7, 0.7, 1.0 },
     { 1.0, 1.0, 0.7 },
     { 1.0, 0.7, 1.0 },
     { 0.7, 1.0, 1.0 } };

inline SketchObject *addTracker(vtkRenderer *r, SketchModelId modelId, vtkTransform *worldEyeTransform) {
    SketchModel *model = (*modelId);
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(model->getSolidMapper());
    SketchObject *newObject = new SketchObject(actor,modelId,worldEyeTransform);
    newObject->recalculateLocalTransform();
    r->AddActor(actor);
    return newObject;
}

SketchProject::SketchProject(vtkRenderer *r, SketchModelId trackerModel,
                             const bool *buttonStates, const double *analogStates) :
    models(new ModelManager()),
    transforms(new TransformManager()),
    world(new WorldManager(r,transforms->getWorldToEyeTransform())),
    buttonDown(buttonStates),
    analog(analogStates)
{
    leftHand = addTracker(r,trackerModel,transforms->getWorldToEyeTransform());
    rightHand = addTracker(r,trackerModel,transforms->getWorldToEyeTransform());
    lObj = rObj = std::_List_iterator<SketchObject*>();
    lDist = rDist = std::numeric_limits<double>::max();
}

SketchProject::~SketchProject() {
    delete world;
    delete transforms;
    delete models;
}

void SketchProject::timestep(double dt) {
    handleInput();
    world->stepPhysics(dt);
}

void SketchProject::addObject(QString filename) {
    vtkSmartPointer<vtkOBJReader> objReader =
            vtkSmartPointer<vtkOBJReader>::New();
    objReader->SetFileName(filename.toStdString().c_str());
    objReader->Update();
    int fiberSourceType = models->addObjectSource(objReader.GetPointer());
    SketchModelId fiberModelType = models->addObjectType(fiberSourceType,1);

    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    int myIdx = world->getNumberOfObjects();
    q_vec_set(pos,0,2*myIdx/transforms->getWorldToRoomScale(),0);
    ObjectId objectId = world->addObject(fiberModelType,pos,orient);
    (*objectId)->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
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
        ObjectId closest = world->getClosestObject(leftHand,&lDist);

        if (lObj == closest) {
            if (lDist < DISTANCE_THRESHOLD) {
                (*closest)->setWireFrame();
            } else {
                (*closest)->setSolid();
            }
        } else {
            if (*lObj)
                (*lObj)->setSolid();
            lObj = closest;
            if (lDist < DISTANCE_THRESHOLD)
                (*lObj)->setWireFrame();
        }

        closest = world->getClosestObject(rightHand,&rDist);

        if (rObj == closest) {
            if (rDist < DISTANCE_THRESHOLD) {
                (*closest)->setWireFrame();
            } else {
                (*closest)->setSolid();
            }
        } else {
            if (*rObj)
                (*rObj)->setSolid();
            rObj = closest;
            if (rDist < DISTANCE_THRESHOLD)
                (*rObj)->setWireFrame();
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
        ObjectId objectToGrab;
        SketchObject *tracker;
        std::vector<SpringId> *springs;
        double dist;
        // select left or right
        if (i == 0) {
            analogIdx = HYDRA_LEFT_TRIGGER;
            worldGrabConstant = LEFT_GRABBED_WORLD;
            tracker = leftHand;
            springs = &lhand;
            objectToGrab = lObj;
            dist = lDist;
        } else if (i == 1) {
            analogIdx = HYDRA_RIGHT_TRIGGER;
            worldGrabConstant = RIGHT_GRABBED_WORLD;
            tracker = rightHand;
            springs = &rhand;
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
                    SketchObject *obj = (*objectToGrab);
                    q_vec_type oPos, tPos, vec;
                    obj->getPosition(oPos);
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
                    SpringId id;
                    q_vec_add(wPos2,tPos,tPer1);
                    id = world->addSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                         analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    springs->push_back(id);
                    // second spring --defined as rotated 120 degrees about "z" axis.
                    // coordinates in terms of x and y: (-1/2x, sqrt(3)/2y)
                    q_vec_scale(tPer1,-.5,tPer1);
                    q_vec_scale(tPer2,sqrt(3.0)/2,tPer2);
                    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                    q_vec_add(wPos2,wPos2,tPer2); // + sqrt(3)/2 y
                    id = world->addSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                         analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    springs->push_back(id);
                    // third spring --defined as rotated 240 degrees about "z" axis.
                    // coordinates in terms of x and y: (-1/2x, -sqrt(3)/2y)
                    q_vec_invert(tPer2,tPer2);
                    q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                    q_vec_add(wPos2,wPos2,tPer2); // - sqrt(3)/2 y
                    id = world->addSpring(objectToGrab,tracker,wPos2,wPos2,true,
                                         analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                    springs->push_back(id);
                }
            } else { // update springs stiffness if they are already there
                for (std::vector<SpringId>::iterator it = springs->begin(); it != springs->end(); it++) {
                    (*(*it))->setStiffness(analog[analogIdx]);
                }
            }
        } else {
            if (!springs->empty()) { // if we have springs and they are no longer gripping the trigger
                // remove springs between model & tracker
                for (std::vector<SpringId>::iterator it = springs->begin(); it != springs->end(); it++) {
                    world->removeSpring(*it);
                }
                springs->clear();
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
    leftHand->setPosition(pos);
    leftHand->setOrientation(orient);
    transforms->getLeftTrackerTransformInEyeCoords((vtkTransform*)leftHand->getActor()->GetUserTransform());
    transforms->getRightTrackerPosInWorldCoords(pos);
    transforms->getRightTrackerOrientInWorldCoords(orient);
    rightHand->setPosition(pos);
    rightHand->setOrientation(orient);
    transforms->getRightTrackerTransformInEyeCoords((vtkTransform*)rightHand->getActor()->GetUserTransform());
}