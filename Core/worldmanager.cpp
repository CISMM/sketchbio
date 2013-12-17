#include "worldmanager.h"

#include <limits>
#include <iostream>
using std::cout;
using std::endl;

#include <vtkTubeFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkVersion.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkLineSource.h>
#include <vtkPolyData.h>
#include <vtkAppendPolyData.h>

#include <QDebug>

#include <vtkProjectToPlane.h>

#include "sketchtests.h"
#include "keyframe.h"
#include "sketchmodel.h"
#include "sketchobject.h"
#include "modelinstance.h"
#include "objectgroup.h"
#include "springconnection.h"
#include "physicsstrategy.h"

// The color used for the shadows of objects
#define SHADOW_COLOR 0.1,0.1,0.1
#define HALFPLANE_COLOR 0.9,0.3,0.3

//##################################################################################################
//##################################################################################################
WorldManager::WorldManager(vtkRenderer *r) :
    objects(),
    shadows(),
    lines(),
    connections(),
    lHand(),
    rHand(),
    strategies(),
    renderer(r),
    orientedHalfPlaneOutlines(vtkSmartPointer< vtkAppendPolyData >::New()),
    halfPlanesActor(vtkSmartPointer< vtkActor >::New()),
    maxGroupNum(0),
    doPhysicsSprings(true),
    doCollisionCheck(true),
    showInvisible(true),
    showShadows(true),
    collisionResponseMode(PhysicsMode::POSE_MODE_TRY_ONE)
{
    PhysicsStrategyFactory::populateStrategies(strategies);
    vtkSmartPointer< vtkPoints > pts =
            vtkSmartPointer< vtkPoints >::New();
    pts->InsertNextPoint(0.0,0.0,0.0);
    vtkSmartPointer< vtkPolyData > pdata =
            vtkSmartPointer< vtkPolyData >::New();
    pdata->SetPoints(pts);
    orientedHalfPlaneOutlines->AddInputData(pdata);
    vtkSmartPointer< vtkPolyDataMapper > orientedHalfPlanesMapper =
            vtkSmartPointer< vtkPolyDataMapper >::New();
    orientedHalfPlanesMapper->SetInputConnection(
                orientedHalfPlaneOutlines->GetOutputPort());
    orientedHalfPlanesMapper->Update();
    halfPlanesActor->SetMapper(orientedHalfPlanesMapper);
    halfPlanesActor->GetProperty()->SetColor(HALFPLANE_COLOR);
}

//##################################################################################################
//##################################################################################################
WorldManager::~WorldManager() {
    lines.clear();
    shadows.clear();
    qDeleteAll(connections);
    qDeleteAll(lHand);
    qDeleteAll(rHand);
    qDeleteAll(objects);
    connections.clear();
    lHand.clear();
    rHand.clear();
    objects.clear();
}

//##################################################################################################
//##################################################################################################
int WorldManager::getNextGroupId() {
    return ++maxGroupNum;
}

//##################################################################################################
//##################################################################################################
SketchObject *WorldManager::addObject(SketchModel *model,const q_vec_type pos, const q_type orient) {
    SketchObject *newObject = new ModelInstance(model);
    newObject->setPosAndOrient(pos,orient);
    return addObject(newObject);
}

//##################################################################################################
//##################################################################################################
SketchObject *WorldManager::addObject(SketchObject *object) {
    objects.push_back(object);
    if (object->getPrimaryCollisionGroupNum() == OBJECT_HAS_NO_GROUP) {
        object->setPrimaryCollisionGroupNum(getNextGroupId());
    }
    if (object->isVisible() || showInvisible) {
        insertActors(object);
    }
    object->addObserver(this);
    return object;
}

//##################################################################################################
//##################################################################################################
void WorldManager::removeObject(SketchObject *object) {
    // if we are deleting an object, it should not have any springs...
    // TODO - fix this assumption later
    int index = objects.indexOf(object);
    if (index != -1)
    {
        removeActors(object);
        removeShadows(object);
        objects.removeAt(index);
        object->removeObserver(this);
    }
    else if (object->getParent() != NULL)
    {
        // TODO - add test for this case where an object in a group is removed/deleted
        SketchObject *p = object->getParent(), *r = p;
        while (r->getParent() != NULL)
        {
            r = r->getParent();
        }
        ObjectGroup *grp = dynamic_cast<ObjectGroup *>(p);
        if (objects.contains(r) && grp != NULL)
        {
            grp->removeObject(object);
        }
    }
}
//##################################################################################################
//##################################################################################################
void WorldManager::deleteObject(SketchObject *object) {
    removeObject(object);
    delete object;
}

