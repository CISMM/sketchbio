#include <iostream>
using std::cout;
using std::endl;

#include <quat.h>

#include <QScopedPointer>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <sketchmodel.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <worldmanager.h>

#include "TestCoreHelpers.h"

// Tests of object methods...
int testRemoveObject();
int testDeleteObject();

int main()
{
    int errors = 0;
    errors += testRemoveObject();
    errors += testDeleteObject();
    return errors;
}

// Creates test objects for functions that take an object.
// World is the world to add all the objects to.
// model is the model that all the model instances should be
// o1 - will be a single instance outside of a group
// o2 - will be a group
// o3 - will be an instance in a group
// o4 - will be a subgroup of a group
void createObjects(WorldManager *world,
                   SketchModel *model,
                   SketchObject * &o1,
                   SketchObject * &o2,
                   SketchObject * &o3,
                   SketchObject * &o4)
{
    o1 = new ModelInstance(model);
    world->addObject(o1);
    SketchObject *o = new ModelInstance(model);
    ObjectGroup *grp1 = new ObjectGroup();
    grp1->addObject(o);
    o = new ModelInstance(model);
    grp1->addObject(o);
    world->addObject(grp1);
    o2 = grp1;
    grp1 = new ObjectGroup();
    o = new ModelInstance(model);
    grp1->addObject(o);
    o = new ModelInstance(model);
    grp1->addObject(o);
    world->addObject(grp1);
    o3 = o;
    grp1 = new ObjectGroup();
    o = new ModelInstance(model);
    ObjectGroup *grp2 = new ObjectGroup();
    grp1->addObject(o);
    o = new ModelInstance(model);
    grp1->addObject(o);
    grp2->addObject(grp1);
    grp1 = new ObjectGroup();
    o = new ModelInstance(model);
    grp1->addObject(o);
    o = new ModelInstance(model);
    grp1->addObject(o);
    grp2->addObject(grp1);
    world->addObject(grp2);
    o4 = grp1;
}

// This is currently testing for recurrence of Bug #807 in which some cases of
// deleting an object would cause segfaults, with the problem being in removeObject
int testRemoveObject()
{
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    SketchObject *o1, *o2, *o3, *o4;
    createObjects(world.data(),model.data(),o1,o2,o3,o4);
    world->removeObject(o1);
    delete o1;
    world->removeObject(o2);
    delete o2;
    world->removeObject(o3);
    delete o3;
    world->removeObject(o4);
    delete o4;
    return 0;
}

// This is currently testing for recurrence of Bug #807 in which some cases of
// deleting an object would cause segfaults.  The problem was in removeObject, but
// just in case something changes, this test is added also
int testDeleteObject()
{
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    SketchObject *o1, *o2, *o3, *o4;
    createObjects(world.data(),model.data(),o1,o2,o3,o4);
    world->deleteObject(o1);
    world->deleteObject(o2);
    world->deleteObject(o3);
    world->deleteObject(o4);
    return 0;
}
