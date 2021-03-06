#ifndef TRANSFORMEQUALS_H
#define TRANSFORMEQUALS_H

#include <quat.h>

#include <QVector>

#include <vtkSmartPointer.h>
class vtkTransform;

class SketchObject;
#include "objectchangeobserver.h"
class GroupIdGenerator;

/*
 * This is a simple structure to associate a pair of SketchObjects together for use with the TransformEquals class
 */
typedef struct ObjectPair {
    SketchObject *o1;
    SketchObject *o2;
    ObjectPair(SketchObject *first, SketchObject *second);
    ObjectPair();
} ObjectPair;

/*
 * This class implements a transform equality between pairs of objects.  Essentially you can say A is to B as C is
 * to D, when talking about the transformations between the objects.  One pair of objects is the "master" from which
 * the transformation is copied to all the others, however when any object experiences a force, it alerts its
 * observers and if this is alerted, it makes the object that alerted it the master.  This operator only listens for
 * forces on the second object, the first can be used freely to position the pair, the moving the second object
 * in the pair changes the relation for all pairs of objects in this operator.
 */
class TransformEquals : public ObjectChangeObserver
{
public:
    TransformEquals(SketchObject *first, SketchObject *second, GroupIdGenerator *gen);
    virtual ~TransformEquals();
    // have to check if there are multiple TransformEquals
    // affecting the object
    // also, object should not be both first and second object in separate pairs of the transformequals...
    // causes a circular dependency between transformations
    // returns true if the objects were added, false if they failed the above test
    bool addPair(SketchObject *first, SketchObject *second);
    // remove pairs
    void removePairAt(int i);
    void removePair(SketchObject *first, SketchObject *second);
    void removePairByFirst(SketchObject *first);
    void removePairBySecond(SketchObject *second);
    // get list of pairs
    const QVector<ObjectPair> *getPairsList() const;
    // update transforms when force applied to an object in one of the pairs
    virtual void objectPushed(SketchObject *obj);
    // when one of the objects is keyframed, apply keyframes to objects that generate its position
    virtual void objectKeyframed(SketchObject *obj, double time);
    // when one of the first two objects is moved, update the other or the transform between them
    virtual void objectMoved(SketchObject *obj);
private:
    // Disable copy constructor and assignment operator these are not implemented
    // and not supported
    TransformEquals(const TransformEquals &other);
    TransformEquals &operator=(const TransformEquals &other);

    // fields
    QVector<ObjectPair> pairsList;
    vtkSmartPointer<vtkTransform> transform;
    int transformEqualsGroupId;
    bool isMovingBase;
    q_vec_type posOffset;
    q_type orientOffset;
};

#endif // TRANSFORMEQUALS_H