//##################################################################################################
//##################################################################################################
int WorldManager::getNumberOfObjects() const {
    return objects.size();
}

//##################################################################################################
//##################################################################################################
QListIterator<SketchObject *> WorldManager::getObjectIterator() const {
    return QListIterator<SketchObject *>(objects);
}

//##################################################################################################
//##################################################################################################
void WorldManager::clearObjects()
{
    while (!objects.empty())
    {
        deleteObject(objects[0]);
    }
    orientedHalfPlaneOutlines->RemoveAllInputConnections(0);
    vtkSmartPointer< vtkPoints > pts =
            vtkSmartPointer< vtkPoints >::New();
    pts->InsertNextPoint(0.0,0.0,0.0);
    vtkSmartPointer< vtkPolyData > pdata =
            vtkSmartPointer< vtkPolyData >::New();
    pdata->SetPoints(pts);
    orientedHalfPlaneOutlines->AddInputData(pdata);
    orientedHalfPlaneOutlines->Update();
}

//##################################################################################################
//##################################################################################################
Connector* WorldManager::addConnector(Connector* spring) {
    addConnector(spring,&connections);

    return spring;
}

//##################################################################################################
//##################################################################################################
void WorldManager::clearLeftHandSprings() {
    for (QMutableListIterator< Connector* > it(lHand); it.hasNext();) {
        Connector* spring = it.next();
        renderer->RemoveActor(lines.value(spring).second);
        lines.remove(spring);
        it.remove();
        delete spring;
    }
    lHand.clear();
}

//##################################################################################################
//##################################################################################################
void WorldManager::clearRightHandSprings() {
    for (QMutableListIterator< Connector* > it(rHand); it.hasNext();) {
        Connector* spring = it.next();
        renderer->RemoveActor(lines.value(spring).second);
        lines.remove(spring);
        it.remove();
        delete spring;
    }
    rHand.clear();
}

void WorldManager::clearConnectors()
{
    while (! connections.empty())
    {
        removeSpring(connections[0]);
    }
}

//##################################################################################################
//##################################################################################################
SpringConnection *WorldManager::addSpring(SketchObject *o1, SketchObject *o2,const q_vec_type pos1,
                             const q_vec_type pos2, bool worldRelativePos, double k, double minLen, double maxLen)
{
    SpringConnection *spring = SpringConnection::makeSpring(o1,o2,pos1,pos2,worldRelativePos,k,minLen,maxLen);
    addConnector(spring, &connections);
    return spring;
}

//##################################################################################################
//##################################################################################################
void WorldManager::removeSpring(Connector* spring) {
    int index = connections.indexOf(spring);
    assert(index >= 0); // if this fails fix the code that broke it
    connections.removeAt(index);

    renderer->RemoveActor(lines.value(spring).second);
    lines.remove(spring);

    delete spring;
}

//##################################################################################################
//##################################################################################################
int WorldManager::getNumberOfConnectors() const {
    return connections.size();
}


//##################################################################################################
//##################################################################################################
QListIterator< Connector* > WorldManager::getSpringsIterator() const {
    return QListIterator< Connector* >(connections);
}

//##################################################################################################
//##################################################################################################
void WorldManager::setCollisionMode(PhysicsMode::Type mode) {
    collisionResponseMode = mode;
}

//##################################################################################################
//##################################################################################################
void WorldManager::stepPhysics(double dt) {

    // clear the accumulated force in the objects
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject * obj = it.next();
        obj->clearForces();
        obj->setLastLocation();
    }
    strategies[collisionResponseMode]->performPhysicsStepAndCollisionDetection(
                lHand,rHand,connections,doPhysicsSprings,objects,dt,doCollisionCheck);

    updateConnectors();
}

