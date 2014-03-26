#include "hand.h"

#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

//#define DEBUG_TRACKER_ORIENTATIONS

#ifdef DEBUG_TRACKER_ORIENTATIONS
#include <vtkCubeSource.h>
#else
#include <vtkSphereSource.h>
#endif

#include <vtkProjectToPlane.h>

#include "transformmanager.h"
#include "sketchobject.h"
#include "springconnection.h"
#include "worldmanager.h"

namespace SketchBio
{
// ###########################################################################
// ###########################################################################
// ###########################################################################
// ###########################################################################
// The TrackerObject class, a subclass of SketchObject to display the tracker

// This is a minimal subclass to provide basic functionality: showing the
// tracker geometry, providing geometry to be flattened to a shadow, and
// converting points and vectors to and from the object-local coordinate
// system

// the tracker is so small that we need to scale its shadow up to be visible
#define TRACKER_SHADOW_SCALE 50, 50, 50
// the color of the tracker's shadows
#define TRACKER_SHADOW_COLOR 0.0, 0.0, 0.0

class TrackerObject : public SketchObject
{
 public:
  TrackerObject()
      : SketchObject(),
        actor(vtkSmartPointer< vtkActor >::New()),
        shadow(vtkSmartPointer< vtkActor >::New()),
        shadowGeometry(vtkSmartPointer< vtkTransformPolyDataFilter >::New())
  {
    double d =
        TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE * SCALE_DOWN_FACTOR * 4;
    vtkSmartPointer< vtkPolyDataMapper > mapper =
        vtkSmartPointer< vtkPolyDataMapper >::New();
#ifndef DEBUG_TRACKER_ORIENTATIONS
    vtkSmartPointer< vtkSphereSource > sphereSource =
        vtkSmartPointer< vtkSphereSource >::New();
    sphereSource->SetRadius(d);
    sphereSource->Update();
    shadowGeometry->SetInputConnection(sphereSource->GetOutputPort());
    mapper->SetInputConnection(sphereSource->GetOutputPort());
#else
    vtkSmartPointer< vtkCubeSource > cubeSource =
        vtkSmartPointer< vtkCubeSource >::New();
    cubeSource->SetBounds(-d, d, -2 * d, 2 * d, -3 * d, 3 * d);
    cubeSource->Update();
    shadowGeometry->SetInputConnection(cubeSource->GetOutputPort());
    mapper->SetInputConnection(cubeSource->GetOutputPort());
#endif
    vtkSmartPointer< vtkTransform > sTrans =
        vtkSmartPointer< vtkTransform >::New();
    sTrans->Identity();
    sTrans->PostMultiply();
    sTrans->Scale(TRACKER_SHADOW_SCALE);
    shadowGeometry->SetTransform(sTrans);
    shadowGeometry->Update();
    mapper->Update();
    actor->SetMapper(mapper);
    vtkSmartPointer< vtkTransform > transform =
        vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    actor->SetUserTransform(transform);
    setLocalTransformPrecomputed(true);
    setLocalTransformDefiningPosition(false);
    vtkSmartPointer< vtkProjectToPlane > projection =
        vtkSmartPointer< vtkProjectToPlane >::New();
    projection->SetInputConnection(shadowGeometry->GetOutputPort());
    projection->SetPointOnPlane(0, PLANE_ROOM_Y + 0.1 * LINE_FROM_PLANE_OFFSET,
                                0);
    projection->SetPlaneNormalVector(0, 1, 0);
    projection->Update();
    vtkSmartPointer< vtkPolyDataMapper > shadowMapper =
        vtkSmartPointer< vtkPolyDataMapper >::New();
    shadowMapper->SetInputConnection(projection->GetOutputPort());
    shadowMapper->Update();
    shadow->SetMapper(shadowMapper);
    shadow->GetProperty()->SetColor(TRACKER_SHADOW_COLOR);
    shadow->GetProperty()->LightingOff();
  }

