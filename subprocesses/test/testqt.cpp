#include "testqt.h"

#include <subprocessrunner.h>
#include <QtCore/QCoreApplication>

TestQObject::TestQObject(QCoreApplication &app,Test &t) :
    QObject(&app),
    application(app),
    test(t)
{
}

TestQObject::~TestQObject()
{
}

void TestQObject::start()
{
    test.setUp();
    SubprocessRunner *runner = test.getRunner();
    if (test.useFinishedEvent())
        runner->connect(runner,SIGNAL(finished(bool)),this,SLOT(runTests(bool)));
    else
        runner->connect(runner,SIGNAL(destroyed()),this,SLOT(runTests()));
    runner->start();
}

void TestQObject::runTests(bool success)
{
    int result = test.testResults();
    if (!success || result != 0)
        application.exit(result);
    else
        emit finished();
}

void TestQObject::runTests()
{
    runTests(true);
}
