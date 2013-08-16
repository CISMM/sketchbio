#ifndef OBJECTGROUP_H
#define OBJECTGROUP_H

#include <vtkSmartPointer.h>
class vtkAppendPolyData;

#include <QList>

#include "sketchobject.h"

/*
 * This class is a composite of SketchObjects that can be
 * treated as a single object
 */

class ObjectGroup : public SketchObject {
public:
    // constructor
    ObjectGroup();
    // destructor
    virtual ~ObjectGroup();
    // specify this is not a leaf (returns -1 * number of objects)
    virtual int numInstances() const;
    virtual ColorMapType::Type getColorMapType() const;
    virtual void setColorMapType(ColorMapType::Type cmap);
    virtual QString getArrayToColorBy() const;
    virtual void setArrayToColorBy(const QString &arrayName);
    virtual vtkTransformPolyDataFilter *getTransformedGeometry();
    // methods to add/remove objects
    // note: this method gives ObjectGroup ownership until the object is removed
    // so these objects are cleaned up in ObjectGroup's destructor
    void addObject(SketchObject *obj);
    // when the object is removed it is no longer owned by ObjectGroup, the remover is responsible
    // for ensuring it is deallocated
    void removeObject(SketchObject *obj);
    // get the list of child objects
    virtual QList<SketchObject *> *getSubObjects();
    virtual const QList<SketchObject *> *getSubObjects() const;
    // collision function... have to change declaration
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics, int pqp_flags);
    virtual void getBoundingBox(double bb[]);
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes();
    virtual vtkAlgorithm *getOrientedHalfPlaneOutlines();
    virtual SketchObject *deepCopy();
protected:
    virtual void localTransformUpdated();
private:
    QList< SketchObject * > children;
    vtkSmartPointer< vtkAppendPolyData > orientedBBs;
    vtkSmartPointer< vtkAppendPolyData > orientedHalfPlaneOutlines;
};

#endif
