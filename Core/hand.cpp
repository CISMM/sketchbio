#include "hand.h"

#include <limits>

#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkConeSource.h>
#include <vtkLineSource.h>
#include <vtkExtractEdges.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

//#define DEBUG_TRACKER_ORIENTATIONS

#ifdef DEBUG_TRACKER_ORIENTATIONS
#include <vtkCubeSource.h>
#else
#include <vtkSphereSource.h>
#endif

#include <vtkProjectToPlane.h>

#include "objectchangeobserver.h"
#include "transformmanager.h"
#include "sketchobject.h"
#include "springconnection.h"
#include "worldmanager.h"

namespace SketchBio
{
// Color of outlines r=g=b= OUTLINES_COLOR_VAL
static const float OUTLINES_COLOR_VAL = 0.7;

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
static const double TRACKER_SHADOW_SCALE = 50 * 0.0625;
// the color of the tracker's shadows
#define TRACKER_SHADOW_COLOR 0.0, 0.0, 0.0

class TrackerObject : public SketchObject
{
   public:
    TrackerObject()
        : SketchObject(),
#ifndef DEBUG_TRACKER_ORIENTATIONS
          sphereSource(vtkSmartPointer< vtkSphereSource >::New()),
#else
          cubeSource(vtkSmartPointer< vtkCubeSource >::New()),
#endif
          shadowTransform(vtkSmartPointer< vtkTransform >::New()),
          actor(vtkSmartPointer< vtkActor >::New()),
          shadow(vtkSmartPointer< vtkActor >::New()),
          shadowGeometry(vtkSmartPointer< vtkTransformPolyDataFilter >::New())
    {
        double d = SCALE_DOWN_FACTOR * 4;
        vtkSmartPointer< vtkPolyDataMapper > mapper =
            vtkSmartPointer< vtkPolyDataMapper >::New();
#ifndef DEBUG_TRACKER_ORIENTATIONS
        sphereSource->SetRadius(d);
        sphereSource->Update();
        shadowGeometry->SetInputConnection(sphereSource->GetOutputPort());
        mapper->SetInputConnection(sphereSource->GetOutputPort());
#else
        cubeSource->SetBounds(-d, d, -2 * d, 2 * d, -3 * d, 3 * d);
        cubeSource->Update();
        shadowGeometry->SetInputConnection(cubeSource->GetOutputPort());
        mapper->SetInputConnection(cubeSource->GetOutputPort());
#endif
        shadowTransform->Identity();
        shadowTransform->PostMultiply();
        shadowTransform->Scale(TRACKER_SHADOW_SCALE,TRACKER_SHADOW_SCALE,
                               TRACKER_SHADOW_SCALE);
        shadowGeometry->SetTransform(shadowTransform);
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
        projection->SetPointOnPlane(
            0, PLANE_ROOM_Y + 0.1 * LINE_FROM_PLANE_OFFSET, 0);
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
    virtual void getModelSpacePointInWorldCoordinates(
        const q_vec_type modelPoint, q_vec_type worldCoordsOut) const
    {
        q_vec_type pos;
        q_type orient;
        getPosition(pos);
        getOrientation(orient);
        q_xform(worldCoordsOut, orient, modelPoint);
        q_vec_add(worldCoordsOut, pos, worldCoordsOut);
    }
    virtual void getWorldSpacePointInModelCoordinates(
        const q_vec_type worldPoint, q_vec_type modelCoordsOut) const
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
        vtkTransform* t = shadowTransform.GetPointer();
        t->Identity();
        t->Scale(TRACKER_SHADOW_SCALE,
                 TRACKER_SHADOW_SCALE,
                 TRACKER_SHADOW_SCALE);
        t->Translate(pos);
        t->Update();
    }

