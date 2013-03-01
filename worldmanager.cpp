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
    maxGroupNum(0)
{
    renderer = r;
    doPhysicsSprings = true;
    doCollisionCheck = true;
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
    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
    trans->PostMultiply();
    trans->Identity();
    trans->Concatenate(worldEyeTransform);
    actor->SetUserTransform(trans);
    renderer->AddActor(actor);
    this->worldEyeTransform = worldEyeTransform;
}

//##################################################################################################
//##################################################################################################
WorldManager::~WorldManager() {
    for (QMutableListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        it.setValue((SketchObject *)NULL);
        delete obj;
    }
    for (QMutableListIterator<SpringConnection *> it(connections); it.hasNext();) {
        SpringConnection *spring = it.next();
        it.setValue((SpringConnection *)NULL);
        delete spring;
    }
    clearLeftHandSprings();
    clearRightHandSprings();
    objects.clear();
    connections.clear();
}

//##################################################################################################
//##################################################################################################
SketchObject *WorldManager::addObject(SketchModel *model,const q_vec_type pos, const q_type orient) {
    vtkSmartPointer<vtkActor> actor;
    SketchObject *newObject = new ModelInstance(model);
    actor = newObject->getActor();
    actor->SetUserTransform(worldEyeTransform);
    newObject->setPosAndOrient(pos,orient);
    newObject->setPrimaryCollisionGroupNum(maxGroupNum++);
    renderer->AddActor(actor);
    objects.push_back(newObject);
    return newObject;
}

//##################################################################################################
//##################################################################################################
SketchObject *WorldManager::addObject(SketchObject *object) {
    objects.push_back(object);
    if (object->getPrimaryCollisionGroupNum() == OBJECT_HAS_NO_GROUP) {
        object->setPrimaryCollisionGroupNum(maxGroupNum++);
    }
    renderer->AddActor(object->getActor());
    return object;
}

//##################################################################################################
//##################################################################################################
void WorldManager::removeObject(SketchObject *object) {
    // if we are deleting an object, it should not have any springs...
    // TODO - fix this assumption later
    int index = objects.indexOf(object);
    renderer->RemoveActor(object->getActor());
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
// helper method for stepPhysics -- does Euler's method once force
// and torque on an object have been calculated
inline void euler(SketchObject *obj, double dt) {
    q_vec_type pos, angularVel,deltaPos,force,torque;
    q_type spin, orient;
//    if (obj->doPhysics()) {
        SketchModel *model = obj->getModel();
        // get force & torque
        obj->getForce(force);
        obj->getTorque(torque);
        // new position
        obj->getPosition(pos);
        q_vec_scale(deltaPos,dt*model->getInverseMass(),force);
        q_vec_add(pos,pos,deltaPos);
        // new orientation
        obj->getOrientation(orient);
        q_vec_scale(angularVel,model->getInverseMomentOfInertia(),torque);
        // I'll summarize this next section like this:
        // quaternion integration is wierd
        spin[Q_W] = 0;
        spin[Q_X] = angularVel[Q_X];
        spin[Q_Y] = angularVel[Q_Y];
        spin[Q_Z] = angularVel[Q_Z];
        q_mult(spin,orient,spin);
        orient[Q_W] += spin[Q_W]*.5*dt;
        orient[Q_X] += spin[Q_X]*.5*dt;
        orient[Q_Y] += spin[Q_Y]*.5*dt;
        orient[Q_Z] += spin[Q_Z]*.5*dt;
        q_normalize(orient,orient);
        // end quaternion integration
        obj->setPosAndOrient(pos,orient);
//    }
}

//##################################################################################################
//##################################################################################################
// -helper function to add spring forces from a list of springs to the objects the springs are
//   attached to.  Does not actually update object position
inline void springForcesFromList(QList<SpringConnection *> &list, QSet<int> &affectedGroups) {
    for (QListIterator<SpringConnection *> it(list); it.hasNext();) {
        SpringConnection *c = it.next();
        InterObjectSpring *cp = dynamic_cast<InterObjectSpring *>(c);
        c->addForce();
        int i = c->getObject1()->getPrimaryCollisionGroupNum();
        if (i != OBJECT_HAS_NO_GROUP) { // if this one isn't the tracker
            affectedGroups.insert(i);
        }
        if (cp != NULL && cp->getObject2()->getPrimaryCollisionGroupNum() != OBJECT_HAS_NO_GROUP) {
            affectedGroups.insert(cp->getObject2()->getPrimaryCollisionGroupNum());
        }
    }
}
//##################################################################################################
//##################################################################################################
// -helper function that loops over all objects and updates their locations based on current
//   forces on them
inline void applyEuler(QList<SketchObject *> &list, double dt, bool clearForces = true) {
    int n = list.size();
    for (int i = 0; i < n; i++) {
        SketchObject *obj = list.at(i);
        euler(obj,dt);
        if (clearForces) {
            obj->clearForces();
        }
    }
}

//##################################################################################################
//##################################################################################################
// -helper function that runs the double loop of collision detection over list with only objects
//   from the affected groups selected as the first object in the collision detection.  Does collision
//   response only if the last parameter is set.  Returns true if a collision was found anywhere, false
//   otherwise
// note - if affectedGroups is empty, this does full n^2 collision detection and response over all objects
//         (non pose-mode)

inline bool collideAndRespond(QList<SketchObject *> &list, QSet<int> &affectedGroups, double dt,
                              bool respond, bool usePCAResponse = false) {
    int n = list.size();
    bool foundCollision = false;
    for (int i = 0; i < n; i++) {
        // TODO - self collision once deformation added
        bool needsTest = affectedGroups.empty();
        for (QSetIterator<int> it(affectedGroups); it.hasNext();) {
            if (list.at(i)->isInCollisionGroup(it.next())) {
                needsTest = true;
            }
        }
        if (needsTest) {
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    PQP_CollideResult *cr = NULL;
                    cr = list.at(i)->collide(list.at(j), respond ? PQP_ALL_CONTACTS : PQP_FIRST_CONTACT);
                    if (cr->NumPairs() != 0) {
                        if (respond) {
                            if (usePCAResponse) {
                                WorldManager::applyPCACollisionResponseForce(
                                            list.at(i),list.at(j),cr,affectedGroups);
                            } else {
                                WorldManager::applyCollisionResponseForce(
                                            list.at(i),list.at(j),cr,affectedGroups);
                            }
                        }
                        foundCollision = true;
                    }
                    if (cr != NULL) {
                        delete cr;
                    }
                }
            }
        }
    }
    if (foundCollision && respond) {
        applyEuler(list,dt);
    }
    return foundCollision;
}

//##################################################################################################
//##################################################################################################
// -helper function that does what its name says, applies pose-mode style collision response to the
//   objects.  This means that the objects are tested for collisions, then respond, then are tested again.
//   If the collision response did not fix the collision, then the entire movement (including changes
//   from before this) is undone.
inline void applyPoseModeCollisionResponse(QList<SketchObject *> &list, QSet<int> &affectedGroups,
                                           double dt,bool usePCAResponse = false) {
    int n = list.size();
    bool appliedResponse = collideAndRespond(list,affectedGroups,dt,true,usePCAResponse);
    if (appliedResponse) {
        bool stillColliding = collideAndRespond(list,affectedGroups,dt,false);
        if (stillColliding) {
            // if we couldn't fix collisions, undo the motion and return
            for (int i = 0; i < n; i++) {
                list.at(i)->restoreToLastLocation();
            }
            return;
        }
    }
    // if we fixed the collision or there were none, finalize new positions
    for (int i = 0; i < n; i++) {
        list.at(i)->setLastLocation();
    }
}

inline void divideForces(QList<SketchObject *> &list,double divisor) {
    for (QMutableListIterator<SketchObject *> it(list); it.hasNext();) {
        q_vec_type f, t;
        SketchObject *obj = it.next();
        obj->getForce(f);
        obj->getTorque(t);
        q_vec_scale(f,divisor,f);
        q_vec_scale(t,divisor,t);
        obj->setForceAndTorque(f,t);
    }
}

//##################################################################################################
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does binary
//   collision search on the force/torque applied
// -assumes that applyEuler has NOT been called on the list, but the forces have been added to each
//   object
inline void applyBinaryCollisionSearch(QList<SketchObject *> &list, QSet<int> &affectedGroups,
                                       double dt, bool testCollisions) {
    applyEuler(list,dt,false);
    if (testCollisions) {
        int times = 1;
        while (collideAndRespond(list,affectedGroups,dt,false) && times < 10) {
            for (int i = 0; i < list.size(); i++) {
                list.at(i)->restoreToLastLocation();
            }
            divideForces(list,.5);
            applyEuler(list,dt,false);
            times++;
        }
    }
    for (int i = 0; i < list.size(); i++) {
        list.at(i)->setLastLocation();
    }
}

