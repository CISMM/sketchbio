/*
 *
 * This is a test of the ModelFromPDBRunner class.
 *
 */

#include <vector>

#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QDir>
#include <QDebug>

#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>

#include <modelmanager.h>
#include <modelutilities.h>
#include <sketchmodel.h>
#include <sketchproject.h>
#include <subprocessrunner.h>
#include <subprocessutils.h>

#include "testqt.h"

class TestModelFromPDB : public Test
{
public:
    TestModelFromPDB(QString pdb);
    virtual ~TestModelFromPDB();
    virtual bool useFinishedEvent() { return false; }
    virtual void setUp();
    virtual SubprocessRunner *getRunner() { return runner; }
    virtual int testResults();
private:
    QString pdbId;
    SubprocessRunner *runner;
    vtkSmartPointer< vtkRenderer > renderer;
    SketchProject *proj;
};

TestModelFromPDB::TestModelFromPDB(QString pdb) :
    Test(),
    pdbId(pdb),
    runner(NULL),
    renderer(vtkSmartPointer< vtkRenderer >::New()),
    proj(new SketchProject(renderer.GetPointer()))
{
    proj->setProjectDir(QDir::currentPath());
}

TestModelFromPDB::~TestModelFromPDB()
{
    delete proj;
}

void TestModelFromPDB::setUp()
{
    runner = SubprocessUtils::loadFromPDB(proj,pdbId,"");
}

int TestModelFromPDB::testResults()
{
    SketchModel *model = proj->getModelManager()->getModel(
                ModelUtilities::createSourceNameFor(pdbId,""));
    if (model == NULL)
        return 1;
    std::vector< ModelResolution::ResolutionType > resolution;
    resolution.push_back(ModelResolution::FULL_RESOLUTION);
    resolution.push_back(ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
    resolution.push_back(ModelResolution::SIMPLIFIED_5000);
    resolution.push_back(ModelResolution::SIMPLIFIED_2000);
    resolution.push_back(ModelResolution::SIMPLIFIED_1000);

    int numArrays = model->getVTKSource(0)->GetOutput()->
            GetPointData()->GetNumberOfArrays();
    int errors = 0;

    if (numArrays < 9) {
        errors++;
    }

    for (unsigned i = 0; i < resolution.size(); i++)
    {
        if (!model->hasFileNameFor(0,resolution[i]))
        {
            qDebug() << "Model has no file for resolution " << i;
            return 1;
        }
        QString fName =model->getFileNameFor(0,resolution[i]);
        QFile f(fName);
        if (!f.exists())
        {
            qDebug() << "Model file for resolution " << i << ": " << f.fileName() <<
                        " does not exist.";
            return 1;
        }
        f.remove();
        f.setFileName(fName.mid(0,fName.length()-3) + "mtl");
        f.remove();
    }
    return errors;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);
    QDir::setCurrent(app.applicationDirPath());

    // set required fields to use QSettings API (these specify the location of the settings)
    // used to locate subprocess files
    app.setApplicationName("Sketchbio");
    app.setOrganizationName("UNC Computer Science");
    app.setOrganizationDomain("sketchbio.org");

    TestModelFromPDB test("1m1j");
    TestQObject obj(app, test);

    QObject::connect(&obj, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, &obj, SLOT(start()));

    return app.exec();
}
