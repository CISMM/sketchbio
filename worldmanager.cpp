#include "worldmanager.h"
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
    pausePhysics = false;
    usePoseMode = true;
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
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(model->getSolidMapper());
    SketchObject *newObject = new SketchObject(actor,model,worldEyeTransform);
    newObject->setPosition(pos);
    newObject->setOrientation(orient);
    newObject->recalculateLocalTransform();
    newObject->setPrimaryGroupNum(maxGroupNum++);
    renderer->AddActor(actor);
    objects.push_back(newObject);
    return newObject;
}

//##################################################################################################
//##################################################################################################
SketchObject *WorldManager::addObject(SketchObject *object) {
    objects.push_back(object);
    if (object->getPrimaryGroupNum() == OBJECT_HAS_NO_GROUP) {
        object->setPrimaryGroupNum(maxGroupNum++);
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
// helper method for stepPhysics -- does Euler's method once force
// and torque on an object have been calculated
inline void euler(SketchObject *obj, double dt) {
    q_vec_type pos, angularVel,deltaPos,force,torque;
    q_type spin, orient;
    if (obj->doPhysics()) {
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
        obj->setPosition(pos);
        obj->setOrientation(orient);
    }
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
        int i = c->getObject1()->getPrimaryGroupNum();
        if (i != OBJECT_HAS_NO_GROUP) { // if this one isn't the tracker
            affectedGroups.insert(i);
        }
        if (cp != NULL && cp->getObject2()->getPrimaryGroupNum() != OBJECT_HAS_NO_GROUP) {
            affectedGroups.insert(cp->getObject2()->getPrimaryGroupNum());
        }
    }
}
//##################################################################################################
//##################################################################################################
// -helper function that loops over all objects and updates their locations based on current
//   forces on them
inline void applyEuler(QList<SketchObject *> &list, double dt) {
    int n = list.size();
    for (int i = 0; i < n; i++) {
        SketchObject *obj = list.at(i);
        euler(obj,dt);
        obj->recalculateLocalTransform();
        obj->clearForces();
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

inline bool collideAndRespond(QList<SketchObject *> &list, QSet<int> &affectedGroups, double dt, bool respond) {
    int n = list.size();
    bool foundCollision = false;
    for (int i = 0; i < n; i++) {
        // TODO - self collision once deformation added
        bool needsTest = affectedGroups.empty();
        for (QSetIterator<int> it(affectedGroups); it.hasNext();) {
            if (list.at(i)->isInGroup(it.next())) {
                needsTest = true;
            }
        }
        if (needsTest) {
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    PQP_CollideResult cr;
                    SketchObject::collide(list.at(i),list.at(j),&cr);
                    if (cr.NumPairs() != 0) {
                        if (respond) {
                            WorldManager::applyCollisionResponseForce(list.at(i),list.at(j),&cr,affectedGroups);
                        }
                        foundCollision = true;
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
inline void applyPoseModeCollisionResponse(QList<SketchObject *> &list, QSet<int> &affectedGroups, double dt) {
    int n = list.size();
    bool appliedResponse = collideAndRespond(list,affectedGroups,dt,true);
    if (appliedResponse) {
        bool stillColliding = collideAndRespond(list,affectedGroups,dt,false);
        if (stillColliding) {
            // if we couldn't fix collisions, undo the motion and return
            for (int i = 0; i < n; i++) {
                list.at(i)->restoreToLastLocation();
                list.at(i)->recalculateLocalTransform();
            }
            return;
        }
    }
    // if we fixed the collision or there were none, finalize new positions
    for (int i = 0; i < n; i++) {
        list.at(i)->setLastLocation();
    }
}
//##################################################################################################
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//   collision response on them.
inline void poseModeForSprings(QList<SpringConnection *> springs, QList<SketchObject *> objs, double dt) {
    QSet<int> affectedGroups;
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyEuler(objs,dt);
        applyPoseModeCollisionResponse(objs,affectedGroups,dt);
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
    if (usePoseMode) { // pose mode
        // spring forces for right hand interacton
        poseModeForSprings(rHand,objects,dt);
        // spring forces for left hand interaction
        poseModeForSprings(lHand,objects,dt);

        // spring forces for physics springs interaction
        if (!pausePhysics) {
            poseModeForSprings(connections,objects,dt);
        }
    } else { // non-pose-mode
        QSet<int> affectedGroups;
        springForcesFromList(rHand,affectedGroups);
        springForcesFromList(lHand,affectedGroups);
        if (!pausePhysics)
            springForcesFromList(connections,affectedGroups);
        applyEuler(objects,dt);
        affectedGroups.clear(); // if passed an empty set, it does full collision checks
        collideAndRespond(objects,affectedGroups,dt,true); // calculates collision response
        applyEuler(objects,dt);
    }

    updateSprings();
}

//##################################################################################################
//##################################################################################################
void WorldManager::togglePhysics() {
    pausePhysics = ! pausePhysics;
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
SketchObject *WorldManager::getClosestObject(SketchObject *subj, double *distOut) {
    SketchObject *closest;
    double dist = std::numeric_limits<double>::max();
    SketchModel *model1 = subj->getModel();
    PQP_Model *pqp_model1 = model1->getCollisionModel();
    PQP_REAL r1[3][3], t1[3];
    q_type quat1;
    subj->getOrientation(quat1);
    quatToPQPMatrix(quat1,r1);
    subj->getPosition(t1);
    for (QListIterator<SketchObject *> it(objects); it.hasNext();) {
        SketchObject *obj = it.next();
        if (obj->doPhysics()) {
            // get the collision models:
            SketchModel* model2 = obj->getModel();
            PQP_Model *pqp_model2 = model2->getCollisionModel();

            // get the offsets and rotations in PQP's format
            PQP_DistanceResult dr;
            PQP_REAL r2[3][3],t2[3];
            q_type quat2;
            obj->getOrientation(quat2);
            quatToPQPMatrix(quat2,r2);
            obj->getPosition(t2);
            PQP_Distance(&dr,r1,t1,pqp_model1,r2,t2,pqp_model2,1.5,500);
            if (dr.Distance() < dist) {
                closest = obj;
                dist = dr.Distance();
            }
        }
    }
    *distOut = dist;
    return closest;
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
        if (affectedGroups.empty() || affectedGroups.contains(o1->getPrimaryGroupNum())) {
            (o1)->addForce(p1,f1);
        }
        if (affectedGroups.empty() || affectedGroups.contains(o2->getPrimaryGroupNum())) {
            (o2)->addForce(p2,f2);
        }
    }
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
