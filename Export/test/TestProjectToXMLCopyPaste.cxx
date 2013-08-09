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
#include <sketchproject.h>

#include <projecttoxml.h>

int testSavePastedItem();

int main(int argc, char *argv[])
{
    return testSavePastedItem();
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
