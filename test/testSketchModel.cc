#include <sketchmodel.h>

#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>

#include <QScopedPointer>
#include <QDir>
#include <QDebug>


int testUseCount();
int testAddResolutionFileAndChangeResolutions();
int testAddConformations();
int testTranslateAndRotateBounds();

// The main method for the program that tests the SketchModel class
int main(int argc, char *argv[])
{
    int errors = 0;

    // setup
    QDir dir = QDir::current();
    // change the working dir to the dir where the test executable is
    QString executable = dir.absolutePath() + "/" + argv[0];
    int last = executable.lastIndexOf("/");
    if (QDir::setCurrent(executable.left(last)))
        dir = QDir::current();
    qDebug() << "Working directory: " <<
                 dir.absolutePath().toStdString().c_str();

    // tests
    errors += testAddConformations();
    errors += testTranslateAndRotateBounds();
    errors += testUseCount();
    errors += testAddResolutionFileAndChangeResolutions();

    // return result of tests
    return errors;
}

int testUseCount()
{
    int retVal = 0;
    QScopedPointer< SketchModel > model(new SketchModel(1,1,false));
    QString filename = "models/1m1j.obj";
    // add some conformations
    model->addConformation(filename,filename);
    model->addConformation(filename,filename);

    model->incrementUses(0);
    if (model->getNumberOfUses(0) != 1)
    {
        retVal++;
        qDebug() << "Incrementing use count failed.";
    }
    model->incrementUses(1);
    if (model->getNumberOfUses(1) != 1)
    {
        retVal++;
        qDebug() << "Incrementing use count of other failed.";
    }
    model->incrementUses(0);
    model->incrementUses(0);
    model->incrementUses(1);
    model->incrementUses(0);
    model->incrementUses(1);
    model->incrementUses(0);
    if (model->getNumberOfUses(0) != 5 || model->getNumberOfUses(1) != 3)
    {
        retVal++;
        qDebug() << "Multi-incrementing number of uses failed.";
    }
    model->decrementUses(0);
    if (model->getNumberOfUses(0) != 4)
    {
        retVal++;
        qDebug() << "Decrementing use count failed.";
    }
    model->decrementUses(1);
    model->decrementUses(1);
    model->decrementUses(1);
    if (model->getNumberOfUses(1) != 0)
    {
        retVal++;
        qDebug() << "Multi-decrementing use count failed.";
    }

    return retVal;
}

int testAddResolutionFileAndChangeResolutions()
{
    int retVal = 0;
    QScopedPointer< SketchModel > model(new SketchModel(1,1,false));
    QString filename = "models/1m1j.obj";
    // add some conformations
    model->addConformation(filename,filename);
    model->addConformation(filename,filename);

    // create a lower resolution model
    vtkSmartPointer< vtkCubeSource > cube =
            vtkSmartPointer< vtkCubeSource >::New();
    cube->SetBounds(-1,1,-1,1,-1,1);
    cube->Update();
    filename = ModelUtilities::createFileFromVTKSource(cube,"models/cube_for_model_test");
    model->addSurfaceFileForResolution(0,ModelResolution::SIMPLIFIED_1000,filename);
    int points1 = model->getVTKSource(0)->GetOutput()->GetNumberOfPoints();
    model->setReslutionForConfiguration(0,ModelResolution::SIMPLIFIED_1000);
    int points2 = model->getVTKSource(0)->GetOutput()->GetNumberOfPoints();
    if (points1 == points2)
    {
        retVal++;
        qDebug() << "Switching model resolutions failed.";
    }
    model->setReslutionForConfiguration(0,ModelResolution::SIMPLIFIED_2000);
    if (model->getVTKSource(0)->GetOutput()->GetNumberOfPoints() != points2)
    {
        retVal++;
        qDebug() << "Resolution changed even though no file for given resolution.";
    }
    model->setReslutionForConfiguration(0,ModelResolution::FULL_RESOLUTION);
    if (points1 != model->getVTKSource(0)->GetOutput()->GetNumberOfPoints())
    {
        retVal++;
        qDebug() << "Setting resolution back to full failed.";
    }

    return retVal;
}

