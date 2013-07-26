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
#include <worldmanager.h>
#include <structurereplicator.h>
#include <sketchtests.h>

using std::cout;
using std::endl;


/*
 * Generates the model used for these tests
 */
SketchModel *getModel()
{
    vtkSmartPointer< vtkCubeSource > cube =
        vtkSmartPointer< vtkCubeSource >::New();
    cube->SetBounds(-1,1,-1,1,-1,1);
    cube->Update();
    QString fname = ModelUtilities::createFileFromVTKSource(cube, "replicated_cube");
    SketchModel *model = new SketchModel(DEFAULT_INVERSE_MASS,
                                         DEFAULT_INVERSE_MOMENT);
    model->addConformation(fname, fname);
    return model;
}

/*
 * Tests replication with translation only
 */
int test1()
{
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchModel > model(getModel());
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
        q_vec_add(pos,offset,pos);
        SketchObject *replica = itr.next();
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
        }
        num++;
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
    QScopedPointer< SketchModel > model(getModel());
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
        q_mult(orient,increment,orient);
        SketchObject *replica = itr.next();
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
    return errors;
}

int main()
{
    return test1() + test2();
}
