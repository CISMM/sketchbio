#include "worldmanager.h"
#include <vtkTubeFilter.h>
#include <QDebug>

#define COLLISION_FORCE 5

WorldManager::WorldManager(ModelManager *models, vtkRenderer *r, vtkTransform *worldEyeTransform) :
    objects(),
    connections()
{
    modelManager = models;
    renderer = r;
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

WorldManager::~WorldManager() {
    for (ObjectId it = objects.begin(); it != objects.end(); it++) {
        SketchObject *obj = (*it);
        (*it) = NULL;
        delete obj;
    }
    for (SpringId it = connections.begin(); it != connections.end(); it++) {
        SpringConnection *spring = (*it);
        (*it) = NULL;
        delete spring;
    }
    objects.clear();
    connections.clear();
}

ObjectId WorldManager::addObject(int modelId,q_vec_type pos, q_type orient) {
    SketchModel *model = modelManager->getModelFor(modelId);
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(model->getMapper());
    SketchObject *newObject = new SketchObject(actor,modelId,worldEyeTransform);
    newObject->setPosition(pos);
    newObject->setOrientation(orient);
    newObject->recalculateLocalTransform();
    renderer->AddActor(actor);
    objects.push_back(newObject);
    ObjectId id = objects.end();
    id--;
    return id;
}

void WorldManager::removeObject(ObjectId objectId) {
    // if we are deleting an object, it should not have any springs...
    // TODO - fix this assumption later
    SketchObject *obj = (*objectId);
    objects.erase(objectId);
    delete obj;
}

int WorldManager::getNumberOfObjects() {
    return objects.size();
}

SpringId WorldManager::addSpring(SpringConnection *spring) {
    connections.push_back(spring);
    SpringId it = connections.end();
    it--;

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

    return it;
}

SpringId WorldManager::addSpring(ObjectId id1, ObjectId id2,const q_vec_type worldPos1,
                             const q_vec_type worldPos2, double k, double l)
{
    SketchObject *obj1 = *id1, *obj2 = *id2;
    q_type pos1, pos2, newPos1, newPos2;
    q_type orient1, orient2;
    obj1->getPosition(pos1);
    obj1->getOrientation(orient1);
    obj2->getPosition(pos2);
    obj2->getOrientation(orient2);
    q_invert(orient1,orient1);
    q_invert(orient2,orient2);
    q_vec_subtract(newPos1,worldPos1,pos1);
    q_xform(newPos1,orient1,newPos1);
    q_vec_subtract(newPos2,worldPos2,pos2);
    q_xform(newPos2,orient2,newPos2);
    SpringConnection *spring = new SpringConnection(obj1,obj2,l,k,newPos1,newPos2);
    return addSpring(spring);
}

void WorldManager::removeSpring(SpringId id) {
    SpringConnection *conn = (*id);
    connections.erase(id);

    springEndConnections->DeleteCell(conn->getCellId());
    // can't delete points...

    delete conn;
}

int WorldManager::getNumberOfSprings() {
    return connections.size();
}

// helper method for stepPhysics -- does Euler's method once force
// and torque on an object have been calculated
inline void euler(ObjectId o, ModelManager *modelManager, double dt) {
    q_vec_type pos, angularVel,deltaPos,force,torque;
    q_type spin, orient;
    SketchObject * obj = (*o);
    if (obj->doPhysics()) {
        SketchModel *model = modelManager->getModelFor(obj->getModelId());
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
    obj->recalculateLocalTransform();
}

void WorldManager::stepPhysics(double dt) {

    // clear the accumulated force in the objects
    for (ObjectId it = objects.begin(); it != objects.end(); it++) {
        (*it)->clearForces();
    }

    // TODO collision detection - collision causes a force
    // TODO force from collisions
    for (ObjectId i = objects.begin(); i != objects.end(); i++) {
        for (ObjectId j = i; j != objects.end(); j++) {
            if (i == j) // self collisions not handled now --TODO
                continue;
            if ((*i)->isNormalObject() && (*j)->isNormalObject())
                collide(i,j);
        }
    }

    // spring forces between models / models & trackers
    for (SpringId it = connections.begin(); it != connections.end(); it++) {
        (*it)->addForce();
    }

    // apply forces - this is using Euler's Method for now
    for (ObjectId it = objects.begin(); it != objects.end(); it++) {
        euler(it,modelManager,dt);
    }

    updateSprings();
}

void WorldManager::updateSprings() {
    // set spring ends to new positions
    for (SpringId id = connections.begin(); id != connections.end(); id++) {
        q_vec_type pos1,pos2;
        (*id)->getEnd1WorldPosition(pos1);
        springEnds->SetPoint((*id)->getEnd1Id(),pos1);
        (*id)->getEnd2WorldPosition(pos2);
        springEnds->SetPoint((*id)->getEnd2Id(),pos2);
    }
    springEnds->Modified();
    int num = springEndConnections->GetNumberOfLines(), num2;
    springEndConnections->RemoveDeletedCells();
    if (num != (num2 = springEndConnections->GetNumberOfLines())) {
        for (vtkIdType i = 0; i < num2; i++) {
            springEndConnections->DeleteCell(i);
        }
        springEndConnections->RemoveDeletedCells();
        for (SpringId id = connections.begin(); id != connections.end(); id++) {
            vtkIdType pts[2];
            pts[0] = (*id)->getEnd1Id();
            pts[1] = (*id)->getEnd2Id();
            vtkIdType cellId = springEndConnections->InsertNextCell(VTK_LINE,2,pts);
            (*id)->setCellId(cellId);
        }
        springEndConnections->Modified();
        springEndConnections->Update();
        tubeFilter->Update();
    }
    springEndConnections->Update();
}

// helper function-- converts quaternion to a PQP matrix
inline void quatToPQPMatrix(const q_type quat, PQP_REAL mat[3][3]) {
    q_matrix_type rowMat;
    q_to_col_matrix(rowMat,quat);
    for (int i = 0; i < 3; i++) {
        mat[i][0] = rowMat[i][0];
        mat[i][1] = rowMat[i][1];
        mat[i][2] = rowMat[i][2];
    }
}

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


void WorldManager::collide(ObjectId o1, ObjectId o2) {
    // get the collision models:
    int m1 = (*o1)->getModelId();
    int m2 = (*o2)->getModelId();
    SketchModel *model1 = modelManager->getModelFor(m1);
    SketchModel *model2 = modelManager->getModelFor(m2);
    PQP_Model *pqp_model1 = model1->getCollisionModel();
    PQP_Model *pqp_model2 = model2->getCollisionModel();

    // get the offsets and rotations in PQP's format
    PQP_CollideResult cr;
    PQP_REAL r1[3][3], t1[3], r2[3][3],t2[3];
    q_type quat1, quat2;
    (*o1)->getOrientation(quat1);
    quatToPQPMatrix(quat1,r1);
    (*o1)->getPosition(t1);
    (*o2)->getOrientation(quat2);
    quatToPQPMatrix(quat2,r2);
    (*o2)->getPosition(t2);

    // collide them
    PQP_Collide(&cr,r1,t1,pqp_model1,r2,t2,pqp_model2,PQP_ALL_CONTACTS);

//    qDebug() << "Collisions:" << cr.NumPairs();
//    if (cr.NumPairs()!= 0) {
//        q_vec_print(t1);
//        q_vec_print(t2);
//        q_print(quat1);
//        q_print(quat2);
//    }

    // for each pair in collision
    for (int i = 0; i < cr.NumPairs(); i++) {
        int m1Tri = cr.Id1(i);
        int m2Tri = cr.Id2(i);
        // compute the normal vectors...
        q_vec_type n1, n2;
        unit_normal(n1,pqp_model1,m1Tri);
        unit_normal(n2,pqp_model2,m2Tri);
        // compute the forces from those normal vectors
        q_vec_type f1,f2;
        q_xform(f1,quat2,n2);
        q_xform(f2,quat1,n1);
        q_vec_scale(f1,COLLISION_FORCE,f1);
        q_vec_scale(f2,COLLISION_FORCE,f2);
        // compute the centroids of the triangles as the point to which
        // the forces is applied
        q_vec_type p1,p2;
        centriod(p1,pqp_model1,m1Tri);
        centriod(p2,pqp_model2,m2Tri);
        // apply the forces
        (*o1)->addForce(p1,f1);
        (*o2)->addForce(p2,f2);
//        qDebug() << "points" << endl;
//        q_vec_print(p1);
//        q_vec_print(p2);
//        qDebug() << "torques" << endl;
    }
}
