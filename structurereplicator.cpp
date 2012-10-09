#include "structurereplicator.h"

StructureReplicator::StructureReplicator(vtkActor *ref1, vtkActor *ref2, vtkRenderer *rend, vtkMapper *map, TransformManager *transforms) {
    actor1 = ref1;
    actor2 = ref2;
    renderer = rend;
    mapper = map;
    numShown = 0;
    master = vtkSmartPointer<vtkTransform>::New();
    master->Identity();
    transformMgr = transforms;
    for (int i = 0; i < STRUCTURE_REPLICATOR_MAX_COPIES; i++) {
        copies[i] = NULL;
    }
}


void StructureReplicator::setNumShown(int num) {
    if (num > STRUCTURE_REPLICATOR_MAX_COPIES) {
        num = STRUCTURE_REPLICATOR_MAX_COPIES;
    } else if (num < 0) {
        num = 0;
    }
    if (num > numShown) {
        for (; numShown < num; numShown++) {
            copies[numShown] = vtkSmartPointer<vtkActor>::New();
            copies[numShown]->SetMapper(mapper);
            vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
            trans->Identity();
            trans->PostMultiply();
            if (numShown == 0) {
                // special code for first one
                trans->Concatenate(vtkTransform::SafeDownCast(actor2->GetUserTransform()));
            } else {
                trans->Concatenate(vtkTransform::SafeDownCast(copies[numShown-1]->GetUserTransform()));
            }
            trans->Concatenate(master);
            copies[numShown]->SetUserTransform(trans);
            renderer->AddActor(copies[numShown]);
        }
    } else {
        while (numShown > num) {
            renderer->RemoveActor(copies[numShown-1]);
            copies[numShown-1] = NULL;
            numShown--;
        }
    }
}


void StructureReplicator::updateBaseline() {
    master->Identity();
    master->PostMultiply();
    vtkSmartPointer<vtkTransform> eyeWorld = vtkSmartPointer<vtkTransform>::New();
    transformMgr->getWorldToEyeTransform(eyeWorld);
    eyeWorld->Inverse();
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkTransform> trans1 = vtkTransform::SafeDownCast(actor1->GetUserTransform());
    vtkSmartPointer<vtkTransform> trans2 = vtkTransform::SafeDownCast(actor2->GetUserTransform());
    vtkSmartPointer<vtkTransform> trans1Inv = vtkSmartPointer<vtkTransform>::New();
    trans1Inv->Identity();
    trans1Inv->PostMultiply();
    trans1Inv->Concatenate(trans1);
    trans1Inv->Concatenate(eyeWorld);
    trans1Inv->Inverse();
    master->Concatenate(eyeWorld);
    master->Concatenate(trans1Inv);
    master->Concatenate(trans2);
    master->Concatenate(mat);
}