//##################################################################################################
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//   collision response on them.
inline void poseModeForSprings(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                               double dt, bool doCollisionCheck) {
    QSet<int> affectedGroups;
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyEuler(objs,dt);
        if (doCollisionCheck)
            applyPoseModeCollisionResponse(objs,affectedGroups,dt);
    }
}

//##################################################################################################
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//  pca collision response on them.
inline void poseModePCAForSprings(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                               double dt, bool doCollisionCheck) {
    QSet<int> affectedGroups;
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyEuler(objs,dt);
        if (doCollisionCheck)
            applyPoseModeCollisionResponse(objs,affectedGroups,dt,true);
    }
}

//##################################################################################################
//##################################################################################################
inline void binaryCollisionSearch(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                                  double dt, bool doCollisionCheck) {
    QSet<int> affectedGroups;
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyBinaryCollisionSearch(objs,affectedGroups,dt,doCollisionCheck);
    }
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
    if (collisionResponseMode == CollisionMode::POSE_MODE_TRY_ONE) { // pose mode
        // spring forces for right hand interacton
        poseModeForSprings(rHand,objects,dt,doCollisionCheck);
        // spring forces for left hand interaction
        poseModeForSprings(lHand,objects,dt,doCollisionCheck);

        // spring forces for physics springs interaction
        if (doPhysicsSprings) {
            poseModeForSprings(connections,objects,dt,doCollisionCheck);
        }
    } else if (collisionResponseMode == CollisionMode::ORIGINAL_COLLISION_RESPONSE) { // non-pose-mode
        QSet<int> affectedGroups;
        springForcesFromList(rHand,affectedGroups);
        springForcesFromList(lHand,affectedGroups);
        if (doPhysicsSprings)
            springForcesFromList(connections,affectedGroups);
        applyEuler(objects,dt);
        affectedGroups.clear(); // if passed an empty set, it does full collision checks
        if (doCollisionCheck)
            collideAndRespond(objects,affectedGroups,dt,true); // calculates collision response
        applyEuler(objects,dt);
    } else if (collisionResponseMode == CollisionMode::BINARY_COLLISION_SEARCH) {
        binaryCollisionSearch(rHand,objects,dt,doCollisionCheck);
        binaryCollisionSearch(lHand,objects,dt,doCollisionCheck);
        if (doPhysicsSprings) {
            binaryCollisionSearch(connections,objects,dt,doCollisionCheck);
        }
    } else if (collisionResponseMode == CollisionMode::POSE_WITH_PCA_COLLISION_RESPONSE) {
        poseModePCAForSprings(rHand,objects,dt,doCollisionCheck);
        poseModePCAForSprings(lHand,objects,dt,doCollisionCheck);
        if (doPhysicsSprings) {
            poseModePCAForSprings(connections,objects,dt,doCollisionCheck);
        }
    }

    updateSprings();
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
    SketchObject *closest;
    double distance = std::numeric_limits<double>::max();
    q_vec_type pos1;
    subj->getPosition(pos1);
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        double bb[6];
        double dist;
        q_vec_type pos2;
        obj->getAABoundingBox(bb);
        obj->getPosition(pos2);
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
// helper function-- compute the normal n of triangle tri in the model
// assumes points in each triangle in the model are added in counterclockwise order
// relative to the outside of the surface looking down
inline void unit_normal(q_vec_type n, PQP_Model *m, int t) {
    q_vec_type diff1, diff2;
#ifdef PQP_UPDATE_EPSILON
    Tri *tri = &(m->tris[m->idsToIndices[t]]);
#else
    Tri *tri = NULL;
    for (int i = 0; i < m->num_tris; i++) {
        if (m->tris[i].id == t) {
            tri = &(m->tris[i]);
            break;
        }
    }
#endif
    q_vec_subtract(diff1,tri->p3,tri->p1);
    q_vec_subtract(diff2,tri->p2,tri->p1);
    q_vec_cross_product(n,diff2,diff1);
    q_vec_normalize(n,n);
}