  // I'm using disabling localTransform, so I need to reimplement these
  virtual void getModelSpacePointInWorldCoordinates(const q_vec_type modelPoint,
                                                    q_vec_type worldCoordsOut)
      const
  {
    q_vec_type pos;
    q_type orient;
    getPosition(pos);
    getOrientation(orient);
    q_xform(worldCoordsOut, orient, modelPoint);
    q_vec_add(worldCoordsOut, pos, worldCoordsOut);
  }
  virtual void getWorldSpacePointInModelCoordinates(const q_vec_type worldPoint,
                                                    q_vec_type modelCoordsOut)
      const
  {
    q_vec_type pos;
    q_type orient;
    getPosition(pos);
    getOrientation(orient);
    q_invert(orient, orient);
    q_vec_invert(pos, pos);
    q_vec_add(modelCoordsOut, pos, worldPoint);
    q_xform(modelCoordsOut, orient, modelCoordsOut);
  }
  virtual void getWorldVectorInModelSpace(const q_vec_type worldVec,
                                          q_vec_type modelVecOut) const
  {
    q_type orient;
    getOrientation(orient);
    q_type orientInv;
    q_invert(orientInv, orient);
    q_xform(modelVecOut, orientInv, worldVec);
  }
  virtual void getModelVectorInWorldSpace(const q_vec_type modelVec,
                                          q_vec_type worldVecOut) const
  {
    q_type orient;
    getOrientation(orient);
    q_xform(worldVecOut, orient, modelVec);
  }

  virtual void setColorMapType(ColorMapType::Type) {}
  virtual void setArrayToColorBy(const QString&) {}
  virtual int numInstances() const { return 0; }
  virtual vtkActor* getActor() { return actor; }
  virtual vtkActor* getShadow() { return shadow; }
  vtkTransformPolyDataFilter* getTransformedGeometry()
  {
    return shadowGeometry;
  }
  virtual bool collide(SketchObject* other, PhysicsStrategy* physics,
                       int pqp_flags)
  {
    return false;
  }
  virtual void getBoundingBox(double bb[])
  {
    for (int i = 0; i < 6; ++i) {
      bb[i] = 0.0;
    }
  }
  virtual vtkPolyDataAlgorithm* getOrientedBoundingBoxes() { return NULL; }
  virtual vtkAlgorithm* getOrientedHalfPlaneOutlines() { return NULL; }
  virtual void setOrientedHalfPlaneData(double) {}
  virtual SketchObject* getCopy() { return NULL; }
  virtual SketchObject* deepCopy() { return NULL; }

  void updateShadowPosition(q_vec_type pos)
  {
    vtkTransform* t =
        vtkTransform::SafeDownCast(shadowGeometry->GetTransform());
    t->Identity();
    t->Scale(TRACKER_SHADOW_SCALE);
    t->Translate(pos);
  }

 private:
  vtkSmartPointer< vtkActor > actor;
  vtkSmartPointer< vtkActor > shadow;
  vtkSmartPointer< vtkTransformPolyDataFilter > shadowGeometry;
};

// ###########################################################################
// ###########################################################################
// ###########################################################################
// ###########################################################################
// The Pitchfork class - implements grabbing by connecting springs between
// tracker and object

// This class creates three springs attached from the tracker to the tracker.
// When impale is called with another object, it sets the second object of
// its springs to that object, locking the current relative position and
// orientation between the two objects.  When release is called, it sets the
// second object of the springs back to the tracker.  Since springs add no
// force if attached to the same object on each end, this removes all forces.

class Pitchfork
{
 public:
  // Creates a new Pitchfork with the given object as its base (the tracker),
  // which adds its springs to the given WorldManager as UI Springs
  Pitchfork(SketchObject& t, WorldManager* w) : tracker(t), worldMgr(w)
  {
    if (w != NULL) {
      init(w);
    }
  }
  ~Pitchfork()
  {
    for (int i = 0; i < 3; ++i) {
      worldMgr->removeUISpring(&springs[i]);
    }
  }
  void init(WorldManager* w)
  {
    worldMgr = w;
    q_vec_type oPos = {OBJECT_SIDE_LEN, 0, 0};
    q_vec_type tPos = {TRACKER_SIDE_LEN, 0, 0};
    q_type quat;
    q_from_axis_angle(quat, 0, 1, 0, 2 * Q_PI / 3);
    double length = Q_ABS(TRACKER_SIDE_LEN - OBJECT_SIDE_LEN);
    for (int i = 0; i < 3; ++i) {
      springs[i].initSpring(&tracker, &tracker, length, length,
                            OBJECT_GRAB_SPRING_CONST, oPos, tPos, false);
      worldMgr->addUISpring(&springs[i]);
      q_xform(oPos, quat, oPos);
      q_xform(tPos, quat, tPos);
    }
  }
  void impale(SketchObject* obj)
  {
    for (int i = 0; i < 3; ++i) {
      springs[i].setObject2(obj);
    }
  }
  void release()
  {
    q_vec_type oPos = {OBJECT_SIDE_LEN, 0, 0};
    q_type quat;
    q_from_axis_angle(quat, 0, 1, 0, 2 * Q_PI / 3);
    for (int i = 0; i < 3; ++i) {
      springs[i].setObject2(&tracker);
      springs[i].setObject2ConnectionPosition(oPos);
      q_xform(oPos, quat, oPos);
    }
  }

