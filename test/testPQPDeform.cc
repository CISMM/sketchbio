/*
 *
 * This test makes sure that PQP is detecting the deformation collisions correctly
 * Author: Shawn Waldon
 *
 */

#include <iostream>
#include <algorithm>
#include <vector>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkSphereSource.h>
#include <vtkTetra.h>
#include "../modelmanager.h"
#include <PQP.h>

using namespace std;

struct less_than_key {
    inline bool operator() (const CollisionPair& struct1, const CollisionPair& struct2)
    {
        return (struct1.id1 < struct2.id1) || (struct1.id1 == struct2.id1 && struct1.id2 < struct2.id2);
    }
};

void makeTetrahedron(vtkSmartPointer<vtkPolyData> *ptr) {

    vtkSmartPointer< vtkPoints > points =
      vtkSmartPointer< vtkPoints > :: New();
    points->InsertNextPoint(0, 0, 0);
    points->InsertNextPoint(1, 0, 0);
    points->InsertNextPoint(1, 1, 0);
    points->InsertNextPoint(0, 1, 1);

    // Method 1
    vtkSmartPointer<vtkPolyData> unstructuredGrid1 =
      vtkSmartPointer<vtkPolyData>::New();
    unstructuredGrid1->SetPoints(points);
    unstructuredGrid1->Allocate();

    vtkSmartPointer<vtkTetra> tetra =
      vtkSmartPointer<vtkTetra>::New();

    tetra->GetPointIds()->SetId(0, 0);
    tetra->GetPointIds()->SetId(1, 1);
    tetra->GetPointIds()->SetId(2, 2);
    tetra->GetPointIds()->SetId(3, 3);
    for (int i = 0; i < tetra->GetNumberOfFaces(); i++) {
        vtkSmartPointer<vtkCell> cell = tetra->GetFace(i);
        unstructuredGrid1->InsertNextCell(cell->GetCellType(),cell->GetPointIds());
    }
    *ptr = unstructuredGrid1.GetPointer();
}

void printResults(PQP_CollideResult &cr,PQP_Model &m) {
    int num;
    vector<CollisionPair> vec = vector<CollisionPair>();
    cout << (num = cr.NumPairs()) << endl;
    for (int i = 0; i < num; i++) {
        vec.push_back(cr.pairs[i]);
    }
    sort(vec.begin(),vec.end(),less_than_key());
    for (int i = 0; i < num; i++) {
        cout << vec[i].id1 << " " << vec[i].id2 << endl;
    }
}

int main() {
    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->Update();
    vtkSmartPointer<vtkPolyData> tetra;
//    makeTetrahedron(&tetra);
    tetra = sphere->GetOutput();
    PQP_Model model, model2;
    makePQP_Model(model,*tetra);
#ifdef PQP_UPDATE_EPSILON
    makePQP_Model(model2,*tetra);
#endif
    PQP_REAL IDENT[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    PQP_REAL t1[3] = {0,0,0}, t2[3] = {.5,.5,-.3};
    PQP_CollideResult cr;
    PQP_Collide(&cr,IDENT,t1,&model,IDENT,t2,&model,PQP_ALL_CONTACTS);
    printResults(cr,model);

    vtkSmartPointer<vtkPoints> pts = tetra->GetPoints();
    for (int i = 0; i < pts->GetNumberOfPoints(); i++) {
        double p[3];
        pts->GetPoint(i,p);
        p[0] = 2*p[0];
        p[1] = p[1]/2;
        p[2] = 3*p[2];
        pts->SetPoint(i,p[0],p[1],p[2]);
    }
    pts->Modified();
    tetra->SetPoints(pts);
    tetra->Update();


    PQP_CollideResult cr2;
    // update model

#ifndef PQP_UPDATE_EPSILON
    makePQP_Model(model2,*tetra);
    cout << "Using original PQP rebuild" << endl;
#else
    updatePQP_Model(model2,*tetra);
    cout << "Using new PQP update" << endl;
#endif
    PQP_Collide(&cr2,IDENT,t1,&model,IDENT,t2,&model2,PQP_ALL_CONTACTS);
    printResults(cr2,model2);
    return 0;
}
