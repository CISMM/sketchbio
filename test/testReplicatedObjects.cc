
#include "../sketchmodel.h"
#include "../modelmanager.h"
#include "../sketchobject.h"
#include "../springconnection.h"
#include "../worldmanager.h"
#include "../structurereplicator.h"

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkOBJReader.h>

#include <iostream>

using namespace std;


int main() {
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    ModelManager *mmgr = new ModelManager();
    WorldManager *wmgr = new WorldManager(renderer,transform);
    vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName("models/1m1j.obj");
    reader->Update();
    int sourceType = mmgr->addObjectSource(reader);
    SketchModelId modelId = mmgr->addObjectType(sourceType,1.0);
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    ObjectId o1 = wmgr->addObject(modelId,pos,orient);
    q_vec_set(pos,0,30,0);
    q_from_axis_angle(orient,0,1,0,Q_PI/22);
    ObjectId o2 = wmgr->addObject(modelId,pos,orient);
    StructureReplicator *rep = new StructureReplicator(o1,o2,wmgr,transform);
    rep->setNumShown(5);

    if (wmgr->getNumberOfObjects() != 7) {
        cout << "There are " << wmgr->getNumberOfObjects() << " objects!" << endl;
    }

    for (int i = 0; i < 10; i++)
        wmgr->stepPhysics(0.016);

    delete rep;
    delete wmgr;
    delete mmgr;
    return 0;
}