 private:
  SketchObject& tracker;
  WorldManager* worldMgr;
  SpringConnection springs[3];
};

// ###########################################################################
// ###########################################################################
// ###########################################################################
// ###########################################################################
// The HandImpl class - the private implementation of Hand

class Hand::HandImpl
{
 public:
  HandImpl(TransformManager* t, WorldManager* w, SketchBioHandId::Type s);

  void init(TransformManager* t, WorldManager* w, SketchBioHandId::Type s);

  void updatePositionAndOrientation();
  void getPosition(q_vec_type pos);
  void getOrientation(q_type orient);
  vtkActor* getHandActor();
  vtkActor* getShadowActor();

  SketchObject* getNearestObject();
  double getNearestObjectDistance();
  Connector* getNearestConnector(bool* atEnd1);
  double getNearestConnectorDistance();
  // Computes the nearest object and connector and the distances to them
  // Should be called every frame or at least before the getNearestX
  // functions are called
  void computeNearestObjectAndConnector();
  // Updates whatever is grabbed.  This function should be called
  // each frame
  void updateGrabbed();

  // The following functions grab and release the nearest object, nearest
  // connector or world
  void grabNearestObject();
  void grabNearestConnector();
  void grabWorld();
  void releaseGrabbed();

  // Increase and decrease the level within the group that is selected
  void selectSubObjectOfCurrent();
  void selectParentObjectOfCurrent();

  // Clears state for mode changes to avoid interference between modes'
  // operations
  void clearState();
  void clearNearestObject();
  void clearNearestConnector();

 private:
  enum GrabType {
    NOTHING_GRABBED,
    OBJECT_GRABBED,
    CONNECTOR_GRABBED,
    WORLD_GRABBED
  };

  TrackerObject tracker;
  Pitchfork pitchfork;

  GrabType grabType;
  SketchObject* nearestObject;
  double objectDistance;
  Connector* nearestConnector;
  bool isClosestToEnd1;
  double connectorDistance;

