
/*
 *
 * This class was originally written for a homework assignment in Physically Based Modeling (COMP768)
 * Author: Shawn Waldon
 *
 */

#include "modelmanager.h"

#include <vtkTransform.h>
#include <vtkCellArray.h>

#include <QDebug>

#define INVERSEMASS 1.0
#define INVERSEMOMENT (1.0/25000)

ModelManager::ModelManager()
{
    sources = vtkSmartPointer<vtkCollection>::New();
    models = std::vector<SketchModel *>();
}

ModelManager::~ModelManager() {
    SketchModel * model;
    while (!models.empty()) {
        model = models.back();
        models.pop_back();
        delete model;
    }
}


/*****************************************************************************
  *
  * Adds a new Algorithm to use as a source in vtk.
  * Returns an index to this source to use with addObjectType.
  *
  * Note: addObjectType initializes the model with scaling and must be called
  * before it is available from the getPolyDataOutput and getOutputPort methods
  *
  * dataSource - the new source to use
  *
  ****************************************************************************/
int ModelManager::addObjectSource(vtkPolyDataAlgorithm *dataSource) {
    dataSource->Update();
    sources->AddItem(vtkObject::SafeDownCast(dataSource));
    return sources->GetNumberOfItems() -1;
}

/*****************************************************************************
  *
  * Adds an instance of the identified model at scale factor scale.
  * Returns the index of the scaled model in the usable models list
  *
  * srcIndex - the model ID, returned by addObjectSource
  * scale    - the scale factor to scale the object by
  *
  ****************************************************************************/
SketchModelId ModelManager::addObjectType(int srcIndex, double scale) {
    vtkSmartPointer<vtkTransformPolyDataFilter> transformPD =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    // set the correct input connection
    vtkSmartPointer<vtkPolyDataAlgorithm> polyDataSrc =
            (vtkPolyDataAlgorithm *)sources->GetItemAsObject(srcIndex);


    transformPD->SetInputConnection(polyDataSrc->GetOutputPort());


    // get the bounding box of the polydata so we can recenter it
    vtkSmartPointer<vtkPolyData> polyData = polyDataSrc->GetOutput();
    double bb[6];
    polyData->GetBounds(bb);

    // set the scale of the transformation
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    // make sure the object is centered
    transform->Translate(
                -(bb[0]+bb[1])/2,
                -(bb[2]+bb[3])/2,
                -(bb[4]+bb[5])/2);
    // apply the requested scale factor
    transform->Scale(scale,scale,scale);
    transformPD->SetTransform(transform);
    transformPD->Update();


    size_t idNum = models.size();
    // fix potential out of space problems as they come up
    if (idNum >= models.capacity()) {
        models.reserve(2*idNum);
    }

    // remember the mass and moment of inertia are inverses!!!!!!!!
    SketchModel * sModel = new SketchModel(transformPD,INVERSEMASS,INVERSEMOMENT);
    // TODO these shouldn't be magic constants

    makePQP_Model(*(sModel->getCollisionModel()),*(transformPD->GetOutput()));

    models.push_back(sModel);
    SketchModelId id = models.end();
    id--;
    return id;
}

/*****************************************************************************
  *
  * This function initializes the PQP_Model from the vtkPolyData object passed
  * to it.
  *
  * m1 a reference parameter to an empty PQP_Model to initialize
  * polyData a reference parameter to a vtkPolyData to pull the model data from
  *
  ****************************************************************************/
