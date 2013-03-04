#include "physicsstrategy.h"
#include <QSet>
#include <quat.h>
#include <sketchobject.h>
#include <springconnection.h>
#include <modelmanager.h>  // for #define default mass and moment of inertia

//######################################################################################
//######################################################################################
// Physics Strategy - abstract superclass
//######################################################################################
//######################################################################################

PhysicsStrategy::PhysicsStrategy() {}

PhysicsStrategy::~PhysicsStrategy() {}
//######################################################################################
//######################################################################################
// Inline helper functions - used to share code between strategies
//######################################################################################
//######################################################################################

// magic # force to use for collisions
#define COLLISION_FORCE 5

//######################################################################################
/*
 * This next bit was stolen from PQP's matrix library.  It computes the eigenvalues/eigenvectors
 * of a matrix.  The #define was part of its code
 */
#define ROTATE(a,i,j,k,l) {g=a[i][j]; h=a[k][l]; a[i][j]=g-s*(h+g*tau); a[k][l]=h+s*(g-h*tau);}

//######################################################################################
// computes the eigenvalues and eigenvectors of the matrix a
// returns eigenvectors as columns of vout and eigenvalues in dout
static void
inline
Meigen(PQP_REAL vout[3][3], PQP_REAL dout[3], PQP_REAL a[3][3])
{
  int n = 3;
  int j,iq,ip,i;
  PQP_REAL tresh,theta,tau,t,sm,s,h,g,c;
  int nrot;
  PQP_REAL b[3];
  PQP_REAL z[3];
  PQP_REAL v[3][3];
  PQP_REAL d[3];

  // identity
  v[0][0] = v[1][1] = v[2][2] = 1;
  v[0][1] = v[0][2] = v[1][0] = v[1][2] = v[2][0] = v[2][1] = 0;

  for(ip=0; ip<n; ip++)
    {
      b[ip] = a[ip][ip];
      d[ip] = a[ip][ip];
      z[ip] = 0.0;
    }

  nrot = 0;

  for(i=0; i<50; i++)
    {

      sm=0.0;
      for(ip=0;ip<n;ip++) for(iq=ip+1;iq<n;iq++) sm+=fabs(a[ip][iq]);
      if (sm == 0.0)
    {
          // matrix & vector copies
          for (int ii = 0; ii < 3; ii++) {
              for (int jj = 0; jj < 3; jj++) {
                  vout[ii][jj] = v[ii][jj];
              }
              dout[ii] = d[ii];
          }
      return;
    }


      if (i < 3) tresh=(PQP_REAL)0.2*sm/(n*n);
      else tresh=0.0;

      for(ip=0; ip<n; ip++) for(iq=ip+1; iq<n; iq++)
    {
      g = (PQP_REAL)100.0*fabs(a[ip][iq]);
      if (i>3 &&
          fabs(d[ip])+g==fabs(d[ip]) &&
          fabs(d[iq])+g==fabs(d[iq]))
        a[ip][iq]=0.0;
      else if (fabs(a[ip][iq])>tresh)
        {
          h = d[iq]-d[ip];
          if (fabs(h)+g == fabs(h)) t=(a[ip][iq])/h;
          else
        {
          theta=(PQP_REAL)0.5*h/(a[ip][iq]);
          t=(PQP_REAL)(1.0/(fabs(theta)+sqrt(1.0+theta*theta)));
          if (theta < 0.0) t = -t;
        }
          c=(PQP_REAL)1.0/sqrt(1+t*t);
          s=t*c;
          tau=s/((PQP_REAL)1.0+c);
          h=t*a[ip][iq];
          z[ip] -= h;
          z[iq] += h;
          d[ip] -= h;
          d[iq] += h;
          a[ip][iq]=0.0;
          for(j=0;j<ip;j++) { ROTATE(a,j,ip,j,iq); }
          for(j=ip+1;j<iq;j++) { ROTATE(a,ip,j,j,iq); }
          for(j=iq+1;j<n;j++) { ROTATE(a,ip,j,iq,j); }
          for(j=0;j<n;j++) { ROTATE(v,j,ip,j,iq); }
          nrot++;
        }
    }
      for(ip=0;ip<n;ip++)
    {
      b[ip] += z[ip];
      d[ip] = b[ip];
      z[ip] = 0.0;
    }
    }

  fprintf(stderr, "eigen: too many iterations in Jacobi transform.\n");

  return;
}

