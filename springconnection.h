#ifndef SPRINGCONNECTION_H
#define SPRINGCONNECTION_H

#include "quat.h"
#include "sketchobject.h"
#include <vtkType.h>

/*
 * This class represents a Spring and has all the data needed to calculate the
 * spring force on the two objects it is connected to, such as attachment position,
 * stiffness, and rest length.
 *
 * It also contains some visualization parameters (end ids in a vtkPoints and cell id
 * in a vtkPolyData)
 */

class SpringConnection
{
public:
    SpringConnection(ObjectId o1, ObjectId o2, double restLen,
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
    ObjectId object1, object2;
    double restLength;
    double stiffness;
    vtkIdType end1, end2, cellId;
    q_vec_type object1ConnectionPosition, object2ConnectionPosition;
};

/*
 * The springs are stored in a list and referenced via these iterators in
 * other parts of the program
 */
typedef std::list<SpringConnection *>::iterator SpringId;

#endif // SPRINGCONNECTION_H
