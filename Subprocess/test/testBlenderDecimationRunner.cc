

#include "testqt.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QtCore/QCoreApplication>
#include <subprocessutils.h>
#include <blenderdecimationrunner.h>
#include <sketchmodel.h>
#include <PQP.h>

#define FILENAME  (QDir::currentPath() + "/models/1m1j.obj")

class BlenderTest : public Test
{
public:
    BlenderTest(DecimationType::Type t, double amt);
    virtual ~BlenderTest() {}
    virtual void setUp();
    virtual SubprocessRunner *getRunner() { return runner; }
    virtual int testResults();
private:
    SubprocessRunner *runner;
    DecimationType::Type type;
    double amount;
};

BlenderTest::BlenderTest(DecimationType::Type t, double amt) :
    Test(),
    runner(NULL),
    type(t),
    amount(amt)
{
}

void BlenderTest::setUp()
{
    QString filename = FILENAME;

    if (type == DecimationType::PERCENT)
    {
        runner = SubprocessUtils::simplifyObjFileByPercent(filename,amount);
    }
    else if (type == DecimationType::POLYGON_COUNT)
    {
        runner = SubprocessUtils::simplifyObjFile(filename,static_cast<int>(amount));
    }
    else
        throw "Unknown Decimation Type";
}

int BlenderTest::testResults()
{
    QString filename = FILENAME;
    SketchModel m(1,1);
    m.addConformation(filename,filename);
    int nTris = m.getCollisionModel(0)->num_tris;
    qDebug() << "There are " << nTris << " triangles in the original.";
    QDir dir(QDir::current().absoluteFilePath("models"));
    QStringList files = dir.entryList();
    for (int i = 0; i < files.size(); i++)
    {
        QString name = dir.absoluteFilePath(files.at(i));
        if (name.startsWith(filename + ".decimated") &&
                files.at(i).endsWith(".obj"))
        {
            m.addConformation(files.at(i),name);
        }
    }
    if (m.getNumberOfConformations() == 1)
    {
        qDebug() << "Error decimating model file.";
        return 1;
    }
    int newNTris = m.getCollisionModel(1)->num_tris;
    qDebug() << "There are " << newNTris << " triangles in the result.";
    if (type == DecimationType::PERCENT)
    {
        double pct = static_cast<double>(newNTris) / static_cast<double>(nTris);
        if (Q_ABS(pct - amount) > .01)
        {
            qDebug() << "Wrong decimation percentage.";
            return 1;
        }
    }
    else if (type == DecimationType::POLYGON_COUNT)
    {
        if (newNTris > 1.1 * amount)
        {
            qDebug() << "Too many triangles in resulting model: " << newNTris;
            return 1;
        }
    }
    QString file = m.getFileNameFor(1,ModelResolution::FULL_RESOLUTION);
    QFile(file).remove();
    QFile(file.mid(0,file.size()-3) + "mtl").remove();
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);
    QDir::setCurrent(app.applicationDirPath());
    qDebug() << "Temp path: " << QDir::tempPath();

    // set required fields to use QSettings API (these specify the location of the settings)
    // used to locate subprocess files
    app.setApplicationName("Sketchbio");
    app.setOrganizationName("UNC Computer Science");
    app.setOrganizationDomain("sketchbio.org");

    BlenderTest b1(DecimationType::PERCENT,.5), b2(DecimationType::POLYGON_COUNT, 5000);
    TestQObject *test = new TestQObject(app,b1);
    TestQObject *test2 = new TestQObject(app,b2);

    QObject::connect(test, SIGNAL(finished()), test2, SLOT(start()));
    QObject::connect(test2, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, test, SLOT(start()));
    return app.exec();
}