#undef ROTATE
// end code stolen from PQP

//##################################################################################################
// helper method for stepPhysics -- does Euler's method once force
// and torque on an object have been calculated
static inline void euler(SketchObject *obj, double dt) {
    q_vec_type pos, angularVel,deltaPos,force,torque;
    q_type spin, orient;
//    if (obj->doPhysics()) {
        SketchModel *model = obj->getModel();
        double iMass, iMoment;
        if (model != NULL) {
            iMass = model->getInverseMass();
            iMoment = model->getInverseMomentOfInertia();
        } else {
            iMass = INVERSEMASS;
            iMoment = INVERSEMOMENT;
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
        obj->setPosAndOrient(pos,orient);
//    }
}


//##################################################################################################
// helper function-- compute the normal n of triangle tri in the model
// assumes points in each triangle in the model are added in counterclockwise order
// relative to the outside of the surface looking down
static inline void unit_normal(q_vec_type n, PQP_Model *m, int t) {
    q_vec_type diff1, diff2;
#ifdef PQP_UPDATE_EPSILON
    Tri *tri = &(m->tris[m->idsToIndices[t]]);
#else
    Tri *tri = NULL;
    for (int i = 0; i < m->num_tris; i++) {
        if (m->tris[i].id == t) {
            tri = &(m->tris[i]);
            break;
        }
    }
#endif
    q_vec_subtract(diff1,tri->p3,tri->p1);
    q_vec_subtract(diff2,tri->p2,tri->p1);
    q_vec_cross_product(n,diff2,diff1);
    q_vec_normalize(n,n);
}

//##################################################################################################
// helper function -- compute the centriod of the triangle
static inline void centriod(q_vec_type c, PQP_Model *m, int t) {
#ifdef PQP_UPDATE_EPSILON
    Tri *tri = &(m->tris[m->idsToIndices[t]]);
#else
    Tri *tri = NULL;
    for (int i = 0; i < m->num_tris; i++) {
        if (m->tris[i].id == t) {
            tri = &(m->tris[i]);
            break;
        }
    }
#endif
    q_vec_copy(c,tri->p1);
    q_vec_add(c,tri->p2,c);
    q_vec_add(c,tri->p3,c);
    q_vec_scale(c,1/3.0,c);
}

//##################################################################################################
// -helper function to compute mean point of all triangle points where the triangles are involved in
//   the collision
static inline void computeMeanCollisionPoint(PQP_REAL mean[3], PQP_CollideResult *cr,
                                      SketchModel *model1, bool isFirst,
                                      PQP_Model *m1) {
    int total = 3 * cr->NumPairs();
    mean[0] = mean[1] = mean[2] = 0.0;

    for (int i = 0; i < cr->NumPairs(); i++) {
        int m1Tri;
        if (isFirst)
            m1Tri= cr->Id1(i);
        else
            m1Tri = cr->Id2(i);
        int triId1;
#ifndef PQP_UPDATE_EPSILON
        triId1 = model1->getTriIdToTriIndexHash()->value(m1Tri);
#else
        triId1 = m1->idsToIndices(m1Tri);
#endif
        mean[0] += m1->tris[triId1].p1[0] + m1->tris[triId1].p2[0] + m1->tris[triId1].p3[0];
        mean[1] += m1->tris[triId1].p1[1] + m1->tris[triId1].p2[1] + m1->tris[triId1].p3[1];
        mean[2] += m1->tris[triId1].p1[2] + m1->tris[triId1].p2[2] + m1->tris[triId1].p3[2];
    }
    mean[0] = mean[0] / total;
    mean[1] = mean[1] / total;
    mean[2] = mean[2] / total;
}

//##################################################################################################
// -helper function to compute the covariance of all triangle points involved in the collision on the
//   given model
static inline void computeCollisonPointCovariance(PQP_REAL mean[3], PQP_CollideResult *cr,
                                           SketchModel *model1, bool isFirst,
                                           PQP_Model *m1, PQP_REAL cov[3][3]) {
    int total = 3 * cr->NumPairs();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cov[i][j] = 0;
        }
    }
    for (int k = 0; k < cr->NumPairs(); k++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int m1Tri;
                if (isFirst) {
                    m1Tri = cr->Id1(k);
                } else {
                    m1Tri = cr->Id2(k);
                }
                int triId1;
#ifndef PQP_UPDATE_EPSILON
                triId1 = model1->getTriIdToTriIndexHash()->value(m1Tri);
#else
                triId1 = m1->idsToIndices(m1Tri);
#endif
                cov[i][j] += (mean[i] - m1->tris[triId1].p1[i]) *
                             (mean[j] - m1->tris[triId1].p1[j])
                          +  (mean[i] - m1->tris[triId1].p2[i]) *
                             (mean[j] - m1->tris[triId1].p2[j])
                          +  (mean[i] - m1->tris[triId1].p3[i]) *
                             (mean[j] - m1->tris[triId1].p3[j]);
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cov[i][j] = cov[i][j] / (total-1);
        }
    }
}