  SketchBioHandId::Type side;
  TransformManager* transformMgr;
  WorldManager* worldMgr;
};

Hand::HandImpl::HandImpl(TransformManager* t, WorldManager* w,
                         SketchBioHandId::Type s)
    : tracker(),
      pitchfork(tracker, w),
      grabType(NOTHING_GRABBED),
      nearestObject(NULL),
      objectDistance(std::numeric_limits< double >::max()),
      nearestConnector(NULL),
      isClosestToEnd1(false),
      connectorDistance(std::numeric_limits< double >::max()),
      side(s),
      transformMgr(t),
      worldMgr(w)
{
  if (t != NULL) {
    tracker.getShadow()->SetUserTransform(
        transformMgr->getRoomToWorldTransform());
  }
}

void Hand::HandImpl::init(TransformManager* t, WorldManager* w,
                          SketchBioHandId::Type s)
{
  side = s;
  transformMgr = t;
  worldMgr = w;
  pitchfork.init(worldMgr);
  tracker.getShadow()->SetUserTransform(
      transformMgr->getRoomToWorldTransform());
}

void Hand::HandImpl::updatePositionAndOrientation()
{
  q_vec_type pos;
  q_type orient;
  transformMgr->getTrackerPosInWorldCoords(pos, side);
  transformMgr->getTrackerOrientInWorldCoords(orient, side);
  tracker.setPosAndOrient(pos, orient);
  transformMgr->getTrackerPosInRoomCoords(pos, side);
  tracker.updateShadowPosition(pos);
  transformMgr->getTrackerTransformInEyeCoords(
      vtkTransform::SafeDownCast(tracker.getActor()->GetUserTransform()), side);
}

void Hand::HandImpl::getPosition(q_type pos) { tracker.getPosition(pos); }

void Hand::HandImpl::getOrientation(q_type orient)
{
  tracker.getOrientation(orient);
}

vtkActor* Hand::HandImpl::getHandActor() { return tracker.getActor(); }
vtkActor* Hand::HandImpl::getShadowActor() { return tracker.getShadow(); }

SketchObject* Hand::HandImpl::getNearestObject() { return nearestObject; }

double Hand::HandImpl::getNearestObjectDistance() { return objectDistance; }

Connector* Hand::HandImpl::getNearestConnector(bool* atEnd1)
{
  if (atEnd1 != NULL) {
    *atEnd1 = isClosestToEnd1;
  }
  return nearestConnector;
}

double Hand::HandImpl::getNearestConnectorDistance()
{
  return connectorDistance;
}

void Hand::HandImpl::computeNearestObjectAndConnector()
{
  q_vec_type pos;
  getPosition(pos);
  if (grabType == NOTHING_GRABBED && worldMgr->getNumberOfConnectors() > 0) {
    nearestConnector = worldMgr->getClosestConnector(pos, &connectorDistance,
                                                     &isClosestToEnd1);
  }
  if (grabType != OBJECT_GRABBED && grabType != WORLD_GRABBED &&
      worldMgr->getNumberOfObjects() > 0) {
    if (nearestObject == NULL || nearestObject->getParent() == NULL ||
        objectDistance >= DISTANCE_THRESHOLD) {
      nearestObject = worldMgr->getClosestObject(&tracker, objectDistance);
    } else {
      nearestObject = worldMgr->getClosestObject(
          *nearestObject->getParent()->getSubObjects(), &tracker,
          objectDistance);
    }
  }
}

void Hand::HandImpl::updateGrabbed()
{
  if (grabType == WORLD_GRABBED) {
    q_vec_type beforePos, afterPos;
    q_type beforeOrient, afterOrient;
    transformMgr->getTrackerPosInWorldCoords(afterPos, side);
    transformMgr->getOldTrackerPosInWorldCoords(beforePos, side);
    transformMgr->getTrackerOrientInWorldCoords(afterOrient, side);
    transformMgr->getOldTrackerOrientInWorldCoords(beforeOrient, side);
    // translate
    q_vec_type delta;
    q_vec_subtract(delta, afterPos, beforePos);
    transformMgr->translateWorldRelativeToRoom(delta);
    // rotate
    q_type inv, rotation;
    q_invert(inv, beforeOrient);
    q_mult(rotation, afterOrient, inv);
    transformMgr->rotateWorldRelativeToRoomAboutTracker(rotation, side);
  }
}

void Hand::HandImpl::grabNearestObject()
{
  if (grabType == NOTHING_GRABBED && nearestObject != NULL) {
    grabType = OBJECT_GRABBED;
    pitchfork.impale(nearestObject);
  }
}

void Hand::HandImpl::grabNearestConnector()
{
  if (grabType == NOTHING_GRABBED && nearestConnector != NULL) {
    grabType = CONNECTOR_GRABBED;
    q_vec_type nullVec = Q_NULL_VECTOR;
    if (isClosestToEnd1) {
      nearestConnector->setObject1(&tracker);
      nearestConnector->setObject1ConnectionPosition(nullVec);
    } else {
      nearestConnector->setObject2(&tracker);
      nearestConnector->setObject2ConnectionPosition(nullVec);
    }
  }
}

void Hand::HandImpl::grabWorld()
{
  if (grabType == NOTHING_GRABBED) {
    grabType = WORLD_GRABBED;
  }
}

void Hand::HandImpl::releaseGrabbed()
{
  switch (grabType) {
    case WORLD_GRABBED:
    case NOTHING_GRABBED:
      break;
    case OBJECT_GRABBED:
      pitchfork.release();
      break;
    case CONNECTOR_GRABBED:
      if (isClosestToEnd1) {
        nearestConnector->setObject1(
            (objectDistance < DISTANCE_THRESHOLD) ? nearestObject : NULL);
      } else {
        nearestConnector->setObject2(
            (objectDistance < DISTANCE_THRESHOLD) ? nearestObject : NULL);
      }
      break;
  }
  grabType = NOTHING_GRABBED;
}

void Hand::HandImpl::selectSubObjectOfCurrent()
{
  if (nearestObject != NULL) {
    QList< SketchObject* >* subObjects = nearestObject->getSubObjects();
    if (subObjects != NULL) {
      nearestObject =
          WorldManager::getClosestObject(*subObjects, &tracker, objectDistance);
    }
  }
}

void Hand::HandImpl::selectParentObjectOfCurrent()
{
  if (nearestObject != NULL) {
    SketchObject* parent = nearestObject->getParent();
    if (parent != NULL) {
      nearestObject = parent;
    }
  }
}

void Hand::HandImpl::clearState() { releaseGrabbed(); }

void Hand::HandImpl::clearNearestObject()
{
  if (grabType == OBJECT_GRABBED) {
    releaseGrabbed();
  }
  nearestObject = NULL;
  objectDistance = std::numeric_limits< double >::max();
}

void Hand::HandImpl::clearNearestConnector()
{
  if (grabType == CONNECTOR_GRABBED) {
    releaseGrabbed();
  }
  nearestConnector = NULL;
  isClosestToEnd1 = false;
  connectorDistance = std::numeric_limits< double >::max();
}

// ###########################################################################
// ###########################################################################
// ###########################################################################
// ###########################################################################
// Implementation of Hand : forward all function calls to HandImpl

Hand::Hand() : impl(new HandImpl(NULL, NULL, SketchBioHandId::LEFT)) {}

Hand::Hand(TransformManager* t, WorldManager* w, SketchBioHandId::Type s)
    : impl(new HandImpl(t, w, s))
{
}

Hand::~Hand() { delete impl; }

void Hand::init(TransformManager* t, WorldManager* w, SketchBioHandId::Type s)
{
  impl->init(t, w, s);
}

void Hand::updatePositionAndOrientation()
{
  impl->updatePositionAndOrientation();
}
void Hand::getPosition(q_vec_type pos) { impl->getPosition(pos); }
void Hand::getOrientation(q_type orient) { impl->getOrientation(orient); }
vtkActor* Hand::getHandActor() { return impl->getHandActor(); }
vtkActor* Hand::getShadowActor() { return impl->getShadowActor(); }
SketchObject* Hand::getNearestObject() { return impl->getNearestObject(); }
double Hand::getNearestObjectDistance()
{
  return impl->getNearestObjectDistance();
}
Connector* Hand::getNearestConnector(bool* atEnd1)
{
  return impl->getNearestConnector(atEnd1);
}
double Hand::getNearestConnectorDistance()
{
  return impl->getNearestConnectorDistance();
}
void Hand::computeNearestObjectAndConnector()
{
  impl->computeNearestObjectAndConnector();
}
void Hand::updateGrabbed() { impl->updateGrabbed(); }
void Hand::grabNearestObject() { impl->grabNearestObject(); }
void Hand::grabNearestConnector() { impl->grabNearestConnector(); }
void Hand::grabWorld() { impl->grabWorld(); }
void Hand::releaseGrabbed() { impl->releaseGrabbed(); }
void Hand::selectSubObjectOfCurrent() { impl->selectSubObjectOfCurrent(); }
void Hand::selectParentObjectOfCurrent()
{
  impl->selectParentObjectOfCurrent();
}
void Hand::clearState() { impl->clearState(); }
void Hand::clearNearestObject() { impl->clearNearestObject(); }
void Hand::clearNearestConnector() { impl->clearNearestConnector(); }

}  // end namespace
