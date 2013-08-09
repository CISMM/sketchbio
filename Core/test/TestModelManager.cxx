#include <iostream>
using std::cout;
using std::endl;
#define PRINT_ERROR_MESSAGE(x)  cout << x << endl

#include <QDir>
#include <QScopedPointer>

#include <vtkSmartPointer.h>
#include <vtkPlaneSource.h>

#include <modelutilities.h>
#include <sketchmodel.h>
#include <modelmanager.h>

#include "TestCoreHelpers.h"

int testAddModels();

int main(int argc, char *argv[])
{
    QDir dir = QDir::current();
    // change the working dir to the dir where the test executable is
    QString executable = dir.absolutePath() + "/" + argv[0];
    int last = executable.lastIndexOf("/");
    if (QDir::setCurrent(executable.left(last)))
        dir = QDir::current();
    cout << "Working directory: " <<
                 dir.absolutePath().toStdString().c_str() << endl;
    int errors = 0;
    errors += testAddModels();
    return errors;
}

int testAddModels()
{
    int errors = 0;
    ModelManager manager;
    if (manager.getNumberOfModels() != 0)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Wrong number of models on initialization");
    }
    SketchModel *model1 = TestCoreHelpers::getCubeModel();
    manager.addModel(model1);
    if (manager.getNumberOfModels() != 1)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Wrong number of models after first add");
    }
    QScopedPointer< SketchModel > model2( TestCoreHelpers::getCubeModel());
    if (manager.addModel(model2.data()) != model1)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Duplicate models allowed in model manager.");
    }
    if (manager.getNumberOfModels() != 1)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Model added when it was a duplicate");
    }
    model2.reset(TestCoreHelpers::getSphereModel());
    SketchModel *model3 = manager.makeModel(
                model2->getSource(0),model2->getFileNameFor(
                          0,ModelResolution::FULL_RESOLUTION),1.0,1.0);
    if (model3 == model1)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Models the same!");
    }
    if (manager.getNumberOfModels() != 2)
    {
        errors++;
        PRINT_ERROR_MESSAGE("No model made!");
    }
    SketchModel *model4 = manager.makeModel(
                     model1->getSource(0),model3->getFileNameFor(
                         0,ModelResolution::FULL_RESOLUTION),1.0,1.0);
    if (model4 != model1)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Model created when it should not have been.");
    }
    if (manager.getNumberOfModels() != 2)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Wrong number of models after makeModel returns existing.");
    }
    vtkSmartPointer< vtkPlaneSource > plane=
            vtkSmartPointer< vtkPlaneSource >::New();
    plane->Update();
    QDir dir = QDir::current();
    model4 = manager.modelForVTKSource(
                     model1->getSource(0),plane,1.0,dir);
    if (model4 != model1)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Model created from vtk when it should not have been.");
    }
    if (manager.getNumberOfModels() != 2)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Wrong number of models after model for"
                            " vtk source returns existing.");
    }
    model4 = manager.modelForVTKSource("abc123",plane,1.0,dir);
    if (model2.data() == model1 || model2.data() == model3)
    {
        errors++;
        PRINT_ERROR_MESSAGE("Model not created for vtk source");
    }
    return errors;
}