//##################################################################################################
static inline void applyPCACollisionResponseForce(SketchObject *o1, SketchObject *o2,
                                               PQP_CollideResult *cr, QSet<int> &affectedGroups) {
    // get the collision models:
    SketchModel *model1 = o1->getModel();
    SketchModel *model2 = o2->getModel();
    PQP_Model *m1 = model1->getCollisionModel();
    PQP_Model *m2 = model2->getCollisionModel();

    // get object's orientations
    q_type quat1, quat2;
    (o1)->getOrientation(quat1);
    (o2)->getOrientation(quat2);

    PQP_REAL mean1[3], covariance1[3][3];
    PQP_REAL mean2[3], covariance2[3][3];

    computeMeanCollisionPoint(mean1,cr,model1,true,m1);
    computeCollisonPointCovariance(mean1,cr,model1,true,m1,covariance1);
    PQP_REAL covEigenVecs1[3][3], covEigenVals1[3];
    Meigen(covEigenVecs1,covEigenVals1,covariance1);
    int min1 = (covEigenVals1[0] < covEigenVals1[1]) ?
                ((covEigenVals1[2] < covEigenVals1[0]) ? 2 : 0) :
                ((covEigenVals1[2] < covEigenVals1[1]) ? 2 : 1);

    computeMeanCollisionPoint(mean2,cr,model2,false,m2);
    computeCollisonPointCovariance(mean2,cr,model2,false,m2,covariance2);
    PQP_REAL covEigenVecs2[3][3], covEigenVals2[3];
    Meigen(covEigenVecs2,covEigenVals2,covariance2);
    int min2 = (covEigenVals2[0] < covEigenVals2[1]) ?
                ((covEigenVals2[2] < covEigenVals2[0]) ? 2 : 0) :
                ((covEigenVals2[2] < covEigenVals2[1]) ? 2 : 1);

    q_vec_type dir1, dir2;
    dir1[0] = covEigenVecs1[0][min1];
    dir1[1] = covEigenVecs1[1][min1];
    dir1[2] = covEigenVecs1[2][min1];
    dir2[0] = covEigenVecs2[0][min2];
    dir2[1] = covEigenVecs2[1][min2];
    dir2[2] = covEigenVecs2[2][min2];
    q_vec_normalize(dir1,dir1);
    q_vec_normalize(dir2,dir2);
    if (q_vec_dot_product(dir1,mean1) < 0) {
        q_vec_invert(dir1,dir1);
    }
    if (q_vec_dot_product(dir2,mean2) < 0) {
        q_vec_invert(dir2,dir2);
    }

    q_vec_type dir1W, dir2W;
    q_vec_type mean1W, mean2W;
    o1->getModelSpacePointInWorldCoordinates(mean1,mean1W);
    o2->getModelSpacePointInWorldCoordinates(mean2,mean2W);
    o1->getLocalTransform()->TransformVector(dir1,dir1W);
    o2->getLocalTransform()->TransformVector(dir2,dir2W);
    q_vec_scale(dir1W,COLLISION_FORCE*30,dir1W);
    q_vec_scale(dir2W,COLLISION_FORCE*30,dir2W);
    if (affectedGroups.empty() || affectedGroups.contains(o1->getPrimaryCollisionGroupNum())) {
        o1->addForce(mean1W,dir1W);
    }
    if (affectedGroups.empty() || affectedGroups.contains(o2->getPrimaryCollisionGroupNum())) {
        o2->addForce(mean2W,dir2W);
    }
}