//##################################################################################################
//##################################################################################################
bool WorldManager::setAnimationTime(double t) {
	bool isDone = true;
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        bool wasVisible = obj->isVisible();
        obj->setPositionByAnimationTime(t);
        if (obj->hasKeyframes()) {
            if (obj->getKeyframes()->upperBound(t) != obj->getKeyframes()->end()) {
                isDone = false;
            }
        }
        if (obj->isVisible() && !wasVisible) {
            insertActors(obj);
        } else if (!obj->isVisible() && wasVisible) {
            removeActors(obj);
        }
    }
    setKeyframeOutlinesForTime(t);
	updateConnectors();
    return isDone;
}
//##################################################################################################
//##################################################################################################
void WorldManager::setKeyframeOutlinesForTime(double t)
{
    int numberWithKeyframeNow = 0;
    bool isShowingHalfPlaneOutlines =
            orientedHalfPlaneOutlines->GetNumberOfInputConnections(0) > 1;
    orientedHalfPlaneOutlines->RemoveAllInputs();
    vtkSmartPointer< vtkPoints > pts =
            vtkSmartPointer< vtkPoints >::New();
    pts->InsertNextPoint(0.0,0.0,0.0);
    vtkSmartPointer< vtkPolyData > pdata =
            vtkSmartPointer< vtkPolyData >::New();
    pdata->SetPoints(pts);
    orientedHalfPlaneOutlines->AddInputData(pdata);
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        if (obj->hasKeyframes()) {
            if (obj->getKeyframes()->contains(t)) {
                orientedHalfPlaneOutlines->AddInputConnection(
                            0,obj->getOrientedHalfPlaneOutlines()->GetOutputPort(0));
                ++numberWithKeyframeNow;
            }
        }
    }
    if (isShowingHalfPlaneOutlines && numberWithKeyframeNow == 0)
    {
        renderer->RemoveActor(halfPlanesActor);
    }
    else if (!isShowingHalfPlaneOutlines && numberWithKeyframeNow > 0)
    {
        renderer->AddActor(halfPlanesActor);
    }
}

//##################################################################################################
//##################################################################################################
bool WorldManager::isShowingShadows() const
{
    return showShadows;
}

//##################################################################################################
//##################################################################################################
void WorldManager::setShowShadows(bool show)
{
    if (show == showShadows)
        return;
    showShadows = show;
    QHashIterator< SketchObject *, ShadowPair > itr(shadows);
    while (itr.hasNext())
    {
        vtkActor *actor = itr.next().value().second;
        if (show)
        {
            renderer->AddActor(actor);
        }
        else
        {
            renderer->RemoveActor(actor);
        }
    }
    if (show)
        renderer->AddActor(halfPlanesActor);
    else
        renderer->RemoveActor(halfPlanesActor);
}

//##################################################################################################
//##################################################################################################
void WorldManager::setShadowsOn()
{
    setShowShadows(true);
}

//##################################################################################################
//##################################################################################################
void WorldManager::setShadowsOff()
{
    setShowShadows(false);
}

//##################################################################################################
//##################################################################################################
void WorldManager::changedVisibility(SketchObject *obj)
{
    if (!showInvisible)
    {
        if (obj->isVisible())
            insertActors(obj);
        else
            removeActors(obj);
    }
}

//##################################################################################################
//##################################################################################################
bool WorldManager::isShowingInvisible() const
{
    return showInvisible;
}

