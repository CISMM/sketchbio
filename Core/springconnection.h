#ifndef SPRINGCONNECTION_H
#define SPRINGCONNECTION_H

#include <quat.h>

#include "connector.h"

/*
 * This class represents a general Spring and contains all the data needed by every spring
 * such as rest length (range) and stiffness.  It also contains a reference to a SketchObject
 * since every spring must connect to at least one object.
 * It also contains some visualization parameters (end ids in a vtkPoints and cell id
 * in a vtkPolyData).
 *
 * This class is abstract and designed to be subclassed to define specific types of springs
 */

class SpringConnection : public Connector
{
public:
    SpringConnection();
    SpringConnection(SketchObject *o1, SketchObject *o2, double minRestLen,
                     double maxRestLen, double k, const q_vec_type obj1Pos,
                     const q_vec_type obj2Pos, bool showLine);
    virtual ~SpringConnection() {}

    void initSpring(SketchObject *o1, SketchObject *o2, double minRestLen,
                    double maxRestLen, double k, const q_vec_type obj1Pos,
                    const q_vec_type obj2Pos, bool showLine);
    virtual bool isInitialized();

    inline double getStiffness() const { return stiffness; }
    inline void setStiffness(double newK) { stiffness = newK; }
    inline double getMinRestLength() const { return minRestLength; }
    inline double getMaxRestLength() const { return maxRestLength; }

    virtual bool addForce();

private:
    double minRestLength, maxRestLength;
    double stiffness;
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
                                        double minLen, double maxLen,
                                        bool showLine);
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
                                        bool worldRelativePos, double k, double len,
                                        bool showLine);
};


inline SpringConnection* SpringConnection::makeSpring(SketchObject *o1,
                                                      SketchObject *o2,
                                                      const q_vec_type pos1,
                                                      const q_vec_type pos2,
                                                      bool worldRelativePos,
                                                      double k,
                                                      double len,
                                                      bool showLine)
{
    return makeSpring(o1,o2,pos1,pos2,worldRelativePos,k,len,len,showLine);
}

#endif // SPRINGCONNECTION_H
