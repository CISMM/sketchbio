#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <quat.h>
#include "colormaptype.h"

#include <QScopedPointer>

class vtkLineSource;
class vtkActor;
class vtkPolyDataMapper;
class SketchObject;

/*
 * This class represents a connection between two objects or between an object
 * and a location in world space.  A connector may just be an indicator of relatedness
 * or it can apply forces.  It may or may not be displayed to the user.
 */
class Connector
{
public:
    Connector(SketchObject* o1, SketchObject* o2, const q_vec_type o1Pos,
              const q_vec_type o2Pos, double a = 1.0, double rad = 10,
              bool display = true);
    virtual ~Connector();

    inline const SketchObject *getObject1() const { return object1; }
    // if we have a non-const connector, get a non-const object
    inline SketchObject *getObject1() { return object1; }
    // when setting the connector's object, the world position of the
    // endpoint will not move (the relative position is recalculated)
    void setObject1(SketchObject *obj);

    inline const SketchObject *getObject2() const { return object2; }
    // if not a const reference to connector, get non-const obj
    inline SketchObject *getObject2() { return object2; }
    // when setting the connector's object, the world position of the
    // endpoint will not move (the relative position is recalculated)
    void setObject2(SketchObject *obj);

    inline void getObject1ConnectionPosition(q_vec_type out) const
    { q_vec_copy(out,object1ConnectionPosition);}
    inline void getObject2ConnectionPosition(q_vec_type out) const
    { q_vec_copy(out,object2ConnectionPosition);}

    inline void setObject1ConnectionPosition(q_vec_type newPos)
    { q_vec_copy(object1ConnectionPosition,newPos);}
    inline void setObject2ConnectionPosition(q_vec_type newPos)
    { q_vec_copy(object2ConnectionPosition,newPos);}

    void getEnd1WorldPosition(q_vec_type out) const;
    void setEnd1WorldPosition(const q_vec_type newPos);
    void getEnd2WorldPosition(q_vec_type out) const;
    void setEnd2WorldPosition(const q_vec_type newPos);
	
	double getLength();

	void snapToTerminus(bool on_object1, bool snap_to_n);

    // These may return NULL if the connector has no line & actor
    // some connectors may not have this
    vtkLineSource *getLine();
    vtkActor *getActor();
	ColorMapType::Type getColorMapType() const { return colorMap; }
	void setColorMapType(ColorMapType::Type cmap);

    // get alpha
    inline double getAlpha() const { return alpha; }
    // get radius
    inline double getRadius() const { return radius; }

    // So that springs don't have to be dynamic casted...
    // default does nothing
    virtual bool addForce() { return false; }
    // updates the displayed line (if there is one)
    void updateLine();

protected:
    // updates the displayed line with the new color map (if there is one)
    void updateColorMap();

    SketchObject* object1, * object2;
    q_vec_type object1ConnectionPosition, object2ConnectionPosition;
    double alpha, radius;
private:
    // class to represent the visibility state of the connector
    class VisiblityData;
    // have to forward declare this here or can't extend VisibilityData
    class LineVisibilityData;
    QScopedPointer< VisiblityData > d;
	ColorMapType::Type colorMap;
};



#endif // CONNECTOR_H