   private:
#ifndef DEBUG_TRACKER_ORIENTATIONS
    vtkSmartPointer< vtkSphereSource > sphereSource;
#else
    vtkSmartPointer< vtkCubeSource > cubeSource;
#endif
    vtkSmartPointer< vtkTransform > shadowTransform;
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

static const double OBJECT_SIDE_LEN = 200;
static const double TRACKER_SIDE_LEN = 200;
static const double OBJECT_GRAB_SPRING_CONST = 2;

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

class Hand::HandImpl : public ObjectChangeObserver
{
   public:
    HandImpl(TransformManager* t, WorldManager* w, SketchBioHandId::Type s,
             vtkRenderer* r);
    virtual ~HandImpl();

    void init(TransformManager* t, WorldManager* w, SketchBioHandId::Type s,
              vtkRenderer* r);

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
    bool hasGrabbedConnector();
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

    // Operations for the outlines for the things selected by the hand
    bool isShowingSelectionOutline();
    void setShowingSelectionOutline(bool show);
    void setSelectionType(OutlineType::Type type);

    // Make sure to outline correctly when something added/removed from group
    // overriding these from ObjectChangeObserver
    virtual void objectDeleted(SketchObject *obj);
    virtual void parentChanged(SketchObject *obj);
    virtual void subobjectAdded(SketchObject *parent, SketchObject *child);
    virtual void subobjectRemoved(SketchObject *parent, SketchObject *child);

   private:
    enum GrabType {
        NOTHING_GRABBED,
        OBJECT_GRABBED,
        CONNECTOR_GRABBED,
        WORLD_GRABBED
    };

    // the outlined object should always be the nearest object
    void outlineObject(SketchObject* obj);
    // the outlined connector should always be the nearest connector
    void outlineConnector(Connector* c, bool end1Large);
    void showOutline();
    void hideOutline();

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
    vtkSmartPointer< vtkRenderer > renderer;

