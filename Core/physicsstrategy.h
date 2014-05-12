#ifndef COLLISIONSTRATEGY_H
#define COLLISIONSTRATEGY_H

#include <QList>
struct PQP_CollideResult;

// Forward declare spring and object... circular dependency with object
class SketchObject;
class Connector;

/*
 * This class implements the Strategy Pattern for collision response techniques
 *for SketchBio.
 *
 * This is an abstract class that defines the interface for a collision strategy
 *to be implemented
 * by each subclass to define how to handle collision detection.
 */

class PhysicsStrategy
{
 public:
  PhysicsStrategy();
  virtual ~PhysicsStrategy();

  virtual void performPhysicsStepAndCollisionDetection(
      QList< Connector * > &uiSprings, QList< Connector * > &physicsSprings,
      bool doPhysicsSprings, QList< SketchObject * > &objects, double dt,
      bool doCollisionCheck) = 0;
  virtual void respondToCollision(SketchObject *o1, SketchObject *o2,
                                  PQP_CollideResult *cr, int pqp_flags) = 0;
  private:
    // Disable copy constructor and assignment operator these are not implemented
    // and not supported
  PhysicsStrategy(const PhysicsStrategy &other);
  PhysicsStrategy &operator=(const PhysicsStrategy &other);
};

#endif  // COLLISIONSTRATEGY_H
