/*
 *
 * This is a test of the ChimeraOBJMaker subprocess class to ensure that
 * it generates obj files.
 *
 */

#include <iostream>
using std::cout;
using std::endl;

#include <QTimer>
#include <QFile>
#include <QDir>
#include <QtCore/QCoreApplication>

#include <subprocessrunner.h>
#include <subprocessutils.h>

#include "testqt.h"

#define FILENAME (QDir::tempPath() + "/1m1j.obj")

class ChimeraTest : public Test
{
public:
    ChimeraTest(int threshold, QString del = "");
    virtual ~ChimeraTest() {}
    virtual void setUp();
    virtual SubprocessRunner *getRunner() { return runner; }
    virtual int testResults();
private:
    SubprocessRunner *runner;
    int thresh;
    QString toDelete;
};

ChimeraTest::ChimeraTest(int threshold, QString del) :
    Test(),
    runner(NULL),
    thresh(threshold),
    toDelete(del)
{
}

void ChimeraTest::setUp()
{
    QFile f(FILENAME);
    if (f.exists())
        f.remove();
    runner = SubprocessUtils::makeChimeraOBJFor("1m1j",FILENAME,thresh,toDelete);
}

int ChimeraTest::testResults()
{
    QFile f(FILENAME);
    if (f.exists())
    {
        f.remove();
        QString name = f.fileName();
        QFile(name.mid(0,name.length()-3)+"mtl").remove();
        return 0;
    }
    else
        return 1;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);
    cout << "Temp dir: " << QDir::tempPath().toStdString() << endl;

    // set required fields to use QSettings API (these specify the location of the settings)
    // used to locate subprocess files
    app.setApplicationName("Sketchbio");
    app.setOrganizationName("UNC Computer Science");
    app.setOrganizationDomain("sketchbio.org");

    ChimeraTest myTest(0), myTest2(3), myTest3(4,"BCD");
    TestQObject *test = new TestQObject(app,myTest);
    TestQObject *test2 = new TestQObject(app,myTest2);
    TestQObject *test3 = new TestQObject(app,myTest3);

    QObject::connect(test, SIGNAL(finished()), test2, SLOT(start()));
    QObject::connect(test, SIGNAL(finished()), test, SLOT(deleteLater()));

    QObject::connect(test2, SIGNAL(finished()), test3, SLOT(start()));
    QObject::connect(test2, SIGNAL(finished()), test2, SLOT(deleteLater()));

    QObject::connect(test3, SIGNAL(finished()), test3, SLOT(deleteLater()));
    QObject::connect(test3, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, test, SLOT(start()));
    return app.exec();
}
