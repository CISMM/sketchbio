#include <quat.h>

#include <iostream>
using std::cout;
using std::endl;

#include <QDir>
#include <QScopedPointer>

#include <sketchtests.h>
#include <sketchmodel.h>
#include <modelinstance.h>
#include <objectgroup.h>

#include "TestCoreHelpers.h"

int testObjectGroup();

int main(int argc, char *argv[]) {
    QDir dir = QDir::current();
    // change the working dir to the dir where the test executable is
    QString executable = dir.absolutePath() + "/" + argv[0];
    int last = executable.lastIndexOf("/");
    if (QDir::setCurrent(executable.left(last)))
        dir = QDir::current();
    std::cout << "Working directory: " <<
                 dir.absolutePath().toStdString().c_str() << std::endl;
    int errors = 0;
    errors += testObjectGroup();
    return errors;
}


//#########################################################################
// tests if the ObjectGroup was properly initialized by the constructor
inline int testNewObjectGroup() {
    QScopedPointer<ObjectGroup> obj(new ObjectGroup());
    int errors = TestCoreHelpers::testNewSketchObject(obj.data());
    if (obj->numInstances() != 0) {
        errors++;
        cout << "Group does not have correct number of instances" << endl;
    }
    if (obj->getSubObjects() == NULL) {
       errors++;
       cout << "Getting list of sub objects failed" << endl;
    } else if (!obj->getSubObjects()->empty()) {
        errors++;
        cout << "Group already contains something." << endl;
    }
    return errors;
}



