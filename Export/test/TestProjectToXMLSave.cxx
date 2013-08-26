#include <iostream>

#include <QDir>

#include <vtkRenderer.h>
#include <vtkXMLUtilities.h>
#include <vtkXMLDataElement.h>

#include <sketchioconstants.h>
#include <sketchmodel.h>
#include <modelmanager.h>
#include <springconnection.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <structurereplicator.h>
#include <transformequals.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <projecttoxml.h>

#include "CompareBeforeAndAfter.h"
#include "MakeTestProject.h"

using std::cout;
using std::endl;

#define SAVE_TEST_DIR "test/test1"
#define LOAD_ONLY_TEST_DIR "test/test2"

#define PRINT_OUT_XML_TESTNUM -1

int saveLoadAndTest(SketchProject *proj, int testNum, bool writeToFile = false)
{
    int retVal = 0;
    vtkSmartPointer< vtkRenderer > r =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj2(
                new SketchProject(r,proj->getProjectDir()));

    vtkSmartPointer< vtkXMLDataElement > root =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                ProjectToXML::projectToXML(proj)
                );

    if (testNum == PRINT_OUT_XML_TESTNUM)
    {
        vtkXMLUtilities::FlattenElement(root,cout);
        cout << endl;
    }
    if (writeToFile)
    {
        QDir dir = proj->getProjectDir();
        QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);
        // write it out and read back in the data to ensure the xml is valid
        // and that the xml tree can be read back in and be the same...
        vtkIndent indent(0);
        vtkXMLUtilities::WriteElementToFile(
                    root,file.toStdString().c_str(),&indent);

        root = vtkSmartPointer< vtkXMLDataElement >::Take(
                    vtkXMLUtilities::ReadElementFromFile(
                        file.toStdString().c_str())
                    );
    }

    if (ProjectToXML::xmlToProject(proj2.data(),root)
            == ProjectToXML::XML_TO_DATA_FAILURE)
    {
        retVal++;
        std::cout << "Reading xml for test "
                  << testNum
                  << " failed..." << std::endl;
    }
    else
    {
        CompareBeforeAndAfter::compareProjects(proj,proj2.data(),retVal);

        if (retVal == 0)
        {
            cout << endl << "Passed test " << testNum << endl;
        }

    }
    return retVal;
}

int testSave1()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    MakeTestProject::addObjectToProject(proj1.data(),0);
    MakeTestProject::addObjectToProject(proj1.data(),1);

    return saveLoadAndTest(proj1.data(),1);
}


int testSave2()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    MakeTestProject::addObjectToProject(proj1.data());
    MakeTestProject::addObjectToProject(proj1.data());
    MakeTestProject::addCameraToProject(proj1.data());

    return saveLoadAndTest(proj1.data(),2);
}

int testSave3()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    MakeTestProject::addReplicationToProject(proj1.data(),12);

    return saveLoadAndTest(proj1.data(),3);
}

int testSave4()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    StructureReplicator *rep =
            MakeTestProject::addReplicationToProject(proj1.data(),12);
    MakeTestProject::addSpringToProject(proj1.data(),
                                        rep->getFirstObject(),
                                        rep->getSecondObject());
    for (QListIterator< SketchObject *> itr(rep->getReplicaIterator());
         itr.hasNext(); )
    {
        MakeTestProject::setColorMapForObject(itr.next());
    }

    return saveLoadAndTest(proj1.data(),4);
}

int testSave6()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    SketchObject *o1 = MakeTestProject::addObjectToProject(proj1.data());
    SketchObject *o2 = MakeTestProject::addObjectToProject(proj1.data());
    MakeTestProject::addGroupToProject(proj1.data(),3);
    MakeTestProject::addReplicationToProject(proj1.data(),12);
    MakeTestProject::addSpringToProject(proj1.data(),o1,o2);

    return saveLoadAndTest(proj1.data(),6);
}

int testSave9()
{
    vtkSmartPointer< vtkRenderer > r =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > project(
                new SketchProject(r,LOAD_ONLY_TEST_DIR));

    QDir dir = project->getProjectDir();
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);

    vtkSmartPointer< vtkXMLDataElement > root =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str())
                );

    if (ProjectToXML::xmlToProject(project.data(),root)
            == ProjectToXML::XML_TO_DATA_SUCCESS)
    {
        cout << endl << "Passed test 9" << endl;
    }
    else
    {
        cout << "Reading xml for test 9 failed..." << endl;
        return 1;
    }
    return 0;
}

// tests that my save state makes sense... i.e. no " in the attributes or such
// tests that it can be read back in as valid xml and reconstructed to the project
int testSave5()
{
    vtkSmartPointer< vtkRenderer > r =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > project(
                new SketchProject(r,SAVE_TEST_DIR));

    StructureReplicator *rep =
            MakeTestProject::addReplicationToProject(project.data(),12);
    MakeTestProject::addKeyframesToObject(rep->getSecondObject(),1);
    MakeTestProject::addSpringToProject(project.data(),
                                        rep->getFirstObject(),
                                        rep->getSecondObject());

    return saveLoadAndTest(project.data(),5,true);
}

int testSave7()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    MakeTestProject::addTransformEqualsToProject(proj1.data(),2);
    MakeTestProject::addObjectToProject(proj1.data());

    return saveLoadAndTest(proj1.data(),7);
}

int testSave8()
{
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,SAVE_TEST_DIR));

    MakeTestProject::addObjectToProject(proj1.data());
    MakeTestProject::addObjectToProject(proj1.data());
    SketchObject *o = MakeTestProject::addObjectToProject(proj1.data());
    MakeTestProject::addKeyframesToObject(o,4);

    return saveLoadAndTest(proj1.data(),8);
}

int main(int argc, char *argv[])
{
    int val = 0;
    QDir dir = QDir::current();
    // change the working dir to the dir where the test executable is
    QString executable = dir.absolutePath() + "/" + argv[0];
    int last = executable.lastIndexOf("/");
    if (QDir::setCurrent(executable.left(last)))
        dir = QDir::current();
    std::cout << "Working directory: " <<
                 dir.absolutePath().toStdString().c_str() << std::endl;
    try
    {
        val = testSave1() + testSave2() + testSave3() + testSave4() + testSave5() +
                testSave6() + testSave7() + testSave8() + testSave9();
    }
    catch (const char *c)
    {
        std::cout << c << std::endl;
        val = 1;
    }
    return val;
}
