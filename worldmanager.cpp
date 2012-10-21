#include "worldmanager.h"

WorldManager::WorldManager(ModelManager *models, vtkRenderer *r) {
    modelManager = models;
    renderer = r;
    objects = std::map<int,SketchObject *>();
    nextIdx = 0;
}

WorldManager::~WorldManager() {
    for (std::map<int,SketchObject *>::iterator it = objects.begin(); it != objects.end(); it++) {
        SketchObject *obj = (*it).second;
        (*it).second = NULL;
        delete obj;
    }
    objects.clear();
}

int WorldManager::addObject(int modelId,q_vec_type pos, q_type orient, vtkTransform *worldEyeTransform) {
    SketchModel *model = modelManager->getModelFor(modelId);
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(model->getMapper());
    SketchObject *newObject = new SketchObject(actor,modelId,worldEyeTransform);
    newObject->setPosition(pos);
    newObject->setOrientation(orient);
    newObject->recalculateLocalTransform();
    renderer->AddActor(actor);
    objects[nextIdx] = newObject;
    return nextIdx++;
}

void WorldManager::removeObject(int objectId) {
    std::map<int,SketchObject *>::iterator it = objects.find(objectId);
    SketchObject *object = (*it).second;
    vtkSmartPointer<vtkActor> actor = object->getActor();
    objects.erase(it);
    renderer->RemoveActor(actor);
    delete object;
}

SketchObject *WorldManager::getObject(int objectId) {
    return objects[objectId];
}

int WorldManager::getNumberOfObjects() {
    return objects.size();
}

// array of i => objectId can be set up in linear time wrt i... I think
void WorldManager::stepPhysics(double dt) {
    int numObjs = objects.size();
    int *index = new int[numObjs];
    int idx = 0;
    // build an array of [0 .. numObjs] => objectId, so we can loop over the first and access
    // the second
    for (std::map<int,SketchObject *>::iterator it = objects.begin(); it != objects.end(); it++) {
        index[idx++] = (*it).first;
    }

    q_vec_type *forces = new q_vec_type[nextIdx];
    q_vec_type *torques = new q_vec_type[nextIdx];
    // collision detection - collision causes a force
    // TODO
    // force & torque calculation
    // initialize
    for (int i = 0; i < numObjs; i++) {
        forces[i][0] = 0;
        forces[i][1] = 0;
        forces[i][2] = 0;
        torques[i][0] = 0;
        torques[i][1] = 0;
        torques[i][2] = 0;
    }
    // TODO force from collisions
    // TODO spring forces between models / models & trackers

    // apply forces - this is using Euler's Method for now
    for (int i = 0; i < numObjs; i++) {
        q_vec_type pos, angularVel,deltaPos;
        q_type spin, orient;
        SketchObject * obj = objects[index[i]];
        SketchModel *model = modelManager->getModelFor(obj->getModelId());
        // new position
        obj->getPosition(pos);
        q_vec_scale(deltaPos,dt*model->getInverseMass(),forces[i]);
        q_vec_add(pos,pos,deltaPos);
        // new orientation
        obj->getOrientation(orient);
        q_vec_scale(angularVel,model->getInverseMomentOfInertia(),torques[i]);
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
        obj->recalculateLocalTransform();
    }

    // cleanup
    delete[] index;
    delete[] forces;
    delete[] torques;
}
