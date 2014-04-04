/*
 * This is a test of the controls that are generally applicable across
 * all modes.  Set collision detection on/off, set springs on/off,
 * undo/redo, change modes, zoom, etc.
 */

#include <QScopedPointer>
#include <QDir>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

#include <sketchioconstants.h>
#include <sketchtests.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <structurereplicator.h>
#include <worldmanager.h>
#include <sketchproject.h>

#include <hydrainputmode.h>
#include <hydrainputmanager.h>
#include <groupeditingmode.h>

#include <test/TestCoreHelpers.h>

// Note: testing individual cases is not really necessary right now
//       since this uses the same code as save/load.  The test is
//       whether the code segfaults or leaks memory and if it gets
//       back to the right state (which is determined by the position
//       of an single object)
int testUndoRedo();
int testMultilevelGroupUndo();

int main(int argc, char *argv[])
{
    int errors = 0;
    errors += testUndoRedo();
    errors += testMultilevelGroupUndo();
    return errors;
}

inline void clickButton(HydraInputManager *mgr, int buttonNum)
{
    mgr->setButtonState(buttonNum,true);
    mgr->setButtonState(buttonNum,false);
}

inline int testPositionOfObject(SketchBio::Project *project, q_vec_type expected)
{
    SketchObject *obj = project->getWorldManager().getObjects()->at(0);
    q_vec_type actual;
    obj->getPosition(actual);
    if (!q_vec_equals(expected,actual))
    {
        std::cout << "Wrong position after undo." << std::endl;
        return 1;
    }
    return 0;
}

inline void createState(HydraInputManager *manager,SketchBio::Project *project, int num)
{
    SketchObject *obj = project->getWorldManager().getObjects()->at(0);
    q_vec_type pos = {num, 0.0, 0.0};
    obj->setPosition(pos);
    manager->getActiveMode()->addXMLUndoState();
}

// Note: testing individual cases is not really necessary right now
//       since this uses the same code as save/load.  The test is
//       whether the code segfaults or leaks memory and if it gets
//       back to the right state (which is determined by the position
//       of an single object)
int testUndoRedo()
{
    int errors = 0;
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchBio::Project > project(
                new SketchBio::Project(renderer,QDir::currentPath()));
    QScopedPointer< HydraInputManager > manager(
                new HydraInputManager(project.data()));
    // As long as we don't have the vrpn server running, this should be fine
    // and we should be able to test things as if vrpn devices were not initialized
    SketchModel *model = TestCoreHelpers::getCubeModel();
    project->getModelManager().addModel(model);
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    project->getWorldManager().addObject(model,pos,orient);
    manager->getActiveMode()->addXMLUndoState();
    createState(manager.data(),project.data(),1);
    createState(manager.data(),project.data(),2);
    // this will be deleted when undo is pressed and the project cleared/reloaded
    // so clear the pointer
    model = NULL;
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    q_vec_type expected = {1,0,0};
    errors += testPositionOfObject(project.data(),expected);
    clickButton(manager.data(),BUTTON_LEFT(OBLONG_BUTTON_IDX));
    expected[0] = 2;
    errors += testPositionOfObject(project.data(),expected);
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    expected[0] = 1;
    errors += testPositionOfObject(project.data(),expected);
    createState(manager.data(),project.data(),3);
    clickButton(manager.data(),BUTTON_LEFT(OBLONG_BUTTON_IDX));
    expected[0] = 3;
    errors += testPositionOfObject(project.data(),expected);
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    expected[0] = 1;
    errors += testPositionOfObject(project.data(),expected);
    clickButton(manager.data(),BUTTON_LEFT(OBLONG_BUTTON_IDX));
    expected[0] = 3;
    errors += testPositionOfObject(project.data(),expected);
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    clickButton(manager.data(),BUTTON_LEFT(OBLONG_BUTTON_IDX));
    clickButton(manager.data(),BUTTON_LEFT(OBLONG_BUTTON_IDX));
    expected[0] = 3;
    errors += testPositionOfObject(project.data(),expected);
    return errors;
}

inline void createMultiLevelGroup(SketchBio::Project *proj,SketchModel *model)
{
    SketchObject *obj[4];
    ObjectGroup *grpL1 = new ObjectGroup();
    ObjectGroup *grp = new ObjectGroup();
    grp->addObject(grpL1);
    for (int i = 0; i < 4; i++)
    {
        obj[i] = new ModelInstance(model,0);
        q_vec_type pos = { i + 1.0 , 0 , 0 };
        obj[i]->setPosition(pos);
    }
    grpL1->addObject(obj[2]);
    grpL1->addObject(obj[3]);
    StructureReplicator *rep = proj->addReplication(obj[1],obj[0],3);
    proj->getWorldManager().removeObject(rep->getReplicaGroup());
    grp->addObject(rep->getReplicaGroup());
    proj->getWorldManager().addObject(grp);
}

// This test is testing the recurrance of Bug841, a segfault in undo/redo when
// multi-level groups are present, potentially a double free issue
int testMultilevelGroupUndo()
{
    cout << "Double free test" << endl;
    int errors = 0;
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchBio::Project > project(
                new SketchBio::Project(renderer,QDir::currentPath()));
    QScopedPointer< HydraInputManager > manager(
                new HydraInputManager(project.data()));
    // As long as we don't have the vrpn server running, this should be fine
    // and we should be able to test things as if vrpn devices were not initialized
    SketchModel *model = TestCoreHelpers::getCubeModel();
    project->getModelManager().addModel(model);
    // create a group with a replica group as a subgroup
    createMultiLevelGroup(project.data(),model);
    // create some undo states
    createState(manager.data(),project.data(),1);
    createState(manager.data(),project.data(),0);
    // call undo and segfault
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    clickButton(manager.data(),BUTTON_LEFT(OBLONG_BUTTON_IDX));
    clickButton(manager.data(),BUTTON_RIGHT(OBLONG_BUTTON_IDX));
    return errors;
}