//##################################################################################################
static inline void applyCollisionResponseForce(SketchObject *o1, SketchObject *o2,
                                               PQP_CollideResult *cr, QSet<int> &affectedGroups) {
    // get the collision models:
    PQP_Model *pqp_model1 = o1->getModel()->getCollisionModel();
    PQP_Model *pqp_model2 = o2->getModel()->getCollisionModel();

    // get object's orientations
    q_type quat1, quat2;
    (o1)->getOrientation(quat1);
    (o2)->getOrientation(quat2);

    double cForce = COLLISION_FORCE *40 / cr->NumPairs();
    // scale it by the number of colliding primaries so that we have some chance of sliding along surfaces
    // instead of sticking

    // for each pair in collision
    for (int i = 0; i < cr->NumPairs(); i++) {
        int m1Tri = cr->Id1(i);
        int m2Tri = cr->Id2(i);
        // compute the normal vectors...
        q_vec_type n1, n2;
        unit_normal(n1,pqp_model1,m1Tri);
        unit_normal(n2,pqp_model2,m2Tri);
        // compute the forces from those normal vectors
        q_vec_type f1,f2;
        q_xform(f1,quat2,n2);
        q_xform(f2,quat1,n1);
        q_vec_scale(f1,cForce,f1);
        q_vec_scale(f2,cForce,f2);
        // compute the centroids of the triangles as the point to which
        // the forces is applied
        q_vec_type p1,p2;
        centriod(p1,pqp_model1,m1Tri);
        centriod(p2,pqp_model2,m2Tri);
        // apply the forces
        if (affectedGroups.empty() || affectedGroups.contains(o1->getPrimaryCollisionGroupNum())) {
            (o1)->addForce(p1,f1);
        }
        if (affectedGroups.empty() || affectedGroups.contains(o2->getPrimaryCollisionGroupNum())) {
            (o2)->addForce(p2,f2);
        }
    }
}

//##################################################################################################
// -helper function to add spring forces from a list of springs to the objects the springs are
//   attached to.  Does not actually update object position
static inline void springForcesFromList(QList<SpringConnection *> &list, QSet<int> &affectedGroups) {
    for (QListIterator<SpringConnection *> it(list); it.hasNext();) {
        SpringConnection *c = it.next();
        InterObjectSpring *cp = dynamic_cast<InterObjectSpring *>(c);
        c->addForce();
        int i = c->getObject1()->getPrimaryCollisionGroupNum();
        if (i != OBJECT_HAS_NO_GROUP) { // if this one isn't the tracker
            affectedGroups.insert(i);
        }
        if (cp != NULL && cp->getObject2()->getPrimaryCollisionGroupNum() != OBJECT_HAS_NO_GROUP) {
            affectedGroups.insert(cp->getObject2()->getPrimaryCollisionGroupNum());
        }
    }
}

//##################################################################################################
// -helper function that loops over all objects and updates their locations based on current
//   forces on them
static inline void applyEuler(QList<SketchObject *> &list, double dt, bool clearForces = true) {
    int n = list.size();
    for (int i = 0; i < n; i++) {
        SketchObject *obj = list.at(i);
        euler(obj,dt);
        if (clearForces) {
            obj->clearForces();
        }
    }
}

//##################################################################################################
// -helper function that runs the double loop of collision detection over list with only objects
//   from the affected groups selected as the first object in the collision detection.  Does collision
//   response only if the last parameter is set.  Returns true if a collision was found anywhere, false
//   otherwise
// note - if affectedGroups is empty, this does full n^2 collision detection and response over all objects
//         (non pose-mode)

static inline bool collideAndRespond(QList<SketchObject *> &list, QSet<int> &affectedGroups, double dt,
                              bool respond, PhysicsStrategy *strategy) {
    int n = list.size();
    bool foundCollision = false;
    for (int i = 0; i < n; i++) {
        // TODO - self collision once deformation added
        bool needsTest = affectedGroups.empty();
        for (QSetIterator<int> it(affectedGroups); it.hasNext();) {
            if (list.at(i)->isInCollisionGroup(it.next())) {
                needsTest = true;
            }
        }
        if (needsTest) {
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    if (list.at(i)->collide(list.at(j), strategy, respond ? PQP_ALL_CONTACTS : PQP_FIRST_CONTACT)) {
                        foundCollision = true;
                    }
                }
            }
        }
    }
    if (foundCollision && respond) {
        applyEuler(list,dt);
    }
    return foundCollision;
}

