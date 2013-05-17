#include "worldmanager.h"
#include <sketchtests.h>
#include <vtkTubeFilter.h>
#include <QDebug>
#include <limits>

#define COLLISION_FORCE 5

//##################################################################################################
//##################################################################################################
WorldManager::WorldManager(vtkRenderer *r, vtkTransform *worldEyeTransform) :
    objects(),
    connections(),
    lHand(),
    rHand(),
    strategies(),
    maxGroupNum(0)
{
    CollisionMode::populateStrategies(strategies);
    renderer = r;
    doPhysicsSprings = true;
    doCollisionCheck = true;
    showInvisible = true;
    collisionResponseMode = CollisionMode::POSE_MODE_TRY_ONE;
    nextIdx = 0;
    lastCapacityUpdate = 1000;
    springEnds = vtkSmartPointer<vtkPoints>::New();
    springEnds->Allocate(lastCapacityUpdate*2);
    springEndConnections = vtkSmartPointer<vtkPolyData>::New();
    springEndConnections->Allocate();
    springEndConnections->SetPoints(springEnds);
    tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    tubeFilter->SetInput(springEndConnections);
    tubeFilter->SetRadius(5);
    tubeFilter->Update();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(tubeFilter->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
    this->worldEyeTransform = worldEyeTransform;
}

//##################################################################################################
//##################################################################################################
WorldManager::~WorldManager() {
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
    return object;
}

//##################################################################################################
//##################################################################################################
void WorldManager::removeObject(SketchObject *object) {
    // if we are deleting an object, it should not have any springs...
    // TODO - fix this assumption later
    int index = objects.indexOf(object);
    removeActors(object);
    objects.removeAt(index);
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
SpringConnection *WorldManager::addSpring(SpringConnection *spring) {
    addSpring(spring,&connections);

    return spring;
}

void WorldManager::clearLeftHandSprings() {
    for (QMutableListIterator<SpringConnection *> it(lHand); it.hasNext();) {
        SpringConnection *spring = it.next();
        springEndConnections->DeleteCell(spring->getCellId());
        it.setValue((SpringConnection *)NULL);
        delete spring;
    }
    lHand.clear();
}

void WorldManager::clearRightHandSprings() {
    for (QMutableListIterator<SpringConnection *> it(rHand); it.hasNext();) {
        SpringConnection *spring = it.next();
        springEndConnections->DeleteCell(spring->getCellId());
        it.setValue((SpringConnection *)NULL);
        delete spring;
    }
    rHand.clear();
}

//##################################################################################################
//##################################################################################################
SpringConnection *WorldManager::addSpring(SketchObject *o1, SketchObject *o2,const q_vec_type pos1,
                             const q_vec_type pos2, bool worldRelativePos, double k, double minLen, double maxLen)
{
    SpringConnection *spring = InterObjectSpring::makeSpring(o1,o2,pos1,pos2,worldRelativePos,k,minLen,maxLen);
    addSpring(spring, &connections);
    return spring;
}

//##################################################################################################
//##################################################################################################
void WorldManager::removeSpring(SpringConnection *spring) {
    int index = connections.indexOf(spring);
    connections.removeAt(index);

    springEndConnections->DeleteCell(spring->getCellId());
    // can't delete points...

    delete spring;
}

//##################################################################################################
//##################################################################################################
int WorldManager::getNumberOfSprings() const {
    return connections.size();
}


//##################################################################################################
//##################################################################################################
QListIterator<SpringConnection *> WorldManager::getSpringsIterator() const {
    return QListIterator<SpringConnection *>(connections);
}

//##################################################################################################
//##################################################################################################
void WorldManager::setCollisionMode(CollisionMode::Type mode) {
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

    updateSprings();
}

//##################################################################################################
//##################################################################################################
bool WorldManager::setAnimationTime(double t) {
    bool isDone = true;
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        obj->setPositionByAnimationTime(t);
        bool wasVisible = obj->isVisible();
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
    return isDone;
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
inline void updatePoints(QList<SpringConnection *> &list, vtkPoints *pts) {
    for (QListIterator<SpringConnection *> it(list); it.hasNext();) {
        SpringConnection *s = it.next();
        q_vec_type pos1,pos2;
        s->getEnd1WorldPosition(pos1);
        pts->SetPoint(s->getEnd1Id(),pos1);
        s->getEnd2WorldPosition(pos2);
        pts->SetPoint(s->getEnd2Id(),pos2);
    }
}
//##################################################################################################
//##################################################################################################
// helper function for updateSprings - adds in cells to the vtkPolyData for each spring a line-type
//   cell is added
inline void addSpringCells(QList<SpringConnection *> &list, vtkPolyData *data) {
    for (QListIterator<SpringConnection *> it(list); it.hasNext();) {
        SpringConnection *s = it.next();
        vtkIdType pts[2];
        pts[0] = s->getEnd1Id();
        pts[1] = s->getEnd2Id();
        vtkIdType cellId = data->InsertNextCell(VTK_LINE,2,pts);
        s->setCellId(cellId);
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::updateSprings() {
    // set spring ends to new positions
    updatePoints(connections,springEnds);
    updatePoints(lHand,springEnds);
    updatePoints(rHand,springEnds);
    springEnds->Modified();
    int num = springEndConnections->GetNumberOfLines(), num2;
    springEndConnections->RemoveDeletedCells();
    if (num != (num2 = springEndConnections->GetNumberOfLines())) {
        for (vtkIdType i = 0; i < num2; i++) {
            springEndConnections->DeleteCell(i);
        }
        springEndConnections->RemoveDeletedCells();
        addSpringCells(connections,springEndConnections);
        addSpringCells(lHand,springEndConnections);
        addSpringCells(rHand,springEndConnections);
        springEndConnections->Modified();
        springEndConnections->Update();
        tubeFilter->Update();
    }
    springEndConnections->Update();
}


//##################################################################################################
//##################################################################################################
// if outside the bounding box returns an approximate distance to it
// else returns negative how far into the box the point as a fraction of the way through (.5 for halfway through)
inline double distOutsideAABB(q_vec_type point, double bb[6]) {
    if (bb[0] == bb[1] || bb[2] == bb[3] || bb[4] == bb[5]) {
        qDebug() << "Error: Bounding box has no volume...";
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
        dist = min(min(xD / (bb[1]-bb[0]),yD / (bb[3]-bb[2])),zD / (bb[5]-bb[4]));
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
SketchObject *WorldManager::getClosestObject(SketchObject *subj, double *distOut) {
    SketchObject *closest = NULL;
    double distance = std::numeric_limits<double>::max();
    q_vec_type pos1;
    subj->getPosition(pos1);
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        double bb[6];
        double dist;
        if (obj->numInstances() == 1) {
            obj->getWorldSpacePointInModelCoordinates(pos1,pos1);
        }
        obj->getAABoundingBox(bb);
        dist = distOutsideAABB(pos1,bb);
        if (dist < distance) {
            distance = dist;
            closest = obj;
        }
    }
    *distOut = distance;
    return closest;
}

//##################################################################################################
//##################################################################################################
SpringConnection *WorldManager::getClosestSpring(q_vec_type point, double *distOut,
                                                 bool *closerToEnd1) {
    SpringConnection *closest = NULL;
    double distance = std::numeric_limits<double>::max();
    bool isCloserToEnd1 = false;
    for (QListIterator<SpringConnection *> it(connections); it.hasNext();) {
        SpringConnection *spr = it.next();
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
void WorldManager::addSpring(SpringConnection *spring,QList<SpringConnection *> *list) {
    list->push_back(spring);

    // code to draw spring as a line
    if (springEndConnections->GetNumberOfCells() > lastCapacityUpdate) {
        vtkIdType newCapacity = springEndConnections->GetNumberOfCells();
        springEnds->Allocate(newCapacity*4);
        springEndConnections->Allocate(newCapacity*2);
        lastCapacityUpdate = newCapacity *2;
    }
    q_vec_type endOfSpring;
    spring->getEnd1WorldPosition(endOfSpring);
    vtkIdType end1Id = springEnds->InsertNextPoint(endOfSpring);
    spring->setEnd1Id(end1Id);
    spring->getEnd2WorldPosition(endOfSpring);
    vtkIdType end2Id = springEnds->InsertNextPoint(endOfSpring);
    spring->setEnd2Id(end2Id);
//    springEnds->Modified();
    vtkIdType pts[2] = { end1Id, end2Id };
    vtkIdType cellId = springEndConnections->InsertNextCell(VTK_LINE,2,pts);
//    springEndConnections->Update();
    spring->setCellId(cellId);
}

//##################################################################################################
//##################################################################################################
void WorldManager::insertActors(SketchObject *obj) {
    if (obj->numInstances() == 1) {
        vtkSmartPointer<vtkActor> actor = obj->getActor();
        renderer->AddActor(actor);
    } else {
        QList<SketchObject *> *children = obj->getSubObjects();
        if (children == NULL) return;
        for (int i = 0; i < children->length(); i++) {
            insertActors(children->at(i));
        }
    }
}


//##################################################################################################
//##################################################################################################
void WorldManager::removeActors(SketchObject *obj) {
    if (obj->numInstances() == 1) {
        vtkSmartPointer<vtkActor> actor = obj->getActor();
        renderer->RemoveActor(actor);
    } else {
        QList<SketchObject *> *children = obj->getSubObjects();
        if (children == NULL) return;
        for (int i = 0; i < children->length(); i++) {
            removeActors(children->at(i));
        }
    }
}
