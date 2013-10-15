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
    TestModelFromPDB();
    virtual ~TestModelFromPDB();
    virtual bool useFinishedEvent() { return false; }
    virtual SubprocessRunner* getRunner() { return runner; }
    virtual int testResults();
    virtual QString getSource() = 0;
protected:
    SubprocessRunner *runner;
    vtkSmartPointer< vtkRenderer > renderer;
    SketchProject *proj;
};

class TestModelFromPDBID : public TestModelFromPDB
{
public:
    TestModelFromPDBID(QString pdb);
    virtual ~TestModelFromPDBID() {}
    virtual void setUp();
    virtual QString getSource();
private:
    QString pdbId;
};

class TestModelFromPDBFile : public TestModelFromPDB
{
public:
    TestModelFromPDBFile(QString fname);
    virtual ~TestModelFromPDBFile() {}
    virtual void setUp();
    virtual QString getSource();
private:
    QString filename;
};

TestModelFromPDB::TestModelFromPDB() :
    Test(),
    runner(NULL),
    renderer(vtkSmartPointer< vtkRenderer >::New()),
    proj(new SketchProject(renderer.GetPointer(),QDir::currentPath()))
{
}

TestModelFromPDBID::TestModelFromPDBID(QString pdb) :
    TestModelFromPDB(),
    pdbId(pdb)
{
}

TestModelFromPDBFile::TestModelFromPDBFile(QString fname) :
    TestModelFromPDB(),
    filename(fname)
{
}

TestModelFromPDB::~TestModelFromPDB()
{
    delete proj;
}

void TestModelFromPDBID::setUp()
{
    runner = SubprocessUtils::loadFromPDBId(proj,pdbId,"",false);
}

void TestModelFromPDBFile::setUp()
{
    runner = SubprocessUtils::loadFromPDBFile(proj,filename,"",true);
}

QString TestModelFromPDBID::getSource()
{
    return ModelUtilities::createSourceNameFor(pdbId,"");
}

QString TestModelFromPDBFile::getSource()
{
    return filename;
}

int TestModelFromPDB::testResults()
{
    SketchModel *model = proj->getModelManager()->getModel(getSource());
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
    QString appPath = app.applicationDirPath();
#if defined(_WIN32)
    int idx = appPath.lastIndexOf("test");
    appPath = appPath.mid(0,idx+4);
#endif
    QDir::setCurrent(appPath);

    // set required fields to use QSettings API (these specify the location of the settings)
    // used to locate subprocess files
    app.setApplicationName("Sketchbio");
    app.setOrganizationName("UNC Computer Science");
    app.setOrganizationDomain("sketchbio.org");

    TestModelFromPDBID test("3mfp"); // use a small model for quicker processing
    TestModelFromPDBFile test2(QDir::currentPath() + "/models/4fun.pdb");
    TestQObject obj(app, test), obj2(app,test2);

    QObject::connect(&obj, SIGNAL(finished()), &obj2, SLOT(start()));
    QObject::connect(&obj2, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, &obj, SLOT(start()));

    return app.exec();
}
