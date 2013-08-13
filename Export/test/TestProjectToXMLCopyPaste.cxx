/*
 * This file tests bugs in copy and paste (and interactions with other things).
 *
 * Even though the copy/paste functionality lives in the same file as Save, it was
 * decided that the save tests should be kept separate to differentiate which is failing.
 */

#include <iostream>

#include <quat.h>

#include <QScopedPointer>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkXMLDataElement.h>

#include <sketchmodel.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <worldmanager.h>
#include <sketchproject.h>

#include <projecttoxml.h>

#include "test/TestCoreHelpers.h"
#include "CompareBeforeAndAfter.h"

int testSavePastedItem();
int testPastedItemIsTheSame();
int testPastedGroupIsTheSame();

int main(int argc, char *argv[])
{
    return    testSavePastedItem()
            + testPastedItemIsTheSame()
            + testPastedGroupIsTheSame();
}

int testSavePastedItem()
{
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    proj1->setProjectDir("test/test1");
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj1->addModelFromFile(filename,filename,
                                              3*sqrt(12.0),4*sqrt(13.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *obj = proj1->addObject(m1,pos1,orient1);

    vtkSmartPointer< vtkXMLDataElement > copy =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                ProjectToXML::objectToClipboardXML(obj)
                );
    q_vec_type newPos = { 5.3, 6.7, 2.3 };
    ProjectToXML::objectFromClipboardXML(proj1.data(),copy,newPos);

    vtkSmartPointer< vtkXMLDataElement > save =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                ProjectToXML::projectToXML(proj1.data())
                );
    if (save.GetPointer() == NULL)
    {
        std::cout << "Saving pasted item failed." << std::endl;
        retVal++;
    }
    return retVal;
}

int testPastedItemIsTheSame()
{
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    proj1->setProjectDir("test/test1");

    SketchModel *m1 = TestCoreHelpers::getCubeModel();
    proj1->addModel(m1);

    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    QScopedPointer< SketchObject > obj( new ModelInstance(m1));
    obj->setPosAndOrient(pos1,orient1);

    vtkSmartPointer< vtkXMLDataElement > copy =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                ProjectToXML::objectToClipboardXML(obj.data())
                );
    ProjectToXML::objectFromClipboardXML(proj1.data(),copy,pos1);

    const QList< SketchObject * > *list = proj1->getWorldManager()->getObjects();
    if (list->size() != 1)
    {
        cout << "Wrong number of objects in result." << endl;
    }
    SketchObject *pasted = list->at(0);

    CompareBeforeAndAfter::compareObjects(obj.data(),pasted,retVal,true);

    return retVal;
}

int testPastedGroupIsTheSame()
{
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1));
    proj1->setProjectDir("test/test1");

    SketchModel *m1 = TestCoreHelpers::getCubeModel();
    SketchModel *m2 = TestCoreHelpers::getSphereModel();
    proj1->addModel(m1);
    proj1->addModel(m2);

    q_vec_type vec = {5, 0, 0};
    SketchObject *obj1 = new ModelInstance(m1);
    obj1->setPosition(vec);
    SketchObject *obj2 = new ModelInstance(m2);
    SketchObject *obj3 = obj1->deepCopy();
    SketchObject *obj4 = obj2->deepCopy();
    vec[0] = -5;
    obj2->setPosition(vec);
    obj2->setColorMapType(SketchObject::ColorMapType::DIM_SOLID_COLOR_GREEN);
    obj2->setArrayToColorBy("xfcde");

    ObjectGroup *grp1 = new ObjectGroup();
    grp1->addObject(obj1);
    grp1->addObject(obj2);
    ObjectGroup *grp2 = new ObjectGroup();
    grp2->addObject(obj3);
    grp2->addObject(obj4);
    QScopedPointer< ObjectGroup > obj( new ObjectGroup());
    obj->addObject(grp1);
    obj->addObject(grp2);
    q_vec_type pos1;
    q_vec_set(pos1,0,10,0);
    obj->setPosition(pos1);

    vtkSmartPointer< vtkXMLDataElement > copy =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                ProjectToXML::objectToClipboardXML(obj.data())
                );
    ProjectToXML::objectFromClipboardXML(proj1.data(),copy,pos1);

    const QList< SketchObject * > *list = proj1->getWorldManager()->getObjects();
    if (list->size() != 1)
    {
        cout << "Wrong number of objects in result." << endl;
    }
    SketchObject *pasted = list->at(0);
    pasted->setPosition(pos1); // position is not preserved by copy/paste

    CompareBeforeAndAfter::compareObjects(obj.data(),pasted,retVal,true);

    return retVal;
}
