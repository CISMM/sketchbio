#ifndef SPRINGCONNECTION_H
#define SPRINGCONNECTION_H

#include "quat.h"
class SketchObject;
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

    SpringConnection(SketchObject *o1, SketchObject *o2, double minRestLen,
                     double maxRestLen, double k, const q_vec_type obj1Pos,
                     const q_vec_type obj2Pos);

    inline double getStiffness() const { return stiffness; }
    inline void setStiffness(double newK) { stiffness = newK; }
    inline double getMinRestLength() const { return minRestLength; }
    inline double getMaxRestLength() const { return maxRestLength; }

    inline void getObject1ConnectionPosition(q_vec_type out) const
    { q_vec_copy(out,object1ConnectionPosition);}
    inline void getObject2ConnectionPosition(q_vec_type out) const
    { q_vec_copy(out,object2ConnectionPosition);}

    inline const SketchObject *getObject1() const { return object1; }
    // if we have a non-const spring, get a non-const object
    inline SketchObject *getObject1() { return object1; }
    // when setting the spring's object, the world position of the
    // endpoint will not move (the relative position is recalculated)
    void setObject1(SketchObject *obj);

    inline const SketchObject *getObject2() const { return object2; }
    // if not a const reference to spring, get non-const obj
    inline SketchObject *getObject2() { return object2; }
    // when setting the spring's object, the world position of the
    // endpoint will not move (the relative position is recalculated)
    void setObject2(SketchObject *obj);

    inline void setObject1ConnectionPosition(q_vec_type newPos)
    { q_vec_copy(object1ConnectionPosition,newPos);}
    inline void setObject2ConnectionPosition(q_vec_type newPos)
    { q_vec_copy(object2ConnectionPosition,newPos);}

    void getEnd1WorldPosition(q_vec_type out) const;
    void setEnd1WorldPosition(const q_vec_type newPos);
    void getEnd2WorldPosition(q_vec_type out) const;
    void setEnd2WorldPosition(const q_vec_type newPos);

#ifdef SHOW_DEBUGGING_FORCE_LINES
    inline vtkIdType getEnd1Id() const { return end1;}
    inline void setEnd1Id(vtkIdType id) { end1 = id;}
    inline vtkIdType getEnd2Id() const { return end2;}
    inline void setEnd2Id(vtkIdType id) { end2 = id;}
    inline vtkIdType getCellId() const { return cellId; }
    inline void setCellId(vtkIdType id) { cellId = id;}
#endif

    void addForce();

private:
    SketchObject *object1, *object2;
    double minRestLength, maxRestLength;
    double stiffness;
    q_vec_type object1ConnectionPosition, object2ConnectionPosition;
#ifdef SHOW_DEBUGGING_FORCE_LINES
    vtkIdType end1, end2, cellId;
#endif
public: // static factories
    /*******************************************************************
     *
     * Creates the spring between the two objects and returns a pointer to it.
     *
     * o1 - the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world coordinate space,
     *                  - false if they are relative to the model coordiante space
     * k - the stiffness of the spring
     * minLen - the minimum rest length of the spring
     * maxLen - the maximum rest length of the spring
     *
     *******************************************************************/
    static SpringConnection *makeSpring(SketchObject *o1, SketchObject *o2,
                                        const q_vec_type pos1, const q_vec_type pos2,
                                        bool worldRelativePos, double k,
                                        double minLen, double maxLen);
    /*******************************************************************
     *
     * Creates the spring between the two objects and returns a pointer to it.
     *
     * o1 - the first object to connect
     * o2 - the second object to connect
     * pos1 - the position where the spring connects to the first model
     * pos2 - the position where the spring connects to the second model
     * worldRelativePos - true if the above positions are relative to the world coordinate space,
     *                  - false if they are relative to the model coordiante space
     * k - the stiffness of the spring
     * len - the length of the spring
     *
     *******************************************************************/
    static SpringConnection *makeSpring(SketchObject *o1, SketchObject *o2,
                                        const q_vec_type pos1, const q_vec_type pos2,
                                        bool worldRelativePos, double k, double len);
};


inline SpringConnection *SpringConnection::makeSpring(SketchObject *o1,
                                                      SketchObject *o2,
                                                      const q_vec_type pos1,
                                                      const q_vec_type pos2,
                                                      bool worldRelativePos,
                                                      double k, double len) {
    return makeSpring(o1,o2,pos1,pos2,worldRelativePos,k,len,len);
}

#endif // SPRINGCONNECTION_H
