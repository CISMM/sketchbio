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
    errors += TestCoreHelpers::testNewModelInstance(obj.data());
    errors += TestCoreHelpers::testModelInstanceActions(obj.data());
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
