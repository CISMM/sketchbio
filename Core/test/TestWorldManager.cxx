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
#include <springconnection.h>
#include <worldmanager.h>

#include "TestCoreHelpers.h"

// Tests of object methods...
int testRemoveObject();
int testDeleteObject();
int testSelection();

// Tests of bugs
int testSelectionBug();

int main()
{
    int errors = 0;
    errors += testRemoveObject();
    errors += testDeleteObject();
    errors += testSelection();
    errors += testSelectionBug();
    return errors;
}

// Creates test objects for functions that take an object.
// World is the world to add all the objects to.
// model is the model that all the model instances should be
// o1 - will be a single instance outside of a group {0,0,0}
// o2 - will be a group {10,0,0}
// o3 - will be an instance in a group {1,10,0}
// o4 - will be a subgroup of a group {0,0,-10}
void createObjects(WorldManager *world,
                   SketchModel *model,
                   SketchObject * &o1,
                   SketchObject * &o2,
                   SketchObject * &o3,
                   SketchObject * &o4)
{
    o1 = new ModelInstance(model);
    world->addObject(o1);
    q_vec_type pos = {1,0,0};
    SketchObject *o = new ModelInstance(model);
    ObjectGroup *grp1 = new ObjectGroup();
    o->setPosition(pos);
    grp1->addObject(o);
    o = new ModelInstance(model);
    q_vec_set(pos,-1,0,0);
    o->setPosition(pos);
    grp1->addObject(o);
    q_vec_set(pos,10,0,0);
    grp1->setPosition(pos);
    world->addObject(grp1);
    o2 = grp1;
    grp1 = new ObjectGroup();
    o = new ModelInstance(model);
    q_vec_set(pos,-1,0,0);
    o->setPosition(pos);
    grp1->addObject(o);
    o = new ModelInstance(model);
    q_vec_set(pos,1,0,0);
    o->setPosition(pos);
    grp1->addObject(o);
    q_vec_set(pos,0,10,0);
    grp1->setPosition(pos);
    world->addObject(grp1);
    o3 = o;
    grp1 = new ObjectGroup();
    o = new ModelInstance(model);
    q_vec_set(pos,1,0,0);
    o->setPosition(pos);
    ObjectGroup *grp2 = new ObjectGroup();
    grp1->addObject(o);
    o = new ModelInstance(model);
    q_vec_set(pos,-1,0,0);
    o->setPosition(pos);
    grp1->addObject(o);
    grp2->addObject(grp1);
    grp1 = new ObjectGroup();
    o = new ModelInstance(model);
    q_vec_set(pos,0,1,0);
    o->setPosition(pos);
    grp1->addObject(o);
    o = new ModelInstance(model);
    q_vec_set(pos,0,-1,0);
    o->setPosition(pos);
    grp1->addObject(o);
    grp2->addObject(grp1);
    q_vec_set(pos,0,0,-10);
    grp2->setPosition(pos);
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

// This is testing for a case in Bug #792 that occurs when a group only has one
// object in it.
int testSelectionBug()
{
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    SketchObject *o1 = new ModelInstance(model.data());
    ObjectGroup *group = new ObjectGroup();
    group->addObject(o1);
    q_vec_type v1 = {400,-30,50};
    o1->setPosition(v1);
    world->addObject(group);
    QScopedPointer< SketchObject > subject(new ModelInstance(model.data()));
    subject->setPosition(v1);
    double dist;
    int errors = 0;
    SketchObject *closest = world->getClosestObject(subject.data(),dist);
    if (closest != group || dist > 0)
    {
        errors++;
        cout << "Group with one object treated wrong in selection test." << endl;
    }
    return errors;
}

int testSelection()
{
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    SketchObject *o1, *o2, *o3, *o4;
    createObjects(world.data(),model.data(),o1,o2,o3,o4);
    q_vec_type v1 = {0,0,0};
    QScopedPointer< SketchObject > subject(new ModelInstance(model.data()));
    subject->setPosition(v1);
    double dist;
    int errors = 0;
    SketchObject *closest = world->getClosestObject(subject.data(),dist);
    if (closest != o1 || dist > 0)
    {
        errors++;
        cout << "Wrong selection of single object." << endl;
    }
    q_vec_set(v1,10,0,0);
    subject->setPosition(v1);
    closest = world->getClosestObject(subject.data(),dist);
    if (closest != o2 || dist > 0)
    {
        errors++;
        cout << "Wrong selection of group." << endl;
    }
    o1->setPosition(v1);
    closest = world->getClosestObject(subject.data(),dist);
    if (closest != o1 || dist > 0)
    {
        errors++;
        cout << "Wrong selection of object when overlapped with group." << endl;
    }
    q_vec_set(v1,0,10.5,0);
    subject->setPosition(v1);
    closest = world->getClosestObject(subject.data(),dist);
    if (closest != o3->getParent() || dist > 0)
    {
        errors++;
        cout << "Wrong selection of group2." << endl;
    }
    q_vec_set(v1,0,0,-15);
    subject->setPosition(v1);
    closest = world->getClosestObject(subject.data(),dist);
    if (closest != o4->getParent() || dist < 0)
    {
        errors++;
        cout << "Wrong selection of group3." << endl;
    }

    Connector* s1 = world->addSpring(o1,o4,v1,v1,false,1.0,0.0);
    q_vec_set(v1,0,0,5);
    Connector* s2 = world->addSpring(o3,o2,v1,v1,false,1.0,0.0);
    q_vec_set(v1,3,0,-10);
    bool closerToEnd1;
    Connector* closestSp = world->getClosestSpring(v1,&dist,&closerToEnd1);
    if (closestSp != s1 || !closerToEnd1)
    {
        errors++;
        cout << "Wrong closest spring end." << endl;
    }

    s2->getEnd2WorldPosition(v1);
    closestSp = world->getClosestSpring(v1,&dist,&closerToEnd1);
    if (closestSp != s2 || dist > Q_EPSILON || closerToEnd1)
    {
        errors++;
        cout << "Wrong closest spring end when giving an end position." << endl;
    }

    return errors;
}