//##################################################################################################
// -helper function that does what its name says, applies pose-mode style collision response to the
//   objects.  This means that the objects are tested for collisions, then respond, then are tested again.
//   If the collision response did not fix the collision, then the entire movement (including changes
//   from before this) is undone.
static inline void applyPoseModeCollisionResponse(QList<SketchObject *> &list, QSet<int> &affectedGroups,
                                           double dt, PhysicsStrategy *strategy) {
    int n = list.size();
    bool appliedResponse = collideAndRespond(list,affectedGroups,dt,true,strategy);
    if (appliedResponse) {
        bool stillColliding = collideAndRespond(list,affectedGroups,dt,false,strategy);
        if (stillColliding) {
            // if we couldn't fix collisions, undo the motion and return
            for (int i = 0; i < n; i++) {
                list.at(i)->restoreToLastLocation();
            }
            return;
        }
    }
    // if we fixed the collision or there were none, finalize new positions
    for (int i = 0; i < n; i++) {
        list.at(i)->setLastLocation();
    }
}

//##################################################################################################
// -helper function: divides forces and torques on all objects by divisor and sets the force and torque on the
//   objects to the new amount
static inline void divideForces(QList<SketchObject *> &list,double divisor) {
    double scale = 1/divisor;
    for (QMutableListIterator<SketchObject *> it(list); it.hasNext();) {
        q_vec_type f, t;
        SketchObject *obj = it.next();
        obj->getForce(f);
        obj->getTorque(t);
        q_vec_scale(f,scale,f);
        q_vec_scale(t,scale,t);
        obj->setForceAndTorque(f,t);
    }
}

//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does binary
//   collision search on the force/torque applied
// -assumes that applyEuler has NOT been called on the list, but the forces have been added to each
//   object
static inline void applyBinaryCollisionSearch(QList<SketchObject *> &list, QSet<int> &affectedGroups,
                                       double dt, bool testCollisions, PhysicsStrategy *strategy) {
    applyEuler(list,dt,false);
    if (testCollisions) {
        int times = 1;
        while (collideAndRespond(list,affectedGroups,dt,false,strategy) && times < 10) {
            for (int i = 0; i < list.size(); i++) {
                list.at(i)->restoreToLastLocation();
            }
            divideForces(list,2);
            applyEuler(list,dt,false);
            times++;
        }
    }
    for (int i = 0; i < list.size(); i++) {
        list.at(i)->setLastLocation();
    }
}


//##################################################################################################
static inline void binaryCollisionSearch(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                                  double dt, bool doCollisionCheck, PhysicsStrategy *strategy) {
    QSet<int> affectedGroups;
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyBinaryCollisionSearch(objs,affectedGroups,dt,doCollisionCheck, strategy);
    }
}

//######################################################################################
//######################################################################################
// Simple Physics Strategy - implements basic physics (like my original physics code)
//######################################################################################
//######################################################################################

SimplePhysicsStrategy::SimplePhysicsStrategy() {}

SimplePhysicsStrategy::~SimplePhysicsStrategy() {}


//######################################################################################
void SimplePhysicsStrategy::performPhysicsStepAndCollisionDetection(
        QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
        QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
        QList<SketchObject *> &objects, double dt, bool doCollisionCheck)
{
    QSet<int> affectedGroups;
    springForcesFromList(rHand,affectedGroups);
    springForcesFromList(lHand,affectedGroups);
    if (doPhysicsSprings)
        springForcesFromList(physicsSprings,affectedGroups);
    applyEuler(objects,dt);
    affectedGroups.clear(); // if passed an empty set, it does full collision checks
    if (doCollisionCheck)
        collideAndRespond(objects,affectedGroups,dt,true,this); // calculates collision response
    applyEuler(objects,dt);
}
//######################################################################################
void SimplePhysicsStrategy::respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags)
{
    QSet<int> emptySet;
    applyCollisionResponseForce(o1,o2,cr,emptySet);
}

//######################################################################################
//######################################################################################
// Pose Mode Physics Strategy - implements our first idea of pose mode
//######################################################################################
//######################################################################################

