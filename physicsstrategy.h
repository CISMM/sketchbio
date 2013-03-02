#ifndef COLLISIONSTRATEGY_H
#define COLLISIONSTRATEGY_H

#include <QList>
#include <QSet>
#include <PQP.h>

// Forward declare spring and object... circular dependency with object
class SketchObject;
class SpringConnection;

/*
 * This class implements the Strategy Pattern for collision response techniques for SketchBio.
 *
 * This is an abstract class that defines the interface for a collision strategy to be implemented
 * by each subclass to define how to handle collision detection.
 */

class PhysicsStrategy
{
public:
    PhysicsStrategy();
    virtual ~PhysicsStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
                                                 QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
                                                 QList<SketchObject *> &objects, double dt, bool doCollisionCheck) = 0;
    virtual
    void respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags) = 0;
};

/*
 * This class implements the original physics mode where all objects are subject to response forces and all
 * forces are applied at once.
 */
class SimplePhysicsStrategy : public PhysicsStrategy {
public:
    SimplePhysicsStrategy();
    virtual ~SimplePhysicsStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
                                                 QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
                                                 QList<SketchObject *> &objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags);
};

/*
 * This class implements my first try at pose mode physics.  Only those objects which moved during the
 * application of the springs are subject to collision response, the others are fixed.  In addition, the
 * springs are applied separately from different sources, first right hand, then left, then world physics.
 *
 * Note: this class uses internal state to keep track of which objects are affected by springs, so it is not
 * safe to call from multiple threads
 */
class PoseModePhysicsStrategy : public PhysicsStrategy {
public:
    PoseModePhysicsStrategy();
    virtual ~PoseModePhysicsStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
                                                 QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
                                                 QList<SketchObject *> &objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags);
private:
    void poseModeForSprings(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                               double dt, bool doCollisionCheck);
    QSet<int> affectedGroups;
};

/*
 * This class implements the binary collision search method of collision detection and response where collisions are
 * never allowed and the motion is always undone if it results in a collision. This strategy is like PoseMode in that
 * forces from different sources are applied or rejected separately.
 */
class BinaryCollisionSearchStrategy : public PhysicsStrategy {
public:
    BinaryCollisionSearchStrategy();
    virtual ~BinaryCollisionSearchStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
                                                 QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
                                                 QList<SketchObject *> &objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags);
};

/*
 * This class implements my second try at pose mode physics.  Only those objects which moved during the
 * application of the springs are subject to collision response, the others are fixed.  In addition, the
 * springs are applied separately from different sources, first right hand, then left, then world physics.
 *
 * This class is different from PoseModePhysicsStrategy in that it uses principal component analysis to
 * determine the response force instead of the normals of each triangle involved in the collision.
 *
 * Note: this class uses internal state to keep track of which objects are affected by springs, so it is not
 * safe to call from multiple threads
 */
class PoseModePCAPhysicsStrategy : public PhysicsStrategy {
public:
    PoseModePCAPhysicsStrategy();
    virtual ~PoseModePCAPhysicsStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
                                                 QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
                                                 QList<SketchObject *> &objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags);
private:
    void poseModePCAForSprings(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                               double dt, bool doCollisionCheck);
    QSet<int> affectedGroups;
};
#endif // COLLISIONSTRATEGY_H
