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

using std::cout;
using std::endl;

int testSave1() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    m1->addConformation(m1->getSource(0),
                        m1->getFileNameFor(0,ModelResolution::FULL_RESOLUTION));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    proj1->addObject(m1,pos1,orient1);
    proj1->addObject(new ModelInstance(m1,1));

    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    if (ProjectToXML::xmlToProject(proj2.data(),root) == ProjectToXML::XML_TO_DATA_FAILURE)
    {
        retVal++;
        std::cout << "Failed to load project." << std::endl;
        root->Delete();
    }
    else
    {

        // test objects and models
        CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
        CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);

        if (retVal == 0) {
            cout << "\nPassed test 1\n" << endl;
        }
        root->Delete();

    }
    return retVal;
}


int testSave2() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    o1->setColorMapType(SketchObject::ColorMapType::BLUE_TO_RED);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    o2->setColorMapType(SketchObject::ColorMapType::DIM_SOLID_COLOR_BLUE);
    o2->setArrayToColorBy("myarray");
    proj1->addCamera(pos1,orient1);

    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareCameras(proj1.data(), proj2.data(), retVal);

    if (retVal == 0) {
        cout << "\nPassed test 2\n" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave3() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    proj1->addReplication(o1,o2,12);


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);

    if (retVal == 0) {
        cout << "\nPassed test 3\n" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave4() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    proj1->addReplication(o1,o2,12);
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0),p1);
    q_vec_set(p2,2.23,6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0),p2);
    proj1->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),3.56*sqrt(11.0),p1,p2); // sqrt(2)


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);
    CompareBeforeAndAfter::compareSprings(proj1->getWorldManager()->getSpringsIterator().next(),
                   proj2->getWorldManager()->getSpringsIterator().next(),retVal,true);

    if (retVal == 0) {
        cout << "\nPassed test 4\n" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave6() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    SketchObject *o3 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    SketchObject *o4 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o4->setPosAndOrient(pos1,orient1);
    SketchObject *o5 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o5->setPosAndOrient(pos1,orient1);
    o5->setColorMapType(SketchObject::ColorMapType::DIM_SOLID_COLOR_BLUE);
    ObjectGroup *grp = new ObjectGroup();
    grp->addObject(o3);
    grp->addObject(o4);
    grp->addObject(o5);
    pos1[Q_Z] = -40;
    pos1[Q_Y] = 300 / Q_PI;
    q_make(orient1,0,1,0,Q_PI/3);
    grp->setPosAndOrient(pos1,orient1);
    proj1->addObject(grp);
    proj1->addReplication(o1,o2,12);
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0),p1);
    q_vec_set(p2,2.23,6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0),p2);
    proj1->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),3.56*sqrt(11.0),p1,p2); // sqrt(2)


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);
    CompareBeforeAndAfter::compareSprings(proj1->getWorldManager()->getSpringsIterator().next(),
                   proj2->getWorldManager()->getSpringsIterator().next(),retVal,true);

    if (retVal == 0) {
        cout << "\nPassed test 6\n" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave9() {
    vtkSmartPointer<vtkRenderer> r = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> project(new SketchProject(r));
    project->setProjectDir("test/test2");

    QDir dir = project->getProjectDir();
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);

    vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());

    if (ProjectToXML::xmlToProject(project.data(),root) == ProjectToXML::XML_TO_DATA_SUCCESS) {
        cout << "\nPassed test 9\n" << endl;
    } else {
        cout << "Failed to load" << endl;
        root->Delete();
        return 1;
    }
    root->Delete();
    return 0;
}


int testSave5() {
    vtkSmartPointer<vtkRenderer> r = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> project(new SketchProject(r)), project2(new SketchProject(r));
    project->setProjectDir("test/test1");
    project2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = project->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = project->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = project->addObject(m1,pos1,orient1);
    o2->addKeyframeForCurrentLocation(0.0);
    project->addReplication(o1,o2,12);
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0),p1);
    q_vec_set(p2,2.23,6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0),p2);
    project->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),3.56*sqrt(11.0),p1,p2); // sqrt(2)

    QDir dir = project->getProjectDir();
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);

    vtkXMLDataElement *b4 = ProjectToXML::projectToXML(project.data());
    vtkIndent indent(0);
    vtkXMLUtilities::WriteElementToFile(b4,file.toStdString().c_str(),&indent);

    b4->Delete();

    vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());

    if (ProjectToXML::xmlToProject(project2.data(),root) == ProjectToXML::XML_TO_DATA_SUCCESS) {
        cout << "\nPassed test 5\n" << endl;
    } else {
        cout << "Failed to load" << endl;
        root->Delete();
        return 1;
    }
    root->Delete();
    return 0;
}

int testSave7() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    SketchObject *o3 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    proj1->addObject(o3);
    SketchObject *o4 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o4->setPosAndOrient(pos1,orient1);
    proj1->addObject(o4);
    SketchObject *o5 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o5->setPosAndOrient(pos1,orient1);
    proj1->addObject(o5);

    QWeakPointer<TransformEquals> eq = proj1->addTransformEquals(o1,o2);
    QSharedPointer<TransformEquals> sEq(eq);
    if (sEq) {
        sEq->addPair(o3,o4);
    }


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareTransformOps(proj1->getTransformOps()->first(),
                        proj2->getTransformOps()->first(),retVal,true);

    if (retVal == 0) {
        cout << "\nPassed test 7\n" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave8() {
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    proj1->addObject(m1,pos1,orient1);
    SketchObject *o3 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    proj1->addObject(o3);
    o3->addKeyframeForCurrentLocation(0.4);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    o3->addKeyframeForCurrentLocation(4.332);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    o3->addKeyframeForCurrentLocation(10.5);
    pos1[Q_Z] = 3.14;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    o3->setPosAndOrient(pos1,orient1);

    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    CompareBeforeAndAfter::compareNumbers(proj1.data(),proj2.data(),retVal);
    CompareBeforeAndAfter::compareWorldObjects(proj1.data(),proj2.data(),retVal);

    if (retVal == 0) {
        cout << "\nPassed test 8\n" << endl;
    }

    root->Delete();
    return retVal;
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