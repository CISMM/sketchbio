#ifndef PHYSICSSTRATEGYFACTORY_H
#define PHYSICSSTRATEGYFACTORY_H

#include <QVector>
#include <QSharedPointer>

class PhysicsStrategy;


/*
 * For testing only: dynamic changing of collision response types. Each enum is a
 * response strategy
 */
namespace PhysicsMode {
    enum Type {
        ORIGINAL_COLLISION_RESPONSE=0,
        POSE_MODE_TRY_ONE=1,
        BINARY_COLLISION_SEARCH=2,
        POSE_WITH_PCA_COLLISION_RESPONSE=3
    };
}

namespace PhysicsStrategyFactory {
    /*******************************************************************
     *
     * This method populates the list of collision strategies so that their
     * index in the list is the same as the value of the Type enum that
     * should trigger them.
     *
     *******************************************************************/
    void populateStrategies(QVector<QSharedPointer<PhysicsStrategy> > &strategies);
}

#endif
