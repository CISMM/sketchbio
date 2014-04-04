/*
 *
 * This file tests the functionality of SketchProject.  It is not done yet,
 * but is being added as a place to put a regression test for a bug I have
 * had to fix twice now.
 *
 */

#include <iostream>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include <QScopedPointer>
#include <QDir>

#include <modelutilities.h>
#include <sketchmodel.h>
#include <sketchproject.h>

#include <test/TestCoreHelpers.h>

#define PROJECT_DIR "projdir"


int testFileRefreshBug(const QDir &projectDir);

int cleanProjectDir(QDir &projectDir);

int main(int argc, char *argv[])
{
    QDir dir = QDir::current();
    // change the working dir to the dir where the test executable is
    // needed so that we know where the project directory is
    QString executable = dir.absolutePath() + "/" + argv[0];
    int last = executable.lastIndexOf("/");
    if (QDir::setCurrent(executable.left(last)))
        dir = QDir::current();
    std::cout << "Working directory: " <<
                 dir.absolutePath().toStdString().c_str() << std::endl;

    // make the project dir
    QDir projDir(dir.absoluteFilePath(PROJECT_DIR));
    if (!projDir.exists())
    {
        if (! dir.mkdir(PROJECT_DIR) )
        {
            std::cout << "Failed to create project directory.";
            return 1;
        }
    }

    int retVal = 0;
    retVal += cleanProjectDir(projDir);
    try
    {
        retVal += testFileRefreshBug(projDir);
    }
    catch (const char *c) // known to throw this in case it tests for
    {
        std::cout << "Exception: " << c << std::endl;
        retVal++;
    }

    return retVal;
}


/*
 * This function tests for recurrence of the bug that when files are
 * dynamically added to the project directory during execution, the
 * project code sometimes fails to find them.  Solve by using QFile::exists()
 * instead of other methods.  (Tests the getFileInProjDir code path)
 */
int testFileRefreshBug(const QDir &projectDir)
{
    vtkSmartPointer< vtkRenderer > renderer =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer<SketchBio::Project> proj(
                new SketchBio::Project(renderer,projectDir.absolutePath()));
    SketchModel *model = NULL;

    vtkSmartPointer< vtkSphereSource > sphere1 =
            vtkSmartPointer< vtkSphereSource >::New();
    sphere1->SetRadius(4);
    sphere1->Update();
    QString file1 = ModelUtilities::createFileFromVTKSource(sphere1,"model1",
                                                            projectDir.absolutePath());
    QString newFName;
    if (!proj->getFileInProjDir(file1,newFName))
    {
        std::cout << "Failed to create first model.";
        return 1;
    }
    vtkSmartPointer< vtkSphereSource > sphere2 =
            vtkSmartPointer< vtkSphereSource >::New();
    sphere2->SetRadius(10);
    sphere2->SetCenter(50,50,-30);
    sphere2->Update();
    QString file2 = ModelUtilities::createFileFromVTKSource(sphere2,"model2",
                                                            projectDir.absolutePath());
    if (!proj->getFileInProjDir(file2,newFName))
    {
        std::cout << "Failed to create second model.";
        return 1;
    }
    QFile(file1).remove();
    QFile(file2).remove();
    return 0;
}

/*
 * This function cleans out the project directory between tests.  Tests should
 * clean up after themselves, but occasionally that may not be possible and this
 * will take care of it instead.
 */
int cleanProjectDir(QDir &projectDir)
{
    QDir::Filters oldFilter = projectDir.filter();
    projectDir.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    QStringList list = projectDir.entryList();
    for (int i = 0; i < list.size(); i++)
    {
        QFile f(projectDir.absoluteFilePath(list.at(i)));
        if (!f.remove())
        {
            std::cout << "Failed to clean up project dir.";
            return 1;
        }
    }
    projectDir.setFilter(oldFilter);
    return 0;
}
