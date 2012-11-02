#include "worldmanager.h"
#include <vtkTubeFilter.h>

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
    springEndConnections->RemoveDeletedCells();
    // can't delete points...
    springEndConnections->Modified();
    springEndConnections->Update();
    tubeFilter->Update();

    delete conn;
}

int WorldManager::getNumberOfSprings() {
    return connections.size();
}

void WorldManager::stepPhysics(double dt) {

    // clear the accumulated force in the objects
    for (ObjectId it = objects.begin(); it != objects.end(); it++) {
        (*it)->clearForces();
    }

    // TODO collision detection - collision causes a force
    // TODO force from collisions

    // spring forces between models / models & trackers
    for (SpringId it = connections.begin(); it != connections.end(); it++) {
        (*it)->addForce();
    }

    // apply forces - this is using Euler's Method for now
    for (ObjectId it = objects.begin(); it != objects.end(); it++) {
        q_vec_type pos, angularVel,deltaPos,force,torque;
        q_type spin, orient;
        SketchObject * obj = (*it);
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

    // set spring ends to new positions
    for (SpringId id = connections.begin(); id != connections.end(); id++) {
        q_vec_type pos1,pos2;
        (*id)->getEnd1WorldPosition(pos1);
        springEnds->SetPoint((*id)->getEnd1Id(),pos1);
        (*id)->getEnd2WorldPosition(pos2);
        springEnds->SetPoint((*id)->getEnd2Id(),pos2);
    }
    springEnds->Modified();
    springEndConnections->Update();
}
