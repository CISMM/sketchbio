#ifndef SPRINGCONNECTION_H
#define SPRINGCONNECTION_H

#include "quat.h"
#include "sketchobject.h"
#include <vtkType.h>

/*
 * This class represents a general Spring and contains all the data needed by every spring
 * such as rest length (range) and stiffness.  It also contains a reference to a SketchObject
 * since every spring must connect to at least one object.
 * It also contains some visualization parameters (end ids in a vtkPoints and cell id
 * in a vtkPolyData).
 *
 * This class is abstract and designed to be subclassed to define specific types of springs
 */

class SpringConnection
{
public:

    SpringConnection(SketchObject *o1, double minRestLen, double maxRestLen,
                     double k, const q_vec_type obj1Pos);

    inline double getStiffness() const { return stiffness; }
    inline void setStiffness(double newK) { stiffness = newK; }
    inline double getMinRestLength() const { return minRestLength; }
    inline double getMaxRestLength() const { return maxRestLength; }
    inline void getObject1ConnectionPosition(q_vec_type out) const { q_vec_copy(out,object1ConnectionPosition);}
    inline const SketchObject *getObject1() const { return object1; }
    inline void setObject1ConnectionPosition(q_vec_type newPos) { q_vec_copy(object1ConnectionPosition,newPos);}
    void getEnd1WorldPosition(q_vec_type out) const;
    virtual void getEnd2WorldPosition(q_vec_type out) const = 0;
    inline vtkIdType getEnd1Id() const { return end1;}
    inline void setEnd1Id(vtkIdType id) { end1 = id;}
    inline vtkIdType getEnd2Id() const { return end2;}
    inline void setEnd2Id(vtkIdType id) { end2 = id;}
    inline vtkIdType getCellId() const { return cellId; }
    inline void setCellId(vtkIdType id) { cellId = id;}

    virtual void addForce() = 0;

protected:
    SketchObject *object1;
    double minRestLength, maxRestLength;
    double stiffness;
    q_vec_type object1ConnectionPosition;
private:
    vtkIdType end1, end2, cellId;
};

/*
 * This class extends SpringConnection to define a spring between two objects.
 */
class InterObjectSpring : public SpringConnection
{
public:
    InterObjectSpring(SketchObject *o1, SketchObject *o2, double minRestLen, double maxRestLen,
                        double k, const q_vec_type obj1Pos, const q_vec_type obj2Pos);

    inline void getObject2ConnectionPosition(q_vec_type out) const { q_vec_copy(out,object2ConnectionPosition);}
    inline const SketchObject *getObject2() const { return object2; }
    inline void setObject2ConnectionPosition(q_vec_type newPos) { q_vec_copy(object2ConnectionPosition,newPos);}
    virtual void getEnd2WorldPosition(q_vec_type out) const;

    virtual void addForce();
private:
    SketchObject *object2;
    q_vec_type object2ConnectionPosition;
};

/*
 * This class extends SpringConnection to define a spring between an object and a location in the world
 */
class ObjectPointSpring : public SpringConnection
{
public:
    ObjectPointSpring(SketchObject *o1, double minRestLen, double maxRestLen, double k,
                        const q_vec_type obj1Pos, const q_vec_type worldPoint);

    inline void setWorldPoint(q_vec_type newPos) { q_vec_copy(point,newPos); }
    virtual void getEnd2WorldPosition(q_vec_type out) const;

    virtual void addForce();
private:
    q_vec_type point;
};

#endif // SPRINGCONNECTION_H