PoseModePhysicsStrategy::PoseModePhysicsStrategy() : affectedGroups() {}

PoseModePhysicsStrategy::~PoseModePhysicsStrategy() {}

//######################################################################################
void PoseModePhysicsStrategy::performPhysicsStepAndCollisionDetection(
        QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
        QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
        QList<SketchObject *> &objects, double dt, bool doCollisionCheck)
{
        // spring forces for right hand interacton
        poseModeForSprings(rHand,objects,dt,doCollisionCheck);
        // spring forces for left hand interaction
        poseModeForSprings(lHand,objects,dt,doCollisionCheck);

        // spring forces for physics springs interaction
        if (doPhysicsSprings) {
            poseModeForSprings(physicsSprings,objects,dt,doCollisionCheck);
        }
}

//######################################################################################
void PoseModePhysicsStrategy::respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags)
{
    if (pqp_flags == PQP_ALL_CONTACTS) {
        applyCollisionResponseForce(o1,o2,cr,affectedGroups);
    }
}
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//   collision response on them.
void PoseModePhysicsStrategy::poseModeForSprings(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                               double dt, bool doCollisionCheck) {
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyEuler(objs,dt);
        if (doCollisionCheck)
            applyPoseModeCollisionResponse(objs,affectedGroups,dt,this);
        affectedGroups.clear();
    }
}

//######################################################################################
//######################################################################################
// Binary Collision Search Physics Strategy - if there is a collision, back off by 1/2 the force/torque
//######################################################################################
//######################################################################################

BinaryCollisionSearchStrategy::BinaryCollisionSearchStrategy() {}

BinaryCollisionSearchStrategy::~BinaryCollisionSearchStrategy() {}

//######################################################################################
void BinaryCollisionSearchStrategy::performPhysicsStepAndCollisionDetection(
        QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
        QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
        QList<SketchObject *> &objects, double dt, bool doCollisionCheck)
{
    binaryCollisionSearch(rHand,objects,dt,doCollisionCheck,this);
    binaryCollisionSearch(lHand,objects,dt,doCollisionCheck,this);
    if (doPhysicsSprings) {
        binaryCollisionSearch(physicsSprings,objects,dt,doCollisionCheck,this);
    }
}

//######################################################################################
void BinaryCollisionSearchStrategy::respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags)
{
}


//######################################################################################
//######################################################################################
// Pose Mode PCA Physics Strategy - implements pose mode, but with principal component
// analysis collision response
//######################################################################################
//######################################################################################

PoseModePCAPhysicsStrategy::PoseModePCAPhysicsStrategy() : affectedGroups() {}

PoseModePCAPhysicsStrategy::~PoseModePCAPhysicsStrategy() {}

//######################################################################################
void PoseModePCAPhysicsStrategy::performPhysicsStepAndCollisionDetection(
        QList<SpringConnection *> &lHand, QList<SpringConnection *> &rHand,
        QList<SpringConnection *> &physicsSprings, bool doPhysicsSprings,
        QList<SketchObject *> &objects, double dt, bool doCollisionCheck)
{
        // spring forces for right hand interacton
        poseModePCAForSprings(rHand,objects,dt,doCollisionCheck);
        // spring forces for left hand interaction
        poseModePCAForSprings(lHand,objects,dt,doCollisionCheck);

        // spring forces for physics springs interaction
        if (doPhysicsSprings) {
            poseModePCAForSprings(physicsSprings,objects,dt,doCollisionCheck);
        }
}

//######################################################################################
void PoseModePCAPhysicsStrategy::respondToCollision(SketchObject *o1, SketchObject *o2, PQP_CollideResult *cr, int pqp_flags)
{
    if (pqp_flags == PQP_ALL_CONTACTS) {
        applyPCACollisionResponseForce(o1,o2,cr,affectedGroups);
    }
}
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//   collision response on them.
void PoseModePCAPhysicsStrategy::poseModePCAForSprings(QList<SpringConnection *> &springs, QList<SketchObject *> &objs,
                               double dt, bool doCollisionCheck) {
    if (!springs.empty()) {
        springForcesFromList(springs,affectedGroups);
        applyEuler(objs,dt);
        if (doCollisionCheck)
            applyPoseModeCollisionResponse(objs,affectedGroups,dt,this);
        affectedGroups.clear();
    }
}