inline int testConformationAdded(SketchModel *model,int confNum)
{
    int retVal = 0;
    if (model->getNumberOfConformations() <= confNum)
    {
        retVal++;
        qDebug() << "Wrong number of conformations.";
        return retVal;
    }
    QString s = model->getFileNameFor(confNum,ModelResolution::FULL_RESOLUTION);
    if (s.length() == 0)
    {
        retVal++;
        qDebug() << "Conformation must have a full resolution filename.";
    }
    s = model->getSource(confNum);
    if (s.length() == 0)
    {
        retVal++;
        qDebug() << "Conformation has no source.";
    }
    if (model->getCollisionModel(confNum) == NULL)
    {
        retVal++;
        qDebug() << "Conformation has NULL collision model.";
    }
    if (model->getNumberOfUses(confNum) != 0)
    {
        retVal++;
        qDebug() << "Conformation has nonzero number of uses.";
    }
    if (model->getResolutionLevel(confNum) != ModelResolution::FULL_RESOLUTION)
    {
        retVal++;
        qDebug() << "Conformation has wrong initial resolution level.";
    }
    if (model->getVTKSource(confNum) == NULL)
    {
        retVal++;
        qDebug() << "Conformation has no vtk data!.";
    }
    return retVal;
}

int testAddConformations()
{
    int retVal = 0;
    QScopedPointer< SketchModel > model(new SketchModel(1,1,false));
    QString filename = "models/1m1j.obj";
    // test initial conformation
    model->addConformation(filename,filename);
    retVal += testConformationAdded(model.data(),0);
    // add another and test it
    model->addConformation(filename,filename);
    retVal += testConformationAdded(model.data(),1);

    // add a conformation with few triangles to test that
    // the lower resolutions get filled in
    vtkSmartPointer< vtkSphereSource > sphere =
            vtkSmartPointer< vtkSphereSource >::New();
    sphere->SetRadius(4);
    sphere->Update();
    filename = ModelUtilities::createFileFromVTKSource(sphere,"models/sphere_for_model_test");
    model->addConformation(filename,filename);
    retVal += testConformationAdded(model.data(),2);
    // test if the lower resolutions of the model got filled in with the same filename
    if (model->getFileNameFor(2,ModelResolution::SIMPLIFIED_FULL_RESOLUTION) != filename
            || model->getFileNameFor(2,ModelResolution::SIMPLIFIED_5000) != filename
            || model->getFileNameFor(2,ModelResolution::SIMPLIFIED_2000) != filename
            || model->getFileNameFor(2,ModelResolution::SIMPLIFIED_1000) != filename)
    {
        retVal++;
        qDebug() << "Model with few triangles used at all resolutions";
    }
    retVal += testConformationAdded(model.data(),2);

    return retVal;
}

// tests if the given bounding box is centered on zero
inline int testBBCentered(double bb[6])
{
    int retVal = 0;
    if (bb[0] >= 0 || bb[2] >= 0 || bb[4] >= 0)
    {
        qDebug() << "Bounding box minimum greater than 0";
        retVal++;
    }
    if (bb[1] <= 0 || bb[3] <= 0 || bb[5] <= 0)
    {
        qDebug() << "Bounding box maximum less than 0";
        retVal++;
    }
    if (Q_ABS(bb[0] + bb[1]) > 10e-5)
    {
        qDebug() << "Bounds not centered in x: " << bb[0] << " " << bb[1];
        retVal++;
    }
    if (Q_ABS(bb[2] + bb[3]) > 10e-5)
    {
        qDebug() << "Bounds not centered in y: " << bb[2] << " " << bb[3];
        retVal++;
    }
    if (Q_ABS(bb[4] + bb[5]) > 10e-5)
    {
        qDebug() << "Bounds not centered in z: " << bb[4] << " " << bb[5];
        retVal++;
    }
    return retVal;
}

// this ensures that the center of the bounding box for a model
// is centered on the model's local origin
int testTranslateAndRotateBounds()
{
    int retVal = 0;
    // test a model with rotate-to-principle-components off
    QScopedPointer< SketchModel > model(new SketchModel(1,1,false));
    QString filename = "models/1m1j.obj";
    model->addConformation(filename,filename);
    double bb[6];
    model->getVTKSource(0)->GetOutput()->GetBounds(bb);
    retVal += testBBCentered(bb);
    double volumeNoRotation = 8 * bb[1] * bb[3] * bb[5];
    // test a model with rotate-to-principle-components on
    model.reset(new SketchModel(1,1,true));
    model->addConformation(filename,filename);
    model->getVTKSource(0)->GetOutput()->GetBounds(bb);
    retVal += testBBCentered(bb);
    double volumeWithRotation = 8 * bb[1] * bb[3] * bb[5];
    if (volumeWithRotation > volumeNoRotation)
    {
        retVal++;
        qDebug() << "Rotation did not shrink bounding volume.";
    }
    return retVal;
}