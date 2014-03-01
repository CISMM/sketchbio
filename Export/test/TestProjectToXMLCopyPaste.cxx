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
#include "MakeTestProject.h"

#define TEST_DIR "test/test1"

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
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,TEST_DIR));

    SketchObject *obj = MakeTestProject::addObjectToProject(proj1.data());

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
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,TEST_DIR));

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

    CompareBeforeAndAfter::compareObjects(obj.data(),pasted,retVal,true,true);

    return retVal;
}

int testPastedGroupIsTheSame()
{
    int retVal = 0;
    vtkSmartPointer< vtkRenderer > r1 =
            vtkSmartPointer< vtkRenderer >::New();
    QScopedPointer< SketchProject > proj1(
                new SketchProject(r1,TEST_DIR));

    SketchModel *m1 = TestCoreHelpers::getCubeModel();
    SketchModel *m2 = TestCoreHelpers::getSphereModel();
    proj1->addModel(m1);
    proj1->addModel(m2);

    // This ensures that a wide variety of things will be tested including
    // copy/paste of color maps, keyframes, ...
    SketchObject *obj1 = MakeTestProject::addObjectToProject(proj1.data());
    MakeTestProject::addKeyframesToObject(obj1,4);
    SketchObject *obj2 = MakeTestProject::addCameraToProject(proj1.data());
    proj1->getWorldManager()->removeObject(obj1);
    proj1->getWorldManager()->removeObject(obj2);
    SketchObject *obj3 = obj1->getCopy();
    SketchObject *obj4 = obj2->getCopy();

    q_vec_type vec = {5, 0, 0};
    obj1->setPosition(vec);
    vec[0] = -5;
    obj2->setPosition(vec);

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

    CompareBeforeAndAfter::compareObjects(obj.data(),pasted,retVal,true,true);

    return retVal;
}