void makePQP_Model(PQP_Model &m1, vtkPolyData &polyData) {

    int numPoints = polyData.GetNumberOfPoints();

    // get points
    double *points = new double[3*numPoints];
    for (int i = 0; i < numPoints; i++) {
        polyData.GetPoint(i,&points[3*i]);
    }

    int numStrips = polyData.GetNumberOfStrips();
    int numPolygons = polyData.GetNumberOfPolys();
    if (numStrips > 0) {  // if we have triangle strips, great, use them!
        vtkCellArray *strips = polyData.GetStrips();

        m1.BeginModel();
        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numStrips; i++) {
            strips->GetCell( loc, nvertices, pvertices );
            // todo, refactor to getNextCell for better performance

            p1[0] = points[3*pvertices[0]];
            p1[1] = points[3*pvertices[0]+1];
            p1[2] = points[3*pvertices[0]+2];
            p2[0] = points[3*pvertices[1]];
            p2[1] = points[3*pvertices[1]+1];
            p2[2] = points[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
                p3[0] = points[3*pvertices[j]];
                p3[1] = points[3*pvertices[j]+1];
                p3[2] = points[3*pvertices[j]+2];
                // ensure counterclockwise points
                if (j % 2 == 0)
                    m1.AddTri(p1,p2,p3,triId++);
                else
                    m1.AddTri(p1,p3,p2,triId++);
                PQP_REAL *tmp = p1;
                p1 = p2;
                p2 = p3;
                p3 = tmp;
            }
            loc += nvertices+1;
        }
        m1.EndModel();
    } else if (numPolygons > 0) { // we may not have triangle strip data, try polygons
        vtkCellArray *polys = polyData.GetPolys();

        m1.BeginModel();
        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numPolygons; i++) {
            polys->GetCell(loc,nvertices,pvertices);
            p1[0] = points[3*pvertices[0]];
            p1[1] = points[3*pvertices[0]+1];
            p1[2] = points[3*pvertices[0]+2];
            p2[0] = points[3*pvertices[1]];
            p2[1] = points[3*pvertices[1]+1];
            p2[2] = points[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
                p3[0] = points[3*pvertices[j]];
                p3[1] = points[3*pvertices[j]+1];
                p3[2] = points[3*pvertices[j]+2];
                m1.AddTri(p1,p2,p3,triId++);
                PQP_REAL *tmp = p2;
                p2= p3;
                p3 = tmp;
            }
            loc += nvertices +1;
        }
        m1.EndModel();
    }

    delete[] points;
}


#ifdef PQP_UPDATE_EPSILON
/****************************************************************************
  ***************************************************************************/
void updatePQP_Model(PQP_Model &model,vtkPolyData &polyData) {

    int numPoints = polyData.GetNumberOfPoints();

    // get points
    double *points = new double[3*numPoints];
    for (int i = 0; i < numPoints; i++) {
        polyData.GetPoint(i,&points[3*i]);
    }

    int numStrips = polyData.GetNumberOfStrips();
    int numPolygons = polyData.GetNumberOfPolys();
    if (numStrips > 0) {  // if we have triangle strips, great, use them!
        vtkCellArray *strips = polyData.GetStrips();

        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numStrips; i++) {
            strips->GetCell( loc, nvertices, pvertices );
            // todo, refactor to getNextCell for better performance

            p1[0] = points[3*pvertices[0]];
            p1[1] = points[3*pvertices[0]+1];
            p1[2] = points[3*pvertices[0]+2];
            p2[0] = points[3*pvertices[1]];
            p2[1] = points[3*pvertices[1]+1];
            p2[2] = points[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
                p3[0] = points[3*pvertices[j]];
                p3[1] = points[3*pvertices[j]+1];
                p3[2] = points[3*pvertices[j]+2];
                // ensure counterclockwise points
                if (j % 2 == 0)
                    model.UpdateTri(p1,p2,p3,triId++);
                else
                    model.UpdateTri(p1,p3,p2,triId++);
                PQP_REAL *tmp = p1;
                p1 = p2;
                p2 = p3;
                p3 = tmp;
            }
            loc += nvertices+1;
        }
    } else if (numPolygons > 0) { // we may not have triangle strip data, try polygons
        vtkCellArray *polys = polyData.GetPolys();

        vtkIdType nvertices, *pvertices, loc = 0;
        PQP_REAL t[3], u[3],v[3], *p1 = t, *p2 = u, *p3 = v;
        int triId = 0;
        for (int i = 0; i < numPolygons; i++) {
            polys->GetCell(loc,nvertices,pvertices);
            p1[0] = points[3*pvertices[0]];
            p1[1] = points[3*pvertices[0]+1];
            p1[2] = points[3*pvertices[0]+2];
            p2[0] = points[3*pvertices[1]];
            p2[1] = points[3*pvertices[1]+1];
            p2[2] = points[3*pvertices[1]+2];
            for (int j = 2; j < nvertices; j++) {
                p3[0] = points[3*pvertices[j]];
                p3[1] = points[3*pvertices[j]+1];
                p3[2] = points[3*pvertices[j]+2];
                model.UpdateTri(p1,p2,p3,triId++);
                PQP_REAL *tmp = p2;
                p2= p3;
                p3 = tmp;
            }
            loc += nvertices +1;
        }
    }
    model.UpdateModel();

    delete[] points;
}
#endif
