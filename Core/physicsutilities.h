#ifndef PHYSICSUTILITIES_H
#define PHYSICSUTILITIES_H

#include <QList>

class SketchObject;
class Connector;
class PhysicsStrategy;

namespace PhysicsUtilities
{
// Applies euler integration on the object once its force and
// torque have been determined
void euler(SketchObject* obj,double dt);
// Loops over the objects in the list and applies the euler
// function to each one with the given time interval.  If the
// boolean flag is true it then clears the forces on the objects
void applyEuler(QList< SketchObject* >& list, double dt,
                bool clearForces = true);
// Computes the collision response force on the given list of objects
// affectedCollisionGroups - the set of collision groups that are affected,
//                              if empty it does full n^2 collision tests
// find_all_collisions - whether to find all collisions an allow the strategy
//                          to respond to them or return after the first one
//                          is found
// strategy - the PhysicsStrategy object that will be passed to collide and
//                compute the response
// returns: true if a collision was found, false otherwise
bool collideAndComputeResponse(QList< SketchObject* >& list,
                               QSet< int >& affectedCollisionGroups,
                               bool find_all_collisions,
                               PhysicsStrategy* strategy);
// Test internal collisions for groups whose members moved.  The first parameter
// is the set of groups to test, the rest are the same as collideAndComputeResponse
bool collideWithinGroupAndComputeResponse(QSet< SketchObject* >& affectedGroups,
                                          QSet< int >& affectedCollisionGroups,
                                          bool find_all_collisions,
                                          PhysicsStrategy* strategy);
// Adds the spring forces from the list of springs to the objects that springs
// are attached to.  Output values are placed in the two QSet objects.  The
// affectedCollisionGroups set will contain the primary collision groups
// of all objects affected by the forces from the springs and the affectedGroups
// set will contain the parent groups of the objects (if any objects have a parent
// group)
// Returns true if some forces were added, false if this was a no-op
bool springForcesFromList(QList< Connector* >& list,
                          QSet< int >& affectedCollisionGroups,
                          QSet< SketchObject* >& affectedGroups);
// Restores each object in the list to its former location and then recurses on the
// objects in the affectedGroups set, setting all their sub-objects back the their
// previous locations
void restoreToLastLocation(QList< SketchObject* >& list,
                           QSet< SketchObject* >& affectedGroups);
// Sets the restore point for each object in the list, and then recurses on each object
// in the given groups
void setLastLocation(QList< SketchObject* >& list,
                     QSet< SketchObject* >& affectedGroups);
// Performs the euler function on each object in the list and each sub-object of the
// given groups in affectedGroups
void applyEulerToListAndGroups(QList< SketchObject* >& list,
                               QSet< SketchObject* >& affectedGroups,
                               double dt, bool clearForces);
// Clears force and torque from the objects in the list and the children
// of the objects in the set
void clearForces(QList< SketchObject* >& list,
                 QSet< SketchObject* >& affectedGroups);
}

#endif // PHYSICSUTILITIES_H
