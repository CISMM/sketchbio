#include <iostream>

#include <quat.h>

#include <QTime>
#include <QString>
#include <QDir>
#include <QScopedPointer>

#include <vtkSmartPointer.h>
#include <vtkCubeSource.h>
#include <vtkRenderer.h>

#include <sketchioconstants.h>
#include <modelutilities.h>
#include <sketchmodel.h>
#include <sketchobject.h>
#include <objectgroup.h>
#include <worldmanager.h>
#include <structurereplicator.h>
#include <sketchtests.h>

using std::cout;
using std::endl;

#include "TestCoreHelpers.h"


/*
 * Tests replication with translation only
 */
int test1()
{
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    SketchObject *obj1 = world->addObject(model.data(),pos,orient);
    double distance_moved = 1;
    pos[0] = distance_moved;
    SketchObject *obj2 = world->addObject(model.data(),pos,orient);
    QScopedPointer< StructureReplicator > rep(
                new StructureReplicator(obj1, obj2, world.data()));
    rep->setNumShown(5);
    int errors = 0;
    QListIterator< SketchObject * > itr = rep->getReplicaIterator();
    int num = 2;
    q_vec_type offset, temp1;
    q_type resultOr;
    q_vec_copy(offset,pos);
    while (itr.hasNext())
    {
        SketchObject *replica = itr.next();
        if (replica != obj1 && replica != obj2)
        {
            q_vec_add(pos,offset,pos);
            replica->getPosition(temp1);
            replica->getOrientation(resultOr);
            if (!q_equals(resultOr, orient, Q_EPSILON * num))
            {
                errors++;
                cout << "Result orientation is wrong for linear test." << endl;
            }
            if (!q_vec_equals(temp1, pos, Q_EPSILON * num))
            {
                errors++;
                cout << "Result position is wrong for linear test." << endl;
                cout << "Actual: ";
                q_vec_print(temp1);
                cout << "Should be: ";
                q_vec_print(pos);
            }
            num++;
        }
    }
    return errors;
}

/*
 * Test replication with rotation only
 */
int test2()
{
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    SketchObject *obj1 = world->addObject(model.data(),pos,orient);
    q_from_axis_angle(orient,1,0,0,.5);
    SketchObject *obj2 = world->addObject(model.data(),pos,orient);
    QScopedPointer< StructureReplicator > rep(
                new StructureReplicator(obj1, obj2, world.data()));
    rep->setNumShown(5);
    int errors = 0;
    QListIterator< SketchObject * > itr = rep->getReplicaIterator();
    int num = 2;
    q_vec_type temp1;
    q_type resultOr, increment;
    q_copy(increment,orient);
    while (itr.hasNext())
    {
        SketchObject *replica = itr.next();
        if (replica != obj1 && replica != obj2)
        {
            q_mult(orient,increment,orient);
            replica->getPosition(temp1);
            replica->getOrientation(resultOr);
            if (!q_equals(resultOr, orient, Q_EPSILON * num))
            {
                errors++;
                cout << "Result orientation is wrong for rotation test." << endl;
            }
            if (!q_vec_equals(temp1, pos, Q_EPSILON * num))
            {
                errors++;
                cout << "Result position is wrong for rotation test." << endl;
            }
            num++;
        }
    }
    return errors;
}

/*
 * Test replication with rotation and translation (180 degree rotation)
 */
int test3()
{
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    SketchObject *obj1 = world->addObject(model.data(),pos,orient);
    q_vec_set(pos,0,0,5);
    q_from_axis_angle(orient,1,0,0,Q_PI/3.0);
    SketchObject *obj2 = world->addObject(model.data(),pos,orient);
    QScopedPointer< StructureReplicator > rep(
                new StructureReplicator(obj1, obj2, world.data()));
    rep->setNumShown(5);
    int errors = 0;
    QListIterator< SketchObject * > itr = rep->getReplicaIterator();
    int num = 2;
    q_vec_type temp1, temp2;
    q_type resultOr, increment;
    q_copy(increment,orient);
    q_vec_copy(temp2,pos);
    while (itr.hasNext())
    {
        SketchObject *replica = itr.next();
        if (replica != obj1 && replica != obj2)
        {
            q_xform(temp1,orient,pos);
            q_vec_add(temp2,temp1,temp2);
            q_mult(orient,increment,orient);
            replica->getPosition(temp1);
            replica->getOrientation(resultOr);
            if (!q_equals(resultOr, orient, Q_EPSILON * num))
            {
                errors++;
                cout << "Result orientation is wrong for rotation and translation test." << endl;
            }
            if (!q_vec_equals(temp1, temp2, Q_EPSILON * num))
            {
                errors++;
                cout << "Result position is wrong for rotationn and translation test." << endl;
                q_vec_print(temp1);
                q_vec_print(temp2);
            }
            num++;
        }
    }
    return errors;
}

/*
 * Tests a bug found where deleting the replica group from the world causes a segfault on shutdown.
 */
void testDeleteGroupSegfault()
{
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchModel > model(TestCoreHelpers::getCubeModel());
    QScopedPointer< WorldManager > world(new WorldManager(renderer));
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    SketchObject *obj1 = world->addObject(model.data(),pos,orient);
    q_vec_set(pos,0,0,5);
    q_from_axis_angle(orient,1,0,0,Q_PI/3.0);
    SketchObject *obj2 = world->addObject(model.data(),pos,orient);
    QScopedPointer< StructureReplicator > rep(
                new StructureReplicator(obj1, obj2, world.data()));
    rep->setNumShown(5);
    world->deleteObject(rep->getReplicaGroup());
}

int main()
{
    testDeleteGroupSegfault();
    return test1() + test2() + test3();
}
