/*
 * This class tests the Blender animation subprocess runner.  Since there is no
 * way to test the video other than a binary diff, this simply tests if the
 * video is created at all.
 */

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QScopedPointer>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

#include <sketchproject.h>
#include <projecttoxml.h>
#include <subprocessutils.h>

#include "testqt.h"

#define ANIMATION_FILE "test_animation.avi"

class TestBlender : public Test
{
public:
    TestBlender(SketchBio::Project *proj);
    virtual ~TestBlender() {}
    virtual void setUp();
    virtual SubprocessRunner *getRunner() { return runner; }
    virtual int testResults();
private:
    SketchBio::Project *project;
    SubprocessRunner *runner;
};

TestBlender::TestBlender(SketchBio::Project *proj) :
    project(proj)
{
}

void TestBlender::setUp()
{
    QFile f(ANIMATION_FILE);
    if (f.exists())
        f.remove();
    runner = SubprocessUtils::createAnimationFor(project,ANIMATION_FILE);
}

int TestBlender::testResults()
{
    QFile f(ANIMATION_FILE);
    if (!f.exists())
    {
        qDebug() << "Failed to create animation.";
        return 1;
    }
    else
    {
        return 0;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);

    // set required fields to use QSettings API (these specify the location of the settings)
    // used to locate subprocess files
    app.setApplicationName("Sketchbio");
    app.setOrganizationName("UNC Computer Science");
    app.setOrganizationDomain("sketchbio.org");

	QString appPath = app.applicationDirPath();
#if defined(_WIN32)
	int idx = appPath.lastIndexOf("test");
	appPath = appPath.mid(0,idx+4);
#endif
    QDir::setCurrent(appPath);
    qDebug() << "Working directory: " << QDir::currentPath();
	qDebug() << "Temp dir: " << QDir::tempPath();

    vtkSmartPointer< vtkRenderer > r =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchBio::Project > proj(
                new SketchBio::Project(r,"projects/animation"));

    QDir d(proj->getProjectDir());
    QDir d2(d.absoluteFilePath("anim"));
    d2.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QStringList files = d2.entryList();
    for (int idx = 0; idx < files.length(); idx++)
    {
        QFile f(d2.absoluteFilePath(files[idx]));
        if (!f.remove())
        {
            qDebug() << "Could not remove " << files[idx];
        }
    }
    d.rmdir("anim");

    vtkSmartPointer< vtkXMLDataElement > root =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                vtkXMLUtilities::ReadElementFromFile("projects/animation/project.xml")
                );

    ProjectToXML::XML_Read_Status status = ProjectToXML::xmlToProject(proj.data(),root);
    if (status == ProjectToXML::XML_TO_DATA_FAILURE)
    {
        qDebug() << "Failed to load project.";
        return 1;
    }

    TestBlender test(proj.data());

    TestQObject testObj(app,test), testObj2(app,test);

    QObject::connect(&testObj,SIGNAL(finished()),&testObj2,SLOT(start()));
    QObject::connect(&testObj2,SIGNAL(finished()),&app,SLOT(quit()));
    QTimer::singleShot(0,&testObj,SLOT(start()));
    return app.exec();
}
