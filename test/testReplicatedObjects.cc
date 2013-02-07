
#include <sketchmodel.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <structurereplicator.h>
#include <sketchioconstants.h>
#include <sketchproject.h>

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkOBJReader.h>

#include <iostream>

using namespace std;


int main() {
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    bool b[NUM_HYDRA_BUTTONS];
    double a[NUM_HYDRA_ANALOGS];
    SketchProject *project = new SketchProject(renderer,b,a);
    project->setProjectDir("test/testModels/replicatorTest");
//        cout << "Project dir exists" << endl;
//    } else {
//        cout << "Project dir creation failed" << endl;
//    }

    SketchObject *o1 = project->addObject("models/1m1j.obj");
    SketchObject *o2 = project->addObject("models/1m1j.obj");

    project->addReplication(o1,o2,5);

    if (project->getWorldManager()->getNumberOfObjects() != 7) {
        cout << "There are " << project->getWorldManager()->getNumberOfObjects() << " objects!" << endl;
    }

    for (int i = 0; i < 10; i++)
        project->timestep(0.016);

    delete project;
    return 0;
}
