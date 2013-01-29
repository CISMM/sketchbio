
/*
 *
 * This class was originally written for a homework assignment in Physically Based Modeling (COMP768)
 * Author: Shawn Waldon
 *
 */

#include "modelmanager.h"

#include <vtkTransform.h>
#include <vtkCellArray.h>
#include <vtkOBJReader.h>

#include <QDebug>

#define INVERSEMASS 1.0
#define INVERSEMOMENT (1.0/25000)

ModelManager::ModelManager() :
    models()
{
}

ModelManager::~ModelManager() {
    SketchModel * model;
    QMutableHashIterator<QString, SketchModel *> i(models);
     while (i.hasNext()) {
         i.next();
         model = i.value();
         i.setValue((SketchModel *) NULL);
         delete model;
     }
}

/*****************************************************************************
  *
  * This method creates a SketchModel from the given vtk source using the given
  * scale factor.  If there is already a model using a vtk source of the same class name, this
  * method simply returns the old one (ignores scale for now).
  *
  * source  - the VTK source that should be used to generate the geometry for
  *             the model
  * scale   - the scale at which the source should be interpreted
  *
  ****************************************************************************/
SketchModel *ModelManager::modelForVTKSource(vtkPolyDataAlgorithm *source, double scale) {

    const char *src = source->GetClassName();
    QString srcString(src);
    srcString = "VTK: " + srcString;

    if (models.contains(srcString)) {
        return models[srcString];
    }

    vtkSmartPointer<vtkTransformPolyDataFilter> transformPD =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    source->Update();
    transformPD->SetInputConnection(source->GetOutputPort());

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->Scale(scale,scale,scale);

    transformPD->SetTransform(transform);
    transformPD->Update();

    SketchModel *sModel = new SketchModel(srcString,transformPD,INVERSEMASS,INVERSEMOMENT);

    makePQP_Model(*(sModel->getCollisionModel()),*(transformPD->GetOutput()));

    models.insert(srcString,sModel);

    return sModel;
}

/*****************************************************************************
  *
  * This method creates a SketchModel from the given obj file using the given
  * scale factor.  If there is already a model using the given OBJ file, this
  * method simply returns the old one (ignores scale for now).
  *
  * objFile - the filename of the obj file (should be absolute path, but in
  *             the project folder
  * scale   - the scale at which the obj should be interpreted
  *
  ****************************************************************************/
SketchModel *ModelManager::modelForOBJSource(QString objFile, double scale) {
    if (models.contains(objFile)) {
        return models[objFile];
    }

    vtkSmartPointer<vtkTransformPolyDataFilter> transformPD =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    // set the correct input connection
    vtkSmartPointer<vtkOBJReader> polyDataSrc = vtkSmartPointer<vtkOBJReader>::New();
    polyDataSrc->SetFileName(objFile.toStdString().c_str());
    polyDataSrc->Update();

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

    // remember the mass and moment of inertia are inverses!!!!!!!!
    SketchModel * sModel = new SketchModel("",transformPD,INVERSEMASS,INVERSEMOMENT);
    // TODO these shouldn't be magic constants

    makePQP_Model(*(sModel->getCollisionModel()),*(transformPD->GetOutput()));

    models.insert(objFile,sModel);

    return sModel;
}

/*****************************************************************************
  *
  * This method returns an iterator that may be used to examine each of the
  * models in the ModelManager.
  *
  ****************************************************************************/
QHashIterator<QString,SketchModel *> ModelManager::getModelIterator() const {
    return QHashIterator<QString,SketchModel *>(models);
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