//##################################################################################################
//##################################################################################################
// helper function -- compute the centriod of the triangle
inline void centriod(q_vec_type c, PQP_Model *m, int t) {
#ifdef PQP_UPDATE_EPSILON
    Tri *tri = &(m->tris[m->idsToIndices[t]]);
#else
    Tri *tri = NULL;
    for (int i = 0; i < m->num_tris; i++) {
        if (m->tris[i].id == t) {
            tri = &(m->tris[i]);
            break;
        }
    }
#endif
    q_vec_copy(c,tri->p1);
    q_vec_add(c,tri->p2,c);
    q_vec_add(c,tri->p3,c);
    q_vec_scale(c,1/3.0,c);
}

//##################################################################################################
//##################################################################################################
// -helper function to compute mean point of all triangle points where the triangles are involved in
//   the collision
inline void computeMeanCollisionPoint(PQP_REAL mean[3], PQP_CollideResult *cr,
                                      SketchModel *model1, bool isFirst,
                                      PQP_Model *m1) {
    int total = 3 * cr->NumPairs();
    mean[0] = mean[1] = mean[2] = 0.0;

    for (int i = 0; i < cr->NumPairs(); i++) {
        int m1Tri;
        if (isFirst)
            m1Tri= cr->Id1(i);
        else
            m1Tri = cr->Id2(i);
        int triId1;
#ifndef PQP_UPDATE_EPSILON
        triId1 = model1->getTriIdToTriIndexHash()->value(m1Tri);
#else
        triId1 = m1->idsToIndices(m1Tri);
#endif
        mean[0] += m1->tris[triId1].p1[0] + m1->tris[triId1].p2[0] + m1->tris[triId1].p3[0];
        mean[1] += m1->tris[triId1].p1[1] + m1->tris[triId1].p2[1] + m1->tris[triId1].p3[1];
        mean[2] += m1->tris[triId1].p1[2] + m1->tris[triId1].p2[2] + m1->tris[triId1].p3[2];
    }
    mean[0] = mean[0] / total;
    mean[1] = mean[1] / total;
    mean[2] = mean[2] / total;
}
//##################################################################################################
//##################################################################################################
// -helper function to compute the covariance of all triangle points involved in the collision on the
//   given model
inline void computeCollisonPointCovariance(PQP_REAL mean[3], PQP_CollideResult *cr,
                                           SketchModel *model1, bool isFirst,
                                           PQP_Model *m1, PQP_REAL cov[3][3]) {
    int total = 3 * cr->NumPairs();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cov[i][j] = 0;
        }
    }
    for (int k = 0; k < cr->NumPairs(); k++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int m1Tri;
                if (isFirst) {
                    m1Tri = cr->Id1(k);
                } else {
                    m1Tri = cr->Id2(k);
                }
                int triId1;
#ifndef PQP_UPDATE_EPSILON
                triId1 = model1->getTriIdToTriIndexHash()->value(m1Tri);
#else
                triId1 = m1->idsToIndices(m1Tri);
#endif
                cov[i][j] += (mean[i] - m1->tris[triId1].p1[i]) *
                             (mean[j] - m1->tris[triId1].p1[j])
                          +  (mean[i] - m1->tris[triId1].p2[i]) *
                             (mean[j] - m1->tris[triId1].p2[j])
                          +  (mean[i] - m1->tris[triId1].p3[i]) *
                             (mean[j] - m1->tris[triId1].p3[j]);
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cov[i][j] = cov[i][j] / (total-1);
        }
    }
}



