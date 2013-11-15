#include "physicsutilities.h"

#include <PQP.h>

#include "sketchioconstants.h"
#include "sketchmodel.h"
#include "sketchobject.h"
#include "springconnection.h"
#include "physicsstrategy.h"

namespace PhysicsUtilities
{

//###################################################################################
void euler(SketchObject* obj, double dt) {
    q_vec_type pos, angularVel,deltaPos,force,torque;
    q_type spin, orient;
    SketchModel* model = obj->getModel();
    double iMass, iMoment;
    if (model != NULL) {
        iMass = model->getInverseMass();
        iMoment = model->getInverseMomentOfInertia();
    } else {
        iMass = DEFAULT_INVERSE_MASS;
        iMoment = DEFAULT_INVERSE_MOMENT;
    }
    // get force & torque
    obj->getForce(force);
    obj->getTorque(torque);
    // new position
    obj->getPosition(pos);
    q_vec_scale(deltaPos,dt*iMass,force);
    q_vec_add(pos,pos,deltaPos);
    // new orientation
    obj->getOrientation(orient);
    q_vec_scale(angularVel,iMoment,torque);
    // I'll summarize this next section like this:
    // quaternion integration is wierd
    spin[Q_W] = 0;
    spin[Q_X] = angularVel[Q_X];
    spin[Q_Y] = angularVel[Q_Y];
    spin[Q_Z] = angularVel[Q_Z];
    q_mult(spin,orient,spin);
    orient[Q_W] += spin[Q_W]*.5*dt;
    orient[Q_X] += spin[Q_X]*.5*dt;
    orient[Q_Y] += spin[Q_Y]*.5*dt;
    orient[Q_Z] += spin[Q_Z]*.5*dt;
    q_normalize(orient,orient);
    // end quaternion integration
    SketchObject* p = obj->getParent();
    if (p != NULL)
    {
        SketchObject::setParentRelativePositionForAbsolutePosition(obj,p,pos,orient);
    }
    else
    {
        obj->setPosAndOrient(pos,orient);
    }
}

//###################################################################################
void applyEuler(QList< SketchObject* >& list,
                double dt, bool clearForces)
{
    int n = list.size();
    for (int i = 0; i < n; i++)
    {
        SketchObject* obj = list.at(i);
        euler(obj,dt);
        if (clearForces)
        {
            obj->clearForces();
        }
    }
}

//###################################################################################
bool collideAndComputeResponse(QList< SketchObject* >& list,
                               QSet< int >& affectedCollisionGroups,
                               bool find_all_collisions,
                               PhysicsStrategy* strategy)
{
    int n = list.size();
    bool foundCollision = false;
    for (int i = 0; i < n; i++) {
        // TODO - self collision once deformation added
        bool needsTest = affectedCollisionGroups.empty();
        for (QSetIterator< int > it(affectedCollisionGroups); it.hasNext();)
        {
            if (list.at(i)->isInCollisionGroup(it.next()))
            {
                needsTest = true;
            }
        }
        if (needsTest)
        {
            for (int j = 0; j < n; j++)
            {
                if (j != i)
                {
                    if (list.at(i)->collide(list.at(j), strategy,
                                            find_all_collisions ?
                                            PQP_ALL_CONTACTS : PQP_FIRST_CONTACT))
                    {
                        foundCollision = true;
                    }
                }
            }
        }
    }
    return foundCollision;
}

bool collideWithinGroupAndComputeResponse(QSet< SketchObject* >& affectedGroups,
                                          QSet< int >& affectedCollisionGroups,
                                          bool find_all_collisions,
                                          PhysicsStrategy* strategy)
{
    bool hasCollision = false;
    for (QSet< SketchObject* >::iterator it = affectedGroups.begin();
         it != affectedGroups.end() && (find_all_collisions || !hasCollision);
         it++)
    {
        hasCollision = hasCollision || collideAndComputeResponse(
                    *(*it)->getSubObjects(),affectedCollisionGroups,
                    find_all_collisions,strategy);
    }
    return hasCollision;
}


bool springForcesFromList(QList< Connector* >& list,
                          QSet< int >& affectedCollisionGroups,
                          QSet< SketchObject* >& affectedGroups)
{
    bool retVal = false;
    for (QListIterator< Connector* > it(list); it.hasNext();)
    {
        Connector* c = it.next();
        bool addedForce = c->addForce();
        retVal = retVal || addedForce;
        if (addedForce)
        {
            int i = OBJECT_HAS_NO_GROUP;
            if (c->getObject1() != NULL)
            {
                SketchObject* o = c->getObject1();
                i = o->getPrimaryCollisionGroupNum();
                if (o->getParent() != NULL && !o->isPropagatingForceToParent())
                {
                    SketchObject* p = o->getParent();
                    while (p != NULL)
                    {
                        affectedGroups.insert(p);
                        affectedCollisionGroups.insert(
                                    p->getPrimaryCollisionGroupNum());
                        p = p->getParent();
                    }
                }
            }
            if (i != OBJECT_HAS_NO_GROUP)
            { // if this one isn't the tracker
                affectedCollisionGroups.insert(i);
            }
            i = OBJECT_HAS_NO_GROUP;
            if (c->getObject2() != NULL)
            {
                SketchObject* o = c->getObject2();
                i = o->getPrimaryCollisionGroupNum();
                if (o->getParent() != NULL && !o->isPropagatingForceToParent())
                {
                    SketchObject* p = o->getParent();
                    while (p != NULL)
                    {
                        affectedGroups.insert(p);
                        affectedCollisionGroups.insert(
                                    p->getPrimaryCollisionGroupNum());
                        p = p->getParent();
                    }
                }
            }
            if (i != OBJECT_HAS_NO_GROUP)
            { // if this one isn't the tracker
                affectedCollisionGroups.insert(i);
            }
        }
    }
    return retVal;
}


void restoreToLastLocation(QList< SketchObject* >& list,
                           QSet< SketchObject* >& affectedGroups)
{
    int n = list.size();
    for (int i = 0; i < n; i++)
    {
        list.at(i)->restoreToLastLocation();
        if (affectedGroups.contains(list.at(i)))
        {
            restoreToLastLocation(*list.at(i)->getSubObjects(),affectedGroups);
        }
    }
}

void setLastLocation(QList< SketchObject*  >& list,
                     QSet< SketchObject*  >& affectedGroups)
{
    int n = list.size();
    for (int i = 0; i < n; i++)
    {
        list.at(i)->setLastLocation();
        if (affectedGroups.contains(list.at(i)))
        {
            setLastLocation(*list.at(i)->getSubObjects(),affectedGroups);
        }
    }
}

void applyEulerToListAndGroups(QList< SketchObject* >& list,
                               QSet< SketchObject* >& affectedGroups,
                               double dt, bool clearForces)
{
    applyEuler(list,dt);
    for (QSetIterator< SketchObject* > it(affectedGroups); it.hasNext(); )
    {
        applyEuler(*it.next()->getSubObjects(),dt,clearForces);
    }
}

void clearForces(QList< SketchObject* >& list,
                 QSet< SketchObject* >& affectedGroups)
{
    for (int i = 0; i < list.size(); i++)
    {
        list.at(i)->clearForces();
        if (affectedGroups.contains(list.at(i)))
        {
            clearForces(*list.at(i)->getSubObjects(),affectedGroups);
        }
    }
}

}
