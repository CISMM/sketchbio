#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include <QString>

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
    virtual int getModelConformation() const;
    virtual vtkActor *getActor();
    // collision function that depend on data in this subclass
    virtual bool collide(SketchObject *other, PhysicsStrategy *physics,
                         int pqp_flags);
    virtual void getBoundingBox(double bb[]);
    virtual vtkPolyDataAlgorithm *getOrientedBoundingBoxes();
    virtual vtkAlgorithm *getOrientedHalfPlaneOutlines();
    virtual SketchObject *getCopy();
    virtual SketchObject* deepCopy();

	virtual void showFullResolution();
	virtual void hideFullResolution();

	// the luminance relative to min and max luminance (where 0 will result in 
	// displaying the minimum luminance and 1 will result in the max)
	virtual double getLuminance() const;
	// actual luminance level that gets displayed
	virtual double getDisplayLuminance() const;
	virtual void setLuminance(double lum);
	virtual void setMinLuminance(double minLum);
	virtual void setMaxLuminance(double maxLum);
	virtual void updateColorMap();
protected:
    virtual void setSolidColor(double color[3]);
private:
    // Disable copy constructor and assignment operator these are not implemented
    // and not supported
    ModelInstance(const ModelInstance &other);
    ModelInstance &operator=(const ModelInstance &other);

    // fields
	double luminance, minLuminance, maxLuminance;
    vtkSmartPointer<vtkActor> actor;
    SketchModel *model;
    int conformation;
	// true if overriding default resolution for the conformation to show full
	// resolution instead (because it is grabbed or near a grabbed object)
	bool displayingFullRes;
    vtkSmartPointer< vtkTransformPolyDataFilter > orientedBB;
    vtkSmartPointer< vtkTransformPolyDataFilter > orientedHalfPlaneOutlines;
};

#endif
