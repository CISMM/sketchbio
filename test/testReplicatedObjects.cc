
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
    TransformManager *tmgr = project->getTransformManager();
//    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    ModelManager *mmgr = project->getModelManager();
//    ModelManager *mmgr = new ModelManager();
    WorldManager *wmgr = project->getWorldManager();
//    WorldManager *wmgr = new WorldManager(renderer,transform);
//    vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
//    reader->SetFileName("models/1m1j.obj");
//    reader->Update();
//    int sourceType = mmgr->addObjectSource(reader);
//    SketchModelId modelId = mmgr->addObjectType(sourceType,1.0);
    SketchModel *model = mmgr->modelForOBJSource("models/1m1j.obj");
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    ObjectId o1 = wmgr->addObject(model,pos,orient);
    q_vec_set(pos,0,30,0);
    q_from_axis_angle(orient,0,1,0,Q_PI/22);
    ObjectId o2 = wmgr->addObject(model,pos,orient);
//    StructureReplicator *rep = new StructureReplicator(o1,o2,wmgr,transform);
    StructureReplicator *rep = new StructureReplicator(o1,o2,wmgr,tmgr->getWorldToEyeTransform());
    rep->setNumShown(5);

    if (wmgr->getNumberOfObjects() != 7) {
        cout << "There are " << wmgr->getNumberOfObjects() << " objects!" << endl;
    }

//    for (int i = 0; i < 10; i++)
//        wmgr->stepPhysics(0.016);
    for (int i = 0; i < 10; i++)
        project->timestep(0.016);

    delete rep;
//    delete wmgr;
//    delete mmgr;
    delete project;
    return 0;
}
