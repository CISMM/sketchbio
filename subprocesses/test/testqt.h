#ifndef TESTQT_H
#define TESTQT_H

#include <QObject>

class QCoreApplication;
class SubprocessRunner;

class Test
{
public:
    virtual ~Test() {}
    // if true, then testResults will be called on the finished() event
    // of the runner, otherwise it will use destroyed()
    virtual bool useFinishedEvent() { return true; }
    virtual void setUp() = 0;
    virtual SubprocessRunner *getRunner() = 0;
    virtual int testResults() = 0;
};

class TestQObject : public QObject
{
    Q_OBJECT
public:
    TestQObject(QCoreApplication &app, Test &t);
    ~TestQObject();

public slots:
    void start();
private slots:
    void runTests(bool success);
    void runTests();

signals:
    void finished();
private:
    QCoreApplication &application;
    Test &test;
};

#endif // TESTQT_H