//##################################################################################################
//##################################################################################################
void WorldManager::applyPCACollisionResponseForce(SketchObject *o1, SketchObject *o2,
                                               PQP_CollideResult *cr, QSet<int> &affectedGroups) {
    // get the collision models:
    SketchModel *model1 = o1->getModel();
    SketchModel *model2 = o2->getModel();
    PQP_Model *m1 = model1->getCollisionModel();
    PQP_Model *m2 = model2->getCollisionModel();

    // get object's orientations
    q_type quat1, quat2;
    (o1)->getOrientation(quat1);
    (o2)->getOrientation(quat2);

    PQP_REAL mean1[3], covariance1[3][3];
    PQP_REAL mean2[3], covariance2[3][3];

    computeMeanCollisionPoint(mean1,cr,model1,true,m1);
    computeCollisonPointCovariance(mean1,cr,model1,true,m1,covariance1);
    PQP_REAL covEigenVecs1[3][3], covEigenVals1[3];
    Meigen(covEigenVecs1,covEigenVals1,covariance1);
    int min1 = (covEigenVals1[0] < covEigenVals1[1]) ?
                ((covEigenVals1[2] < covEigenVals1[0]) ? 2 : 0) :
                ((covEigenVals1[2] < covEigenVals1[1]) ? 2 : 1);

    computeMeanCollisionPoint(mean2,cr,model2,false,m2);
    computeCollisonPointCovariance(mean2,cr,model2,false,m2,covariance2);
    PQP_REAL covEigenVecs2[3][3], covEigenVals2[3];
    Meigen(covEigenVecs2,covEigenVals2,covariance2);
    int min2 = (covEigenVals2[0] < covEigenVals2[1]) ?
                ((covEigenVals2[2] < covEigenVals2[0]) ? 2 : 0) :
                ((covEigenVals2[2] < covEigenVals2[1]) ? 2 : 1);

    q_vec_type dir1, dir2;
    dir1[0] = covEigenVecs1[0][min1];
    dir1[1] = covEigenVecs1[1][min1];
    dir1[2] = covEigenVecs1[2][min1];
    dir2[0] = covEigenVecs2[0][min2];
    dir2[1] = covEigenVecs2[1][min2];
    dir2[2] = covEigenVecs2[2][min2];
    q_vec_normalize(dir1,dir1);
    q_vec_normalize(dir2,dir2);
    if (q_vec_dot_product(dir1,mean1) < 0) {
        q_vec_invert(dir1,dir1);
    }
    if (q_vec_dot_product(dir2,mean2) < 0) {
        q_vec_invert(dir2,dir2);
    }

    q_vec_type dir1W, dir2W;
    q_vec_type mean1W, mean2W;
    o1->getModelSpacePointInWorldCoordinates(mean1,mean1W);
    o2->getModelSpacePointInWorldCoordinates(mean2,mean2W);
    o1->getLocalTransform()->TransformVector(dir1,dir1W);
    o2->getLocalTransform()->TransformVector(dir2,dir2W);
    q_vec_scale(dir1W,COLLISION_FORCE*30,dir1W);
    q_vec_scale(dir2W,COLLISION_FORCE*30,dir2W);
    if (affectedGroups.empty() || affectedGroups.contains(o1->getPrimaryCollisionGroupNum())) {
        o1->addForce(mean1W,dir1W);
    }
    if (affectedGroups.empty() || affectedGroups.contains(o2->getPrimaryCollisionGroupNum())) {
        o2->addForce(mean2W,dir2W);
    }
}

//##################################################################################################
//##################################################################################################
void WorldManager::applyCollisionResponseForce(SketchObject *o1, SketchObject *o2,
                                               PQP_CollideResult *cr, QSet<int> &affectedGroups) {
    // get the collision models:
    PQP_Model *pqp_model1 = o1->getModel()->getCollisionModel();
    PQP_Model *pqp_model2 = o2->getModel()->getCollisionModel();

    // get object's orientations
    q_type quat1, quat2;
    (o1)->getOrientation(quat1);
    (o2)->getOrientation(quat2);

    double cForce = COLLISION_FORCE *40 / cr->NumPairs();
    // scale it by the number of colliding primaries so that we have some chance of sliding along surfaces
    // instead of sticking

    // for each pair in collision
    for (int i = 0; i < cr->NumPairs(); i++) {
        int m1Tri = cr->Id1(i);
        int m2Tri = cr->Id2(i);
        // compute the normal vectors...
        q_vec_type n1, n2;
        unit_normal(n1,pqp_model1,m1Tri);
        unit_normal(n2,pqp_model2,m2Tri);
        // compute the forces from those normal vectors
        q_vec_type f1,f2;
        q_xform(f1,quat2,n2);
        q_xform(f2,quat1,n1);
        q_vec_scale(f1,cForce,f1);
        q_vec_scale(f2,cForce,f2);
        // compute the centroids of the triangles as the point to which
        // the forces is applied
        q_vec_type p1,p2;
        centriod(p1,pqp_model1,m1Tri);
        centriod(p2,pqp_model2,m2Tri);
        // apply the forces
        if (affectedGroups.empty() || affectedGroups.contains(o1->getPrimaryCollisionGroupNum())) {
            (o1)->addForce(p1,f1);
        }
        if (affectedGroups.empty() || affectedGroups.contains(o2->getPrimaryCollisionGroupNum())) {
            (o2)->addForce(p2,f2);
        }
    }
}

