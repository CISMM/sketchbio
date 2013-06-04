#ifndef TESTQT_H
#define TESTQT_H

#include <QObject>

class QCoreApplication;
class SubprocessRunner;

class Test
{
public:
    virtual ~Test() {}
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

signals:
    void finished();
private:
    QCoreApplication &application;
    Test &test;
};

#endif // TESTQT_H
