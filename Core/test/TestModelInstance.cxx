#include <iostream>
using std::cout;
using std::endl;

#include <quat.h>

#include <QScopedPointer>
#include <QDir>

#include <sketchtests.h>
#include <sketchmodel.h>
#include <sketchobject.h>
#include <modelinstance.h>

#include "TestCoreHelpers.h"

// declare these -- they are at the bottom of the file, but I wanted main at the top
//                  where I can find it
int testModelInstance();

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
    errors += testModelInstance();
    return errors;
}




//#########################################################################
// tests if the ModelInstance initialized correctly (note: if methods used
//  either here or in testNewSketchObject are overridden, this should not be
//  called, use your own test funciton to check initial state).  This does
//  not modify the ModelInstance, just checks to see if the initial state is
//  set up correctly
inline int testNewModelInstance(ModelInstance *obj) {
    int errors = TestCoreHelpers::testNewSketchObject(obj);
    if (obj->numInstances() != 1) {
        errors++;
        cout << "How is one instance more than one instance?" << endl;
    }
    if (obj->getActor() == NULL) {
        errors++;
        cout << "Actor for object is null!" << endl;
    }
    if (obj->getModel() == NULL) {
        errors++;
        cout << "Model for object is null" << endl;
    }
    if (obj->getSubObjects() != NULL) {
        errors++;
        cout << "List of sub-objects isn't null" << endl;
    }
    return errors;
}

//#########################################################################
// tests if the behavioral methods of a ModelInstance are working. this includes
// get/set methods.  Assumes a newly constructed ModelInstance with no modifications
// made to it yet.
inline int testModelInstanceActions(ModelInstance *obj) {
    int errors = TestCoreHelpers::testSketchObjectActions(obj);
    // test set wireframe / set solid... can only test if mapper changed easily, no testing
    // the vtk pipeline to see if it is actually wireframed

    // test the bounding box .... assumes a radius 4 sphere
    double bb[6];
    q_vec_type pos;
    obj->getPosition(pos); // position set in getSketchObjectActions
    obj->getBoundingBox(bb);
    for (int i = 0; i < 3; i++) {
        // due to tessalation, bounding box has some larger fluctuations.
        if (Q_ABS(bb[2*i+1] - bb[2*i] - 8) > .025) {
            errors++;
            cout << "Object bounding box wrong" << endl;
        }
        // average of fluctuations has larger fluctuation
        // note model instance bounding box is now an OBB that fits the
        // untransformed model...
        if (Q_ABS((bb[2*i+1] +bb[2*i])/2) > .05) {
            errors++;
            cout << "Object bounding box position wrong" << endl;
        }
    }
    return errors;
}

//#########################################################################
inline int testModelInstance() {
    // set up
    QScopedPointer<SketchModel> model(TestCoreHelpers::getSphereModel());
    QScopedPointer<ModelInstance> obj(new ModelInstance(model.data()));
    int errors = 0;

    if (model->getNumberOfUses(0) != 1)
    {
        errors++;
        cout << "Did not increment model uses." << endl;
    }

    // test that a new ModelInstance is right
    errors += testNewModelInstance(obj.data());
    errors += testModelInstanceActions(obj.data());
    // maybe
    // testCollisions(obj,obj2);

    // clean up
    cout << "Found " << errors << " errors in ModelInstance." << endl;

    obj.reset();

    if (model->getNumberOfUses(0) != 0)
    {
        errors++;
        cout << "Did not decrement model uses." << endl;
    }

    return errors;
}
