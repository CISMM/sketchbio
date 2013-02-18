#ifndef SKETCHMODEL_H
#define SKETCHMODEL_H

#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <PQP.h>
#include <QString>
#include <QHash>

/*
 * This class holds data about a general type of object such as a fibrin molecule.  The vtk
 * data that defines the object's shape as well as its collision detection model are here.
 * Multiple objects may share one instance of this class (for now, deformations may change this).
 *
 * Since all objects (molecules) of a given type will have the same mass, mass data is stored here
 * as well.
 */

class SketchModel
{
public:
    SketchModel(QString source, vtkTransformPolyDataFilter *data, double iMass, double iMoment);
    ~SketchModel();
    inline QString getDataSource() const { return dataSource; }
    double getScale() const;
    void getTranslate(double out[3]) const;
    inline vtkTransformPolyDataFilter *getModelData() { return modelData; }
    inline PQP_Model *getCollisionModel() { return collisionModel; }
#ifndef PQP_UPDATE_EPSILON
    inline QHash<int,int> *getTriIdToTriIndexHash() { return &triIdToTriIndex; }
#endif
    inline double getInverseMass() const { return invMass;}
    inline double getInverseMomentOfInertia() const { return invMomentOfInertia; }
private:
    QString dataSource;
    vtkSmartPointer<vtkTransformPolyDataFilter> modelData;
    PQP_Model *collisionModel;
#ifndef PQP_UPDATE_EPSILON
    QHash<int,int> triIdToTriIndex;
#endif
    double invMass;
    double invMomentOfInertia;
};


#endif // SKETCHMODEL_H
