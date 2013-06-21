#include <QTime>
#include <signal.h>

#include <sketchmodel.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <structurereplicator.h>
#include <sketchioconstants.h>
#include <sketchproject.h>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkOBJReader.h>

#include <iostream>

using namespace std;

static bool cont = true;

void handle_segfault(int num) {
    cont = false;
}

int main() {
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> project(new SketchProject(renderer));
    project->setProjectDir("test/testModels/replicatorTest");

    QString filename = "models/1m1j.obj";

    SketchObject *o1 = project->addObject(filename,filename);
    SketchObject *o2 = project->addObject(filename,filename);
    SketchObject *o3 = project->addObject(filename,filename);
    SketchObject *o4 = project->addObject(filename,filename);

    project->addReplication(o1,o2,5);
    QWeakPointer<TransformEquals> equals = project->addTransformEquals(o1,o2);
    QSharedPointer<TransformEquals> eq = equals.toStrongRef();
    project->setCollisionTestsOn(false);
    q_vec_type p1 = {2, 0, -1}, p2 = {3, -2, 4}, tv1;

//    if (project->getWorldManager()->getNumberOfObjects() != 7) {
//        cout << "There are " << project->getWorldManager()->getNumberOfObjects() << " objects!" << endl;
//    }
    QTime time = QTime::currentTime();
    uint seed = (uint) time.msec();
    cout << "Seed: " << seed << endl;
    qsrand(seed);
    struct sigaction action;
    action.sa_handler = &handle_segfault;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGSEGV,&action,NULL);

    int order[4];
    for (int i = 0; i < 4; i++) {
        order[i] = i;
    }
    for (int i = 0; i < 40000 && cont; i++) {
        for (int j = 3; j >= 0; j--) {
            int r = qrand() % (j+1);
            int tmp = order[j];
            order[j] = order[r];
            order[r] = tmp;
        }
        for (int j = 0; j < 3 && cont; j++) {
            SketchObject *obj = NULL;
            switch(order[j]) {
            case 0:
                obj = o1;
                break;
            case 1:
                obj = o2;
                break;
            case 2:
                obj = o3;
                break;
            case 3:
                obj = o4;
                break;
            }
            if (obj != NULL) {
                obj->addForce(p1,p2);
                obj->getPosition(tv1);
            }
        }
        project->timestep(0.016);
    }

    return 0;
}