//#########################################################################
// test the actions performed on the object group work correctly
inline int testObjectGroupActions() {
    QScopedPointer<SketchModel> m(TestCoreHelpers::getSphereModel());
    ObjectGroup grp, grp2;
    int errors = TestCoreHelpers::testSketchObjectActions(&grp2);
    ModelInstance *a = new ModelInstance(m.data()), *b = new ModelInstance(m.data()), *c = new ModelInstance(m.data());
    q_vec_type va = {2,0,0}, vb = {0,4,0}, vc = {0,-1,-5}, vi = Q_NULL_VECTOR;
    q_vec_type v1 = {0,0,1}, v2, v3;
    q_type q1, q2, q3, qtmp, qid = Q_ID_QUAT;
    q_from_axis_angle(q1,1,0,0,Q_PI/4);
    q_from_axis_angle(q2,0,1,0,Q_PI/2);
    a->setPosition(va);
    a->setOrientation(q1);
    b->setPosition(vb);
    c->setPosition(vc);
    // test adding an object
    grp.addObject(a);
    a->getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        cout << "Item in group in wrong position" << endl;
    }
    a->getOrientation(q3);
    if (!q_vec_equals(q3,q1)) {
        errors++;
        cout << "Item in group changed orientation" << endl;
    }
    // test group position
    grp.getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        cout << "Group position wrong with one item" << endl;
    }
    if (grp.numInstances() != 1) {
        errors++;
        cout << "Group number instances wrong after add" << endl;
    }
    if (grp.getModel() != a->getModel()) {
        errors++;
        cout << "If num instances is 1, must provide model" << endl;
    }
    if (a->getParent() != &grp) {
        errors++;
        cout << "Parent not set correctly" << endl;
    }
    // add another item and test positions
    grp.addObject(b);
    a->getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        cout << "Item in group in wrong position" << endl;
    }
    a->getOrientation(q3);
    if (!q_vec_equals(q3,q1)) {
        errors++;
        cout << "Item in group changed orientation" << endl;
    }
    b->getPosition(v2);
    if (!q_vec_equals(v2,vb)) {
        errors++;
        cout << "Item in group in wrong position" << endl;
    }
    // test group position (should be average of item positions)
    grp.getPosition(v2);
    if (!q_vec_equals(v2,vi)) {
        errors++;
        cout << "Group position wrong with two item" << endl;
        q_vec_print(v3);
        q_vec_print(v2);
    }
    grp.addObject(c);
    // test getting number of instances in group
    if (grp.numInstances() != 3) {
        errors++;
        cout << "Group number instances wrong after add" << endl;
    }
    // check net positions of group items
    a->getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        cout << "Item in group in wrong position" << endl;
    }
    a->getOrientation(q3);
    if (!q_vec_equals(q3,q1)) {
        errors++;
        cout << "Item in group changed orientation" << endl;
    }
    b->getPosition(v2);
    if (!q_vec_equals(v2,vb)) {
        errors++;
        cout << "Item in group in wrong position" << endl;
    }
    c->getPosition(v2);
    if (!q_vec_equals(v2,vc)) {
        errors++;
        cout << "Item in group in wrong position" << endl;
    }
    // test group position (should be average position of members)
    grp.getPosition(v2);
    if (!q_vec_equals(v2,vi)) {
        errors++;
        cout << "Group position wrong with three item" << endl;
    }
    // check position of group members after group translation
    grp.setPosition(v1);
    q_vec_add(v2,v1,vb);
    b->getPosition(v3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        cout << "Group member didn't move right" << endl;
    }
    // check orientation of members after rotation
    a->getPosition(v3); // save object position before rotation
    grp.setOrientation(q2);
    q_mult(qtmp,q2,q1);
    a->getOrientation(q3);
    if (!q_equals(qtmp,q3)) {
        errors++;
        cout << "Group member didn't rotate right" << endl;
    }
    // check positions of group members after rotation
    grp.getPosition(v2); // get group position
    q_vec_subtract(v3,v3,v2); // get vector from group to object before rotation
    q_xform(v3,q2,v3); // transform the vector by the group's rotation
    q_vec_add(v3,v2,v3); // add the group position to get object's final location
    a->getPosition(v2); // get object's final location
    if (!q_vec_equals(v2,v3)) { // check if they are the same
        errors++;
        cout << "Group member not in right position after rotation/movement" << endl;
    }
    // check combination of move, rotate, add (group orientation should reset, but net rotation of
    //         objects should not change)
    ModelInstance *d = new ModelInstance(m.data());
    d->setPosition(va);
    a->getPosition(v2);
    a->getOrientation(qtmp);
    grp.addObject(d);
    a->getPosition(v3);
    a->getOrientation(q3);
    // check net position/orientation of object already in group to ensure it didn't change
    if (!q_vec_equals(v2,v3)) {
        errors++;
        cout << "Group member moved when new one added" << endl;
    }
    if (!q_equals(qtmp,q3)) {
        errors++;
        cout << "Group member changed orientation when new one added" << endl;
    }
    // check position/orientation of new object to see if it changed
    d->getPosition(v2);
    if (!q_vec_equals(v2,va)) {
        errors++;
        cout << "Object changed position when added to group" << endl;
        q_vec_print(va);
        q_vec_print(v2);
    }
    d->getOrientation(qtmp);
    if (!q_equals(qtmp,qid)) {
        errors++;
        cout << "Object changed orientation when added to group" << endl;
    }
    // check resulting position/orientation of group
    grp.getPosition(v2);
    if (!q_vec_equals(v1,v2)) {
        errors++;
        cout << "Group position not set correctly when new object added" << endl;
    }
    grp.getOrientation(qtmp);
    if (!q_equals(qtmp,q2)) {
        errors++;
        cout << "Group orientation set incorrectly after new object added" << endl;
    }
    // check remove object (resets group rotation, changes center position)
    grp.setOrientation(q1);
    q_vec_type vbtmp;
    q_type qbtmp;
    a->getPosition(v2);
    a->getOrientation(qtmp);
    b->getPosition(vbtmp);
    b->getOrientation(qbtmp);
    grp.removeObject(b);
    // check net position/orientation of object still in group to ensure it didn't change
    a->getPosition(v3);
    a->getOrientation(q3);
    if (!q_vec_equals(v2,v3)) {
        errors++;
        cout << "Group member moved when one removed" << endl;
    }
    if (!q_equals(qtmp,q3)) {
        errors++;
        cout << "Group member changed orientation when one removed" << endl;
    }
    // check net position/orientation of object removed to ensure it didn't change
    b->getPosition(v3);
    b->getOrientation(q3);
    if (!q_vec_equals(v3,vbtmp)) {
        errors++;
        cout << "Object position changed when removed from group" << endl;
    }
    if (!q_equals(q3,qbtmp)) {
        errors++;
        cout << "Object orientation changed when removed from group" << endl;
    }
    // check group center and orientation after removal
    grp.getPosition(v3);
    if (!q_vec_equals(v1,v3)) {
        errors++;
        cout << "Group position wrong after removal." << endl;
        q_vec_print(v1);
        q_vec_print(v3);
    }
    grp.getOrientation(qtmp);
    if (!q_equals(qtmp,q1)) {
        errors++;
        cout << "Group orientation wrong after removal." << endl;
    }
    delete b; // object group deletes everything in it... the only one we need to worry about it b
                // since we remove b to test the remove function
    return errors;
}


//#########################################################################
inline int testObjectGroupSubGroup() {
    ObjectGroup *g1 = new ObjectGroup(), *g2 = new ObjectGroup();
    int errors = 0;
    g1->addObject(g2);
    g1->addObject(g1);
    if (g1->getParent() != NULL) {
        errors++;
        cout << "Object group can be its own parent" << endl;
    }
    if (g1->getSubObjects()->size() != 1) {
        errors++;
        cout << "Object group has incorrect number of children" << endl;
    }
    if (g1->numInstances() != 0) {
        errors++;
        cout << "Object group has instances when only empty group added" << endl;
    }
    g2->addObject(g1);
    if (g1->getParent() != NULL) {
        errors++;
        cout << "Object group can be its own grandparent" << endl;
    }
    if (g2->getSubObjects()->size() != 0) {
        errors++;
        cout << "Object is in its child's children list" << endl;
    }
    delete g1;
    return errors;
}


//#########################################################################
inline int testObjectGroup() {
    int errors = 0;
    errors += testNewObjectGroup();
    if (errors == 0) {
        errors += testObjectGroupActions();
        errors += testObjectGroupSubGroup();
    }
    cout << "Found " << errors << " errors in ObjectGroup." << endl;
    return errors;
}