//##################################################################################################
//##################################################################################################
void WorldManager::showInvisibleObjects() {
    showInvisible = true;
    for (int i = 0; i < objects.length(); i++) {
        if (!objects[i]->isVisible()) {
            insertActors(objects[i]);
        }
    }
    int n = connections.length();
    for (int i = 0; i < n; i++)
    {
        if (connections.at(i)->getAlpha() < Q_EPSILON)
        {
            renderer->AddActor(lines.value(connections[i]).second);
        }
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::hideInvisibleObjects() {
    showInvisible = false;
    for (int i = 0; i < objects.length(); i++) {
        if (!objects[i]->isVisible()) {
            removeActors(objects[i]);
        }
    }
    // TODO - transparent connectors
    int n = connections.length();
    for (int i = 0; i < n; i++)
    {
        if (connections.at(i)->getAlpha() < Q_EPSILON)
        {
            renderer->RemoveActor(lines.value(connections[i]).second);
        }
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::setPhysicsSpringsOn(bool on) {
    doPhysicsSprings = on;
}
//##################################################################################################
//##################################################################################################
void WorldManager::setCollisionCheckOn(bool on) {
    doCollisionCheck = on;
}

//##################################################################################################
//##################################################################################################
// helper function for updateSprings - updates the endpoints of the springs in the vtkPoints object
static inline void updateLines(
        QHash< Connector*, QPair< vtkSmartPointer< vtkLineSource >,
                                  vtkSmartPointer< vtkActor > > >& map)
{
    typedef QPair< vtkSmartPointer< vtkLineSource>,
                   vtkSmartPointer< vtkActor > > ConnectorPair;
    for (QHashIterator< Connector*, ConnectorPair > it(map);
         it.hasNext();)
    {
        Connector* s = it.next().key();
        s->updateLine();
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::updateConnectors() {
    // set spring ends to new positions
    updateLines(lines);
}


//##################################################################################################
//##################################################################################################
// if outside the bounding box returns an approximate distance to it
// else returns negative how far into the box the point as a fraction of the way through (.5 for halfway through)
inline double distOutsideAABB(q_vec_type point, double bb[6]) {
    if (bb[0] == bb[1] || bb[2] == bb[3] || bb[4] == bb[5]) {
        // if bounds have no volume, use distance between point and
        // bounds point
        q_vec_type bbPos = {bb[0], bb[2], bb[4]};
        return q_vec_distance(bbPos,point);
    }
    double xD, yD, zD, dist;
    bool inX = false, inY = false, inZ = false;
    if (point[Q_X] > bb[0] && point[Q_X] < bb[1]) {
        inX = true;
        xD = -min(point[Q_X]-bb[0],bb[1]-point[Q_X]);
    } else {
        xD = min(Q_ABS(bb[0]-point[Q_X]), Q_ABS(point[Q_X] - bb[1]));
    }
    if (point[Q_Y] > bb[2] && point[Q_Y] < bb[3]) {
        inY = true;
        yD = -min(point[Q_Y]-bb[2],bb[3]-point[Q_Y]);
    } else {
        yD = min(Q_ABS(bb[2]-point[Q_Y]),Q_ABS(point[Q_Y]-bb[3]));
    }
    if (point[Q_Z] > bb[4] && point[Q_Z] < bb[5]) {
        inZ = true;
        zD = -min(point[Q_Z]-bb[4],bb[5]-point[Q_Z]);
    } else {
        zD = min(Q_ABS(bb[4]-point[Q_Z]),Q_ABS(point[Q_Z]-bb[5]));
    }
    if (inX && inY && inZ) {
        // if you are in multiple objects, the smallest one wins, that way
        // selecting a little thing inside a big group is as easy as possible
        dist = -1.0 / ((bb[1]-bb[0])*(bb[3]-bb[2])*(bb[5]-bb[4]));
        // old method: whichever object you are closer to the center of (as a %
        // of the way through) wins... difficult to select small thing inside big
        // thing
        //dist = min(min(xD / (bb[1]-bb[0]),yD / (bb[3]-bb[2])),zD / (bb[5]-bb[4]));
    } else if (inX) {
        if (inY) {
            dist = zD;
        } else if (inZ) {
            dist = yD;
        } else {
            dist = max(yD,zD);
        }
    } else if (inY) {
        if (inZ) {
            dist = xD;
        } else {
            dist = max(xD,zD);
        }
    } else if (inZ) {
        dist = max(xD,yD);
    } else {
        dist = max(max(xD,yD),zD);
    }
    return dist;
}

//##################################################################################################
//##################################################################################################
SketchObject *WorldManager::getClosestObject(QList<SketchObject *> &objects,
                                             SketchObject *subj, double &distOut)
{
    SketchObject *closest = NULL;
    double distance = std::numeric_limits<double>::max();
    q_vec_type pos1;
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        double bb[6];
        double dist;
        subj->getPosition(pos1);
        if (obj->numInstances() == 1) {
            obj->getWorldSpacePointInModelCoordinates(pos1,pos1);
        }
        obj->getBoundingBox(bb);
        dist = distOutsideAABB(pos1,bb);
        if (dist < distance) {
            distance = dist;
            closest = obj;
        }
    }
    distOut = distance;
    return closest;
}

//##################################################################################################
//##################################################################################################
Connector* WorldManager::getClosestConnector(q_vec_type point, double *distOut,
                                                 bool *closerToEnd1) {
    Connector* closest = NULL;
    double distance = std::numeric_limits<double>::max();
    bool isCloserToEnd1 = false;
    for (QListIterator< Connector* > it(connections); it.hasNext();) {
        Connector* spr = it.next();
        q_vec_type pt1, pt2, vec;
        spr->getEnd1WorldPosition(pt1);
        spr->getEnd2WorldPosition(pt2);
        q_vec_subtract(vec,pt2,pt1);
        q_vec_type tmp1, tmp2, tmp3;
        double proj1;
        q_vec_subtract(tmp1,point,pt1);
        proj1 = q_vec_dot_product(tmp1,vec) / q_vec_dot_product(vec,vec);
        double dist;
        if (proj1 < 0) {
            dist = q_vec_magnitude(tmp1);
        } else if (proj1 > 1) {
            q_vec_subtract(tmp2,point,pt2);
            dist = q_vec_magnitude(tmp2);
        } else {
            q_vec_scale(tmp3,proj1,vec);
            q_vec_subtract(tmp3,tmp1,tmp3);
            dist = q_vec_magnitude(tmp3);
        }
        if (dist < distance) {
            distance = dist;
            closest = spr;
            isCloserToEnd1 = proj1 < .5;
        }
    }
    *distOut = distance;
    if (closerToEnd1) {
        *closerToEnd1 = isCloserToEnd1;
    }
    return closest;
}

//##################################################################################################
//##################################################################################################
void WorldManager::setShadowPlane(q_vec_type point, q_vec_type nVector)
{
    QHashIterator< SketchObject *, ShadowPair > itr(shadows);
    while (itr.hasNext())
    {
        SketchObject* obj = itr.peekNext().key();
        vtkProjectToPlane *filter = itr.next().value().first;
        vtkLinearTransform* trans = obj->getInverseLocalTransform();
        q_vec_type tpoint, tVector;
        trans->TransformVector(nVector,tVector);
        trans->TransformPoint(point,tpoint);
        filter->SetPointOnPlane(tpoint);
        filter->SetPlaneNormalVector(tVector);
        filter->Update();
    }
}
//##################################################################################################
//##################################################################################################
void WorldManager::subobjectAdded(SketchObject *parent, SketchObject *child)
{
    if (child->isVisible() || isShowingInvisible())
    {
        insertActors(child);
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::subobjectRemoved(SketchObject *parent, SketchObject *child)
{
    removeActors(child);
    removeShadows(child);
}

//##################################################################################################
//##################################################################################################
void WorldManager::addConnector(Connector* spring,QList< Connector* > *list) {
    list->push_back(spring);

    vtkLineSource* line = spring->getLine();
    vtkActor* actor = spring->getActor();
    if (line != NULL && actor != NULL)
    {
        if (spring->getAlpha() > Q_EPSILON || showInvisible)
        {
            renderer->AddActor(spring->getActor());
        }
        lines.insert(spring,ConnectorPair(line,actor));
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::insertActors(SketchObject *obj)
{
    if (obj->numInstances() == 1) {
        vtkSmartPointer<vtkActor> actor = obj->getActor();
        renderer->AddActor(actor);
        if (shadows.contains(obj))
        {
            renderer->AddActor(shadows.value(obj).second);
        }
        else
        {
            vtkSmartPointer< vtkProjectToPlane > projection =
                    vtkSmartPointer< vtkProjectToPlane >::New();
            SketchModel* model = obj->getModel();
            int confNum = obj->getModelConformation();
            projection->SetInputConnection(
                        model->getVTKSurface(confNum)->GetOutputPort());
            projection->SetPointOnPlane(0.0,0.0,0.0);
            projection->SetPlaneNormalVector(0.0,1.0,0.0);
            projection->Update();
            vtkSmartPointer< vtkPolyDataMapper > mapper =
                    vtkSmartPointer< vtkPolyDataMapper >::New();
            mapper->SetInputConnection(projection->GetOutputPort());
            mapper->Update();
            vtkSmartPointer< vtkActor > shadowActor =
                    vtkSmartPointer< vtkActor >::New();
            shadowActor->SetMapper(mapper);
            shadowActor->SetUserTransform(obj->getLocalTransform());
            shadowActor->GetProperty()->LightingOff();
            shadowActor->GetProperty()->SetColor(SHADOW_COLOR);
            shadows.insert(obj,ShadowPair(projection,shadowActor));
            renderer->AddActor(shadowActor);
        }
    }
    else
    {
        QList<SketchObject *> *children = obj->getSubObjects();
        if (children == NULL) return;
        for (int i = 0; i < children->length(); i++)
        {
            insertActors(children->at(i));
        }
    }
}


//##################################################################################################
//##################################################################################################
void WorldManager::removeActors(SketchObject *obj)
{
    if (obj->numInstances() == 1)
    {
        vtkSmartPointer<vtkActor> actor = obj->getActor();
        renderer->RemoveActor(actor);
        renderer->RemoveActor(shadows.value(obj).second);
    }
    else
    {
        QList<SketchObject *> *children = obj->getSubObjects();
        if (children == NULL) return;
        for (int i = 0; i < children->length(); i++)
        {
            removeActors(children->at(i));
        }
    }
}


//##################################################################################################
//##################################################################################################
void WorldManager::removeShadows(SketchObject *obj) {
    if (obj->numInstances() == 1)
    {
        shadows.remove(obj);
    }
    else
    {
        QList<SketchObject *> *children = obj->getSubObjects();
        if (children == NULL) return;
        for (int i = 0; i < children->length(); i++)
        {
            removeShadows(children->at(i));
        }
    }
}
