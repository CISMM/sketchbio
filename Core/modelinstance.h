#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include <vtkSmartPointer.h>
class vtkActor;
class vtkTransformPolyDataFilter;
class vtkPolyDataMapper;

class SketchModel;
#include "sketchobject.h"

/*
 * This class extends SketchObject to provide an object that is a single instance of a
 * single SketchModel's data.
 */

class ModelInstance : public SketchObject {
public:
    // constructor
    explicit ModelInstance(SketchModel *m, int confNum = 0);
    virtual ~ModelInstance();
    // specify that this is a leaf by returning 1
    virtual int numInstances() const;
    // getters for data this subclass holds
    virtual SketchModel *getModel();
    virtual const SketchModel *getModel() const;
    virtual vtkTransformPolyDataFilter *getTransformedGeometry();
    virtual int getModelConformation() const;
    virtual vtkActor *getActor();
    // collision function that depend on data in this subclass
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics,
                         int pqp_flags);
    virtual void getBoundingBox(double bb[]);
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes();
    virtual SketchObject *deepCopy();
protected:
    virtual void localTransformUpdated();
private:
    vtkSmartPointer<vtkActor> actor;
    SketchModel *model;
    int conformation;
    vtkSmartPointer<vtkTransformPolyDataFilter> modelTransformed;
    vtkSmartPointer<vtkTransformPolyDataFilter> orientedBB;
    vtkSmartPointer<vtkPolyDataMapper> solidMapper;
};

#endif