    vtkSmartPointer< vtkPolyDataMapper > outlineMapper;
    vtkSmartPointer< vtkActor > outlineActor;
    bool showOutlineIfClose, isOutlineShown;
    OutlineType::Type outlineType;
};

Hand::HandImpl::HandImpl(TransformManager* t, WorldManager* w,
                         SketchBioHandId::Type s, vtkRenderer* r)
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
      worldMgr(w),
      renderer(r),
      outlineMapper(vtkSmartPointer< vtkPolyDataMapper >::New()),
      outlineActor(vtkSmartPointer< vtkActor >::New()),
      showOutlineIfClose(true),
      isOutlineShown(false),
      outlineType(OutlineType::OBJECTS)
{
    if (t != NULL) {
        tracker.getShadow()->SetUserTransform(
            transformMgr->getRoomToWorldTransform());
    }
    outlineActor->SetMapper(outlineMapper);
    outlineActor->GetProperty()->SetLighting(false);
    outlineActor->GetProperty()->SetColor(
        OUTLINES_COLOR_VAL, OUTLINES_COLOR_VAL, OUTLINES_COLOR_VAL);
}

Hand::HandImpl::~HandImpl() {
    if (nearestObject != NULL) {
        nearestObject->removeObserver(this);
    }
}

void Hand::HandImpl::init(TransformManager* t, WorldManager* w,
                          SketchBioHandId::Type s, vtkRenderer* r)
{
    side = s;
    transformMgr = t;
    worldMgr = w;
    renderer = r;
    pitchfork.init(worldMgr);
    tracker.getShadow()->SetUserTransform(
        transformMgr->getRoomToWorldTransform());
}

void Hand::HandImpl::updatePositionAndOrientation()
{
    assert(transformMgr != NULL);
    q_vec_type pos;
    q_type orient;
    transformMgr->getTrackerPosInWorldCoords(pos, side);
    transformMgr->getTrackerOrientInWorldCoords(orient, side);
    tracker.setPosAndOrient(pos, orient);
    transformMgr->getTrackerPosInRoomCoords(pos, side);
    tracker.updateShadowPosition(pos);
    transformMgr->getTrackerTransformInEyeCoords(
        vtkTransform::SafeDownCast(tracker.getActor()->GetUserTransform()),
        side);
}

void Hand::HandImpl::getPosition(q_type pos)
{
    assert(transformMgr != NULL);
    tracker.getPosition(pos);
}

void Hand::HandImpl::getOrientation(q_type orient)
{
    assert(transformMgr != NULL);
    tracker.getOrientation(orient);
}

vtkActor* Hand::HandImpl::getHandActor() { return tracker.getActor(); }
vtkActor* Hand::HandImpl::getShadowActor()
{
    assert(transformMgr != NULL);
    return tracker.getShadow();
}

SketchObject* Hand::HandImpl::getNearestObject()
{
    assert(transformMgr != NULL);
    return nearestObject;
}

double Hand::HandImpl::getNearestObjectDistance()
{
    assert(transformMgr != NULL);
    return objectDistance;
}

Connector* Hand::HandImpl::getNearestConnector(bool* atEnd1)
{
    assert(transformMgr != NULL);
    if (atEnd1 != NULL) {
        *atEnd1 = isClosestToEnd1;
    }
    return nearestConnector;
}

double Hand::HandImpl::getNearestConnectorDistance()
{
    assert(transformMgr != NULL);
    return connectorDistance;
}

void Hand::HandImpl::computeNearestObjectAndConnector()
{
    assert(transformMgr != NULL);
    q_vec_type pos;
    getPosition(pos);
    if (grabType == NOTHING_GRABBED && worldMgr->getNumberOfConnectors() > 0) {
        bool isCloseTo1;
        Connector* connector = worldMgr->getClosestConnector(
            pos, &connectorDistance, &isCloseTo1);
        if (OutlineType::CONNECTORS == outlineType) {
            if (connectorDistance < SPRING_DISTANCE_THRESHOLD) {
                outlineConnector(connector, isCloseTo1);
                showOutline();
            } else {
                hideOutline();
            }
        }
        nearestConnector = connector;
        isClosestToEnd1 = isCloseTo1;
    }
    if (grabType != OBJECT_GRABBED && grabType != WORLD_GRABBED &&
        worldMgr->getNumberOfObjects() > 0) {
        SketchObject* obj;
        q_vec_type trackerPos;
        getPosition(trackerPos);
        if (nearestObject == NULL || nearestObject->getParent() == NULL ||
            objectDistance >= DISTANCE_THRESHOLD) {
            obj = worldMgr->getClosestObject(trackerPos, objectDistance);
        } else {
            obj = worldMgr->getClosestObject(
                *nearestObject->getParent()->getSubObjects(), trackerPos,
                objectDistance);
        }
        if (obj != nearestObject) {
            if (nearestObject != NULL) {
                nearestObject->removeObserver(this);
            }
            if (obj != NULL) {
                obj->addObserver(this);
            }
            nearestObject = obj;
            if (OutlineType::OBJECTS == outlineType) {
                outlineObject(nearestObject);
            }
        }
        if (OutlineType::OBJECTS == outlineType) {
            if (objectDistance < DISTANCE_THRESHOLD) {
                showOutline();
            } else {
                hideOutline();
            }
        }
    }
}

void Hand::HandImpl::updateGrabbed()
{
    assert(transformMgr != NULL);
	if (grabType == WORLD_GRABBED || grabType == OBJECT_GRABBED) {
		q_vec_type beforePos, afterPos;
        q_type beforeOrient, afterOrient;
        transformMgr->getTrackerPosInWorldCoords(afterPos, side);
        transformMgr->getOldTrackerPosInWorldCoords(beforePos, side);
        transformMgr->getTrackerOrientInWorldCoords(afterOrient, side);
        transformMgr->getOldTrackerOrientInWorldCoords(beforeOrient, side);
        // compute translation
        q_vec_type delta;
        q_vec_subtract(delta, afterPos, beforePos);
		// compute rotation
		q_type inv, rotation;
		q_invert(inv, beforeOrient);
		q_mult(rotation, afterOrient, inv);
		
		if (grabType == WORLD_GRABBED) {
			// translate
			transformMgr->translateWorldRelativeToRoom(delta);
			// rotate
			transformMgr->rotateWorldRelativeToRoomAboutTracker(rotation, side);
		} else if (worldMgr->getCollisionMode() == PhysicsMode::POSE_MODE_TRY_ONE) {
			// object is grabbed and using pose mode physics
			// in Pose Mode 1, we use direct mapping instead of force-based motion
			q_vec_type objPosBefore, objPosAfter;
			q_type objOrientBefore, objOrientAfter;
			nearestObject->getPosition(objPosBefore);
			nearestObject->getOrientation(objOrientBefore);
			//translate
			q_vec_add(objPosAfter, objPosBefore, delta);
			//rotate
			q_mult(objOrientAfter, rotation, objOrientBefore);
			nearestObject->setLastLocation();
			SketchObject* parent = nearestObject->getParent();
			if (parent == NULL) {
				nearestObject->setPosAndOrient(objPosAfter, objOrientAfter);
			} else {
				nearestObject->setParentRelativePositionForAbsolutePosition(
					nearestObject, parent, objPosBefore, objPosAfter);
			}

			// update resolution of nearby objects 
			if (worldMgr->NearbyObjectsShouldUseFullRes()) {
				worldMgr->setNearbyObjectsToFullRes(nearestObject, 
													*worldMgr->getObjects());
			}
		}
    } else if (grabType == CONNECTOR_GRABBED) {
        outlineConnector(nearestConnector,isClosestToEnd1);
    }
}

void Hand::HandImpl::grabNearestObject()
{
    assert(transformMgr != NULL);
    if (grabType == NOTHING_GRABBED && nearestObject != NULL) {
        grabType = OBJECT_GRABBED;
		pitchfork.impale(nearestObject);
		nearestObject->setGrabbed(true);
		if (nearestObject->numInstances() == 1 && 
			worldMgr->GrabbedObjectsShouldUseFullRes()) 
		{
			nearestObject->showFullResolution();
		}
		if (worldMgr->NearbyObjectsShouldUseFullRes()) {
			worldMgr->setNearbyObjectsToFullRes(nearestObject, *worldMgr->getObjects());
		}
    }
}

void Hand::HandImpl::grabNearestConnector()
{
    assert(transformMgr != NULL);
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

bool Hand::HandImpl::hasGrabbedConnector() {
    return CONNECTOR_GRABBED == grabType;
}

void Hand::HandImpl::grabWorld()
{
    assert(transformMgr != NULL);
    if (grabType == NOTHING_GRABBED) {
        grabType = WORLD_GRABBED;
    }
}

void Hand::HandImpl::releaseGrabbed()
{
    assert(transformMgr != NULL);
    switch (grabType) {
        case WORLD_GRABBED:
        case NOTHING_GRABBED:
            break;
        case OBJECT_GRABBED:
            pitchfork.release();
			nearestObject->setGrabbed(false);
			nearestObject->hideFullResolution();
			worldMgr->setNearbyObjectsToPreviousResolution();
            break;
        case CONNECTOR_GRABBED:
            if (isClosestToEnd1) {
                nearestConnector->setObject1(
                    (objectDistance < DISTANCE_THRESHOLD) ? nearestObject
                                                          : NULL);
            } else {
                nearestConnector->setObject2(
                    (objectDistance < DISTANCE_THRESHOLD) ? nearestObject
                                                          : NULL);
            }
            outlineConnector(nearestConnector,isClosestToEnd1);
            break;
    }
    grabType = NOTHING_GRABBED;
}

void Hand::HandImpl::selectSubObjectOfCurrent()
{
    assert(transformMgr != NULL);
    if (nearestObject != NULL) {
        QList< SketchObject* >* subObjects = nearestObject->getSubObjects();
        if (subObjects != NULL) {
            q_vec_type trackerPos;
            getPosition(trackerPos);
            nearestObject->removeObserver(this);
            nearestObject = WorldManager::getClosestObject(
                *subObjects, trackerPos, objectDistance);
            nearestObject->addObserver(this);
            if (OutlineType::OBJECTS == outlineType) {
                outlineObject(nearestObject);
            }
        }
    }
}

void Hand::HandImpl::selectParentObjectOfCurrent()
{
    assert(transformMgr != NULL);
    if (nearestObject != NULL) {
        SketchObject* parent = nearestObject->getParent();
        if (parent != NULL) {
            nearestObject->removeObserver(this);
            nearestObject = parent;
            nearestObject->addObserver(this);
            if (OutlineType::OBJECTS == outlineType) {
                outlineObject(parent);
            }
        }
    }
}

void Hand::HandImpl::clearState()
{
    releaseGrabbed();
    hideOutline();
}

void Hand::HandImpl::clearNearestObject()
{
    if (grabType == OBJECT_GRABBED) {
        releaseGrabbed();
    }
    if (nearestObject != NULL) {
        nearestObject->removeObserver(this);
    }
    nearestObject = NULL;
    objectDistance = std::numeric_limits< double >::max();
    if (OutlineType::OBJECTS == outlineType) {
        hideOutline();
    }
}

void Hand::HandImpl::clearNearestConnector()
{
    if (grabType == CONNECTOR_GRABBED) {
        releaseGrabbed();
    }
    nearestConnector = NULL;
    isClosestToEnd1 = false;
    connectorDistance = std::numeric_limits< double >::max();
    if (OutlineType::CONNECTORS == outlineType) {
        hideOutline();
    }
}

bool Hand::HandImpl::isShowingSelectionOutline()
{
    assert(transformMgr != NULL);
    return showOutlineIfClose;
}

void Hand::HandImpl::setShowingSelectionOutline(bool show)
{
    assert(transformMgr != NULL);
    if (showOutlineIfClose != show) {
        showOutlineIfClose = show;
        if (OutlineType::OBJECTS == outlineType &&
            objectDistance < DISTANCE_THRESHOLD) {
            showOutline();
        } else if (OutlineType::CONNECTORS == outlineType &&
                   connectorDistance < SPRING_DISTANCE_THRESHOLD) {
            showOutline();
        } else {
            hideOutline();
        }
    }
}

void Hand::HandImpl::setSelectionType(OutlineType::Type type)
{
    assert(transformMgr != NULL);
    if (outlineType != type) {
        outlineType = type;
        if (OutlineType::OBJECTS == outlineType) {
            outlineObject(nearestObject);
            if (objectDistance < DISTANCE_THRESHOLD) {
                showOutline();
            } else {
                hideOutline();
            }
        } else {
            assert(OutlineType::CONNECTORS == outlineType);
            outlineConnector(nearestConnector, isClosestToEnd1);
            if (connectorDistance < SPRING_DISTANCE_THRESHOLD) {
                showOutline();
            } else {
                hideOutline();
            }
        }
    }
}

void Hand::HandImpl::outlineObject(SketchObject* obj)
{
    assert(transformMgr != NULL);
    if (obj == NULL) {
        hideOutline();
        return;
    }
    outlineMapper->SetInputConnection(
        obj->getOrientedBoundingBoxes()->GetOutputPort());
    outlineMapper->Update();
    // TODO - fix this hack color stuff
    if (obj->getParent() != NULL) {
        outlineActor->GetProperty()->SetColor(1, 0, 0);
    } else {
        outlineActor->GetProperty()->SetColor(
            OUTLINES_COLOR_VAL, OUTLINES_COLOR_VAL, OUTLINES_COLOR_VAL);
    }
}

void Hand::HandImpl::outlineConnector(Connector* c, bool end1Large)
{
    assert(transformMgr != NULL);
    if (c == NULL) {
        hideOutline();
        return;
    }
    q_vec_type end1, end2, midpoint, direction;
    c->getEnd1WorldPosition(end1);
    c->getEnd2WorldPosition(end2);
    q_vec_add(midpoint, end1, end2);
    q_vec_scale(midpoint, .5, midpoint);
    q_vec_subtract(direction, end2, end1);
    double length = q_vec_magnitude(direction);
    if (!end1Large) {
        q_vec_invert(direction, direction);
    }
    vtkSmartPointer< vtkConeSource > cone =
        vtkSmartPointer< vtkConeSource >::New();
    cone->SetCenter(midpoint);
    cone->SetDirection(direction);
    cone->SetHeight(length);
    cone->SetRadius(50);
    cone->SetResolution(4);
    cone->Update();

    vtkSmartPointer< vtkExtractEdges > edges =
        vtkSmartPointer< vtkExtractEdges >::New();
    edges->SetInputConnection(cone->GetOutputPort());
    edges->Update();

    outlineMapper->SetInputConnection(edges->GetOutputPort());
    outlineMapper->Update();

    outlineActor->GetProperty()->SetColor(
        OUTLINES_COLOR_VAL, OUTLINES_COLOR_VAL, OUTLINES_COLOR_VAL);
}

void Hand::HandImpl::showOutline()
{
    assert(transformMgr != NULL);
    if (!isOutlineShown && showOutlineIfClose) {
        isOutlineShown = true;
        renderer->AddActor(outlineActor);
    }
}

void Hand::HandImpl::hideOutline()
{
    assert(transformMgr != NULL);
    if (isOutlineShown) {
        isOutlineShown = false;
        renderer->RemoveViewProp(outlineActor);
    }
}

void Hand::HandImpl::objectDeleted(SketchObject *obj) {
    assert(obj == nearestObject);
    // if the selected object was deleted, clear the selection
    clearNearestObject();
}

void Hand::HandImpl::parentChanged(SketchObject *obj) {
    assert(obj == nearestObject);
    // If the object selected was added to a group, default to selecting
    // the whole group
    if (obj->getParent() != NULL) {
        selectParentObjectOfCurrent();
    } else {
        // else it was removed from a group, fix the outline color
        outlineObject(nearestObject);
    }
}
void Hand::HandImpl::subobjectAdded(SketchObject *parent, SketchObject *child) {
    assert(parent == nearestObject);
    // we will only ever be observing the nearest object.  If it changed, re-outline it
    outlineObject(nearestObject);
}
void Hand::HandImpl::subobjectRemoved(SketchObject *parent, SketchObject *child) {
    assert(parent == nearestObject);
    // we will only ever be observing the nearest object.  If it changed, re-outline it
    outlineObject(nearestObject);
}

// ###########################################################################
// ###########################################################################
// ###########################################################################
// ###########################################################################
// Implementation of Hand : forward all function calls to HandImpl

Hand::Hand() : impl(new HandImpl(NULL, NULL, SketchBioHandId::LEFT, NULL)) {}

Hand::Hand(TransformManager* t, WorldManager* w, SketchBioHandId::Type s,
           vtkRenderer* r)
    : impl(new HandImpl(t, w, s, r))
{
}

Hand::~Hand() { delete impl; }

void Hand::init(TransformManager* t, WorldManager* w, SketchBioHandId::Type s,
                vtkRenderer* r)
{
    impl->init(t, w, s, r);
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
bool Hand::hasGrabbedConnector() { return impl->hasGrabbedConnector(); }
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

bool Hand::isShowingSelectionOutline()
{
    return impl->isShowingSelectionOutline();
}
void Hand::setShowingSelectionOutline(bool show)
{
    impl->setShowingSelectionOutline(show);
}
void Hand::setSelectionType(OutlineType::Type type)
{
    impl->setSelectionType(type);
}

}  // end namespace
