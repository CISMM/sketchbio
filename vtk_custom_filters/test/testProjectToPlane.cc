/*
 *
 * This is a test of vtkProjectToPlane, my custom filter
 *
 */

#include <vtkSmartPointer.h>
#include <vtkProjectToPlane.h>
#include <vtkPoints.h>
#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkVersion.h>

#include <iostream>

using std::cout;
using std::endl;

inline void makeInput(vtkProjectToPlane *filter)
{
    vtkSmartPointer< vtkPoints > pointsIn =
            vtkSmartPointer< vtkPoints >::New();
    pointsIn->InsertNextPoint(3,5,7);
    pointsIn->InsertNextPoint(0,0,0);
    pointsIn->InsertNextPoint(-3,5.2,8.934);
    pointsIn->InsertNextPoint(6.2,-3,-4.5);
    pointsIn->InsertNextPoint(-1,-3,-7.3);

    vtkSmartPointer< vtkPolyData > polyDataIn =
            vtkSmartPointer< vtkPolyData >::New();
    polyDataIn->Allocate();
    polyDataIn->SetPoints(pointsIn);
    vtkSmartPointer< vtkIdList > ids =
            vtkSmartPointer< vtkIdList >::New();
    ids->InsertNextId(1);
    ids->InsertNextId(3);
    ids->InsertNextId(0);
    polyDataIn->InsertNextCell(VTK_TRIANGLE,ids);

#if VTK_MAJOR_VERSION <= 5
    filter->SetInput(polyDataIn);
#else
    filter->SetInputData(polyDataIn);
#endif
}

inline int testIfOnPlane(double *point, double *normal, vtkPolyData *result)
{
    vtkSmartPointer< vtkPoints > pointsOut = result->GetPoints();
    int numResultPoints = pointsOut->GetNumberOfPoints();

    double length = sqrt( normal[0] * normal[0] +
            normal[1] * normal[1] + normal[2] + normal[2]);
    double myNormal[3] = { normal[0] / length, normal[1] / length, normal[2] / length };

    for (vtkIdType i = 0; i < numResultPoints; i++)
    {
        double pt[3];
        pointsOut->GetPoint(i,pt);

        double testPlaneEquationResult =
                myNormal[0] * ( pt[0] - point[0] ) +
                myNormal[1] * ( pt[1] - point[1] ) +
                myNormal[2] * ( pt[2] - point[2] );

        if (abs(testPlaneEquationResult) > 1e-12)
        {
            cout << "Found point not on plane" << endl;
            return 1;
        }
    }

    if (result->GetNumberOfCells() != 1)
    {
        cout << "Filter removed cell." << endl;
        return 1;
    }
    if (result->GetCellType(0) != VTK_TRIANGLE)
    {
        cout << "Cell changed type." << endl;
        return 1;
    }
    return 0;
}

int test1()
{
    vtkSmartPointer< vtkProjectToPlane > filter =
            vtkSmartPointer< vtkProjectToPlane >::New();
    makeInput(filter);
    double point[3] = { 0.0, 0.0, 0.0 };
    double vector[3] = { 0.0, 0.0, 1.0 };
    filter->SetPointOnPlane(point);
    filter->SetPlaneNormalVector(vector);
    filter->Update();

    return testIfOnPlane(point,vector,filter->GetOutput());
}

int test2()
{
    vtkSmartPointer< vtkProjectToPlane > filter =
            vtkSmartPointer< vtkProjectToPlane >::New();
    makeInput(filter);
    double point[3] = { 2.0, 3.0, 5.0 };
    double vector[3] = { 0.4, 0.6, 1.6 };
    filter->SetPointOnPlane(point);
    filter->SetPlaneNormalVector(vector);
    filter->Update();

    return testIfOnPlane(point,vector,filter->GetOutput());
}

int main() {
    return test1() + test2();
}
