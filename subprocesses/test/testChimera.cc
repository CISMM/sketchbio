/*
 *
 * This is a test of the ChimeraOBJMaker subprocess class to ensure that
 * it generates obj files.
 *
 */


#include <subprocessrunner.h>
#include <subprocessutils.h>
#include "testqt.h"
#include <QTimer>
#include <QFile>
#include <QtCore/QCoreApplication>

#define FILENAME "1m1j.obj"

class ChimeraTest : public Test
{
public:
    ChimeraTest(int threshold) : Test(), runner(NULL), thresh(threshold) {}
    virtual ~ChimeraTest() {}
    virtual void setUp();
    virtual SubprocessRunner *getRunner() { return runner; }
    virtual int testResults();
private:
    SubprocessRunner *runner;
    int thresh;
};

void ChimeraTest::setUp()
{
    QFile f("1m1j.obj");
    if (f.exists())
        f.remove();
    runner = SubprocessUtils::makeChimeraOBJFor("1m1j",FILENAME,thresh);
}

int ChimeraTest::testResults()
{
    QFile f(FILENAME);
    if (f.exists())
    {
        f.remove();
        return 0;
    }
    else
        return 1;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);

    // set required fields to use QSettings API (these specify the location of the settings)
    // used to locate subprocess files
    app.setApplicationName("Sketchbio");
    app.setOrganizationName("UNC Computer Science");
    app.setOrganizationDomain("sketchbio.org");

    ChimeraTest myTest(0), myTest2(3);
    TestQObject *test = new TestQObject(app,myTest);
    TestQObject *test2 = new TestQObject(app,myTest2);

    QObject::connect(test, SIGNAL(finished()), test2, SLOT(start()));
    QObject::connect(test2, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, test, SLOT(start()));
    return app.exec();
}
