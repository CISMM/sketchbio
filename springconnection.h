#ifndef SPRINGCONNECTION_H
#define SPRINGCONNECTION_H

#include "quat.h"
#include "sketchobject.h"
#include <vtkType.h>

class SpringConnection
{
public:
    SpringConnection(SketchObject *o1, SketchObject *o2, double restLen,
                     double k, q_vec_type obj1Pos, q_vec_type obj2Pos);

    inline double getStiffness() const { return stiffness; }
    inline void setStiffness(double newK) { stiffness = newK; }
    inline void getObject1ConnectionPosition(q_vec_type out) const { q_vec_copy(out,object1ConnectionPosition);}
    inline void setObject1ConnectionPosition(q_vec_type newPos) { q_vec_copy(object1ConnectionPosition,newPos);}
    inline void getObject2ConnectionPosition(q_vec_type out) const { q_vec_copy(out,object2ConnectionPosition);}
    inline void setObject2ConnectionPosition(q_vec_type newPos) { q_vec_copy(object2ConnectionPosition,newPos);}
    void getEnd1WorldPosition(q_vec_type out) const;
    void getEnd2WorldPosition(q_vec_type out) const;
    inline vtkIdType getEnd1Id() const { return end1;}
    inline void setEnd1Id(vtkIdType id) { end1 = id;}
    inline vtkIdType getEnd2Id() const { return end2;}
    inline void setEnd2Id(vtkIdType id) { end2 = id;}
    inline vtkIdType getCellId() const { return cellId; }
    inline void setCellId(vtkIdType id) { cellId = id;}

    void addForce();

private:
    SketchObject *object1, *object2;
    double restLength;
    double stiffness;
    vtkIdType end1, end2, cellId;
    q_vec_type object1ConnectionPosition, object2ConnectionPosition;
};

#endif // SPRINGCONNECTION_H