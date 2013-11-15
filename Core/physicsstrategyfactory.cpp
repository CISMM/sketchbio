#include "physicsstrategyfactory.h"

#include <QSet>

#include <vtkTransform.h>

#include <PQP.h>

#include "sketchmodel.h"
#include "sketchobject.h"
#include "sketchioconstants.h"
class Connector;
#include "physicsstrategy.h"
#include "physicsutilities.h"

/*
 * These classes have definitions further down in the file, below
 * the only publicly visible method
 */
namespace PhysicsStrategyFactory
{

/*
 * This class implements the original physics mode where all objects are subject to response forces and all
 * forces are applied at once.
 */
class SimplePhysicsStrategy : public PhysicsStrategy
{
public:
    SimplePhysicsStrategy();
    virtual ~SimplePhysicsStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList< Connector* >& lHand, QList< Connector* >& rHand,
                                                 QList< Connector* >& physicsSprings, bool doPhysicsSprings,
                                                 QList< SketchObject* >& objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags);
};

/*
 * This class implements my first try at pose mode physics.  Only those objects which moved during the
 * application of the springs are subject to collision response, the others are fixed.  In addition, the
 * springs are applied separately from different sources, first right hand, then left, then world physics.
 *
 * Note: this class uses internal state to keep track of which objects are affected by springs, so it is not
 * safe to call from multiple threads
 */
class PoseModePhysicsStrategy : public PhysicsStrategy
{
public:
    PoseModePhysicsStrategy();
    virtual ~PoseModePhysicsStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList< Connector* >& lHand, QList< Connector* >& rHand,
                                                 QList< Connector* >& physicsSprings, bool doPhysicsSprings,
                                                 QList< SketchObject* >& objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags);
private:
    void poseModeForSprings(QList< Connector* >& springs, QList<SketchObject* >& objs,
                               double dt, bool doCollisionCheck);
    QSet<int> affectedGroups;
};

/*
 * This class implements the binary collision search method of collision detection and response where collisions are
 * never allowed and the motion is always undone if it results in a collision. This strategy is like PoseMode in that
 * forces from different sources are applied or rejected separately.
 */
class BinaryCollisionSearchStrategy : public PhysicsStrategy
{
public:
    BinaryCollisionSearchStrategy();
    virtual ~BinaryCollisionSearchStrategy();

    virtual
    void performPhysicsStepAndCollisionDetection(QList< Connector* >& lHand, QList< Connector* >& rHand,
                                                 QList< Connector* >& physicsSprings, bool doPhysicsSprings,
                                                 QList< SketchObject* >& objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags);
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
    void performPhysicsStepAndCollisionDetection(QList< Connector* >& lHand, QList< Connector* >& rHand,
                                                 QList< Connector* >& physicsSprings, bool doPhysicsSprings,
                                                 QList< SketchObject* >& objects, double dt, bool doCollisionCheck);
    virtual
    void respondToCollision(SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags);
private:
    void poseModePCAForSprings(QList< Connector* >& springs, QList<SketchObject* >& objs,
                               double dt, bool doCollisionCheck);
    QSet<int> affectedGroups;
};

/*
 * Creates the PhysicsStrategies
 */
void populateStrategies(QVector<QSharedPointer<PhysicsStrategy> > &strategies) {
    strategies.clear();
    QSharedPointer<PhysicsStrategy> s1(new SimplePhysicsStrategy());
    strategies.append(s1);
    QSharedPointer<PhysicsStrategy> s2(new PoseModePhysicsStrategy());
    strategies.append(s2);
    QSharedPointer<PhysicsStrategy> s3(new BinaryCollisionSearchStrategy());
    strategies.append(s3);
    QSharedPointer<PhysicsStrategy> s4(new PoseModePCAPhysicsStrategy());
    strategies.append(s4);
}

/*
 * Implementations of the strategies defined above (starts with helper functions)
 */

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
// helper function-- compute the normal n of triangle tri in the model
// assumes points in each triangle in the model are added in counterclockwise order
// relative to the outside of the surface looking down
static inline void unit_normal(q_vec_type n, PQP_Model* m, int t) {
    q_vec_type diff1, diff2;
#ifdef PQP_UPDATE_EPSILON
    Tri* tri = &(m->tris[m->idsToIndices[t]]);
#else
    Tri* tri = NULL;
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
static inline void centriod(q_vec_type c, PQP_Model* m, int t) {
#ifdef PQP_UPDATE_EPSILON
    Tri* tri = &(m->tris[m->idsToIndices[t]]);
#else
    Tri* tri = NULL;
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
static inline void computeMeanCollisionPoint(PQP_REAL mean[3], PQP_CollideResult* cr,
                                      SketchModel* model1, bool isFirst,
                                      PQP_Model* m1) {
    int total = 3 * cr->NumPairs();
    mean[0] = mean[1] = mean[2] = 0.0;

    for (int i = 0; i < cr->NumPairs(); i++) {
        int m1Tri;
        if (isFirst)
            m1Tri= cr->Id1(i);
        else
            m1Tri = cr->Id2(i);
        int triId1 = m1Tri;
        /*
#ifndef PQP_UPDATE_EPSILON
        triId1 = model1->getTriIdToTriIndexHash()->value(m1Tri);
#else
        triId1 = m1->idsToIndices(m1Tri);
#endif  */
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
static inline void computeCollisonPointCovariance(PQP_REAL mean[3], PQP_CollideResult* cr,
                                           SketchModel* model1, bool isFirst,
                                           PQP_Model* m1, PQP_REAL cov[3][3]) {
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
                int triId1 = m1Tri;
                /*
#ifndef PQP_UPDATE_EPSILON
                triId1 = model1->getTriIdToTriIndexHash()->value(m1Tri);
#else
                triId1 = m1->idsToIndices(m1Tri);
#endif          */
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
// this method takes the objects in collision and the affected groups and
// computes which level of the heirarchy to add the force to.  Note that
// the result1 and result2 parameters are references to pointers.  These
// will be set to the objects to add the forces to or NULL if no force
// should be added on that side
//
// this function also handles the case where affectedGroups is empty by
// picking the highest level that is not in the other hierarchy as the level
// to add force at
static inline void computeObjectsToAddForce(SketchObject* o1, SketchObject* o2,
                                            QSet< int > affectedGroups,
                                            SketchObject* & result1,
                                            SketchObject* & result2)
{
    QList< SketchObject* > parents1, parents2;
    SketchObject* p = o1;
    while (p != NULL)
    {
        parents1.append(p);
        p = p->getParent();
    }
    p = o2;
    while (p != NULL)
    {
        parents2.append(p);
        p = p->getParent();
    }
    int idx1 = 0, idx2 = 0;
    if (!affectedGroups.empty())
    {
        while (idx1 < parents1.size() &&
               !affectedGroups.contains(
                   parents1[idx1]->getPrimaryCollisionGroupNum()))
        {
            idx1++;
        }
        if (idx1 >= parents1.size() || parents2.contains(parents1[idx1]))
        {
            idx1 = -1;
        }
        while (idx2 < parents2.size() &&
               !affectedGroups.contains(
                   parents2[idx2]->getPrimaryCollisionGroupNum()))
        {
            idx2++;
        }
        if (idx2 >= parents2.size() || parents1.contains(parents2[idx2]))
        {
            idx2 = -1;
        }
    }
    else
    {
        while (idx1 < parents1.size() &&
               !parents2.contains(parents1[idx1]))
        {
            idx1++;
        }
        if (idx1 >= parents1.size())
        {
            idx1 = parents1.size()-1;
        }
        while (idx2 < parents2.size() &&
               !parents2.contains(parents2[idx2]))
        {
            idx2++;
        }
        if (idx2 >= parents2.size())
        {
            idx2 = parents2.size()-1;
        }
    }
    if (idx1 == -1)
    {
        result1 = NULL;
    }
    else
    {
        result1 = parents1[idx1];
    }
    if (idx2 == -1)
    {
        result2 = NULL;
    }
    else
    {
        result2 = parents2[idx2];
    }
}

//##################################################################################################
static inline void applyPCACollisionResponseForce(SketchObject* o1, SketchObject* o2,
                                               PQP_CollideResult* cr, QSet< int >& affectedGroups) {
    // get the collision models:
    SketchModel* model1 = o1->getModel();
    SketchModel* model2 = o2->getModel();
    PQP_Model* m1 = model1->getCollisionModel(o1->getModelConformation());
    PQP_Model* m2 = model2->getCollisionModel(o2->getModelConformation());

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
    q_vec_type p1, p2;
    o1->getLocalTransform()->TransformVector(dir1,dir1W);
    o2->getLocalTransform()->TransformVector(dir2,dir2W);
    q_vec_scale(dir1W,COLLISION_FORCE*30,dir1W);
    q_vec_scale(dir2W,COLLISION_FORCE*30,dir2W);
    SketchObject* obj1 = NULL, * obj2 = NULL;
    computeObjectsToAddForce(o1,o2,affectedGroups,obj1,obj2);
    if (obj1 != NULL)
    {
        o1->getModelSpacePointInWorldCoordinates(p1,mean1);
        obj1->getWorldSpacePointInModelCoordinates(p1,p1);
        obj1->addForce(p1,dir1W);
    }
    if (obj2 != NULL)
    {
        o2->getModelSpacePointInWorldCoordinates(p2,mean2);
        obj2->getWorldSpacePointInModelCoordinates(p2,p2);
        obj2->addForce(p2,dir2W);
    }
}

//##################################################################################################
static inline void applyCollisionResponseForce(SketchObject* o1, SketchObject* o2,
                                               PQP_CollideResult* cr, QSet< int >& affectedGroups) {
    // get the collision models:
    PQP_Model* pqp_model1 = o1->getModel()->getCollisionModel(o1->getModelConformation());
    PQP_Model* pqp_model2 = o2->getModel()->getCollisionModel(o2->getModelConformation());

    // get object's orientations
    q_type quat1, quat2;
    (o1)->getOrientation(quat1);
    (o2)->getOrientation(quat2);

    double cForce = COLLISION_FORCE *40 / cr->NumPairs();
    // scale it by the number of colliding primaries so that we have some chance of sliding along surfaces
    // instead of sticking

    SketchObject* obj1 = NULL, * obj2 = NULL;
    computeObjectsToAddForce(o1,o2,affectedGroups,obj1,obj2);

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
        if (obj1 != NULL)
        {
            o1->getModelSpacePointInWorldCoordinates(p1,p1);
            obj1->getWorldSpacePointInModelCoordinates(p1,p1);
            obj1->addForce(p1,f1);
        }
        if (obj2 != NULL)
        {
            o2->getModelSpacePointInWorldCoordinates(p2,p2);
            obj2->getWorldSpacePointInModelCoordinates(p2,p2);
            obj2->addForce(p2,f2);
        }
    }
}


//##################################################################################################
// -helper function that does what its name says, applies pose-mode style collision response to the
//   objects.  This means that the objects are tested for collisions, then respond, then are tested again.
//   If the collision response did not fix the collision, then the entire movement (including changes
//   from before this) is undone.
static inline void applyPoseModeCollisionResponse(QList< SketchObject* >& list,
                                                  QSet< int > &affectedCollisionGroups,
                                                  QSet< SketchObject* >& affectedGroups,
                                           double dt, PhysicsStrategy* strategy) {
    bool appliedResponse = PhysicsUtilities::collideAndComputeResponse(
                list,affectedCollisionGroups,true,strategy)
            || PhysicsUtilities::collideWithinGroupAndComputeResponse(
                affectedGroups,affectedCollisionGroups,true,strategy);
    PhysicsUtilities::applyEulerToListAndGroups(list,affectedGroups,dt,true);
    if (appliedResponse) {
        bool stillColliding = PhysicsUtilities::collideAndComputeResponse(
                    list,affectedCollisionGroups,false,strategy)
                || PhysicsUtilities::collideWithinGroupAndComputeResponse(
                    affectedGroups,affectedCollisionGroups,false,strategy);
        if (stillColliding) {
            // if we couldn't fix collisions, undo the motion and return
            PhysicsUtilities::restoreToLastLocation(list,affectedGroups);
            return;
        }
    }
}

//##################################################################################################
// -helper function: divides forces and torques on all objects by divisor and sets the force and torque on the
//   objects to the new amount
static inline void divideForces(QList< SketchObject* >& list,
                                QSet< SketchObject* >& affectedGroups,
                                double divisor)
{
    double scale = 1/divisor;
    for (QMutableListIterator< SketchObject* > it(list); it.hasNext();) {
        q_vec_type f, t;
        SketchObject* obj = it.next();
        obj->getForce(f);
        obj->getTorque(t);
        q_vec_scale(f,scale,f);
        q_vec_scale(t,scale,t);
        obj->setForceAndTorque(f,t);
        if (affectedGroups.contains(obj))
        {
            divideForces(*obj->getSubObjects(),affectedGroups,divisor);
        }
    }
}

//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does binary
//   collision search on the force/torque applied
// -assumes that applyEuler has NOT been called on the list, but the forces have been added to each
//   object
static inline void applyBinaryCollisionSearch(QList< SketchObject* >& list,
                                              QSet< int > &affectedCollisionGroups,
                                              QSet< SketchObject* >& affectedGroups,
                                              double dt,
                                              bool testCollisions,
                                              PhysicsStrategy* strategy)
{
    PhysicsUtilities::setLastLocation(list,affectedGroups);
    PhysicsUtilities::applyEulerToListAndGroups(list,affectedGroups,dt,false);
    if (testCollisions) {
        int times = 1;
        while ((PhysicsUtilities::collideAndComputeResponse(
                   list,affectedCollisionGroups,false,strategy)
                || PhysicsUtilities::collideWithinGroupAndComputeResponse(
                    affectedGroups,affectedCollisionGroups,false,strategy))
               && times < 10)
        {
            PhysicsUtilities::restoreToLastLocation(list,affectedGroups);
            divideForces(list,affectedGroups,2);
            PhysicsUtilities::applyEulerToListAndGroups(list,affectedGroups,dt,false);
            times++;
        }
    }
}


//##################################################################################################
static inline void binaryCollisionSearch(QList< Connector* >& springs,
                                         QList< SketchObject* >& objs,
                                         double dt,
                                         bool doCollisionCheck,
                                         PhysicsStrategy* strategy)
{
    QSet< int > affectedCollisionGroups;
    QSet< SketchObject* > affectedGroups;
    if (!springs.empty()) {
        PhysicsUtilities::springForcesFromList(
                    springs,affectedCollisionGroups,affectedGroups);
        applyBinaryCollisionSearch(objs,affectedCollisionGroups,affectedGroups,
                                   dt,doCollisionCheck, strategy);
        PhysicsUtilities::clearForces(objs,affectedGroups);
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
        QList< Connector* >& lHand, QList< Connector* >& rHand,
        QList< Connector* >& physicsSprings, bool doPhysicsSprings,
        QList< SketchObject* >& objects, double dt, bool doCollisionCheck)
{
    QSet< int > affectedCollisionGroups;
    QSet< SketchObject * > affectedGroups;
    PhysicsUtilities::springForcesFromList(rHand,affectedCollisionGroups,affectedGroups);
    PhysicsUtilities::springForcesFromList(lHand,affectedCollisionGroups,affectedGroups);
    if (doPhysicsSprings)
    {
        PhysicsUtilities::springForcesFromList(
                    physicsSprings,affectedCollisionGroups,affectedGroups);
    }
    PhysicsUtilities::applyEulerToListAndGroups(objects,affectedGroups,dt,true);
    affectedCollisionGroups.clear(); // if passed an empty set, it does full collision checks
    if (doCollisionCheck)
    {
        PhysicsUtilities::collideAndComputeResponse(
                    objects,affectedCollisionGroups,true,this);
        PhysicsUtilities::collideWithinGroupAndComputeResponse(
                    affectedGroups,affectedCollisionGroups,true,this);
        PhysicsUtilities::applyEulerToListAndGroups(objects,affectedGroups,dt,true);
    }
}
//######################################################################################
void SimplePhysicsStrategy::respondToCollision(
        SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags)
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
        QList< Connector* >& lHand, QList< Connector* >& rHand,
        QList< Connector* >& physicsSprings, bool doPhysicsSprings,
        QList< SketchObject* >& objects, double dt, bool doCollisionCheck)
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
void PoseModePhysicsStrategy::respondToCollision(
        SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags)
{
    if (pqp_flags == PQP_ALL_CONTACTS) {
        applyCollisionResponseForce(o1,o2,cr,affectedGroups);
    }
}
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//   collision response on them.
void PoseModePhysicsStrategy::poseModeForSprings(QList< Connector* >& springs,
                                                 QList< SketchObject* >& objs,
                                                 double dt,
                                                 bool doCollisionCheck)
{
    QSet< SketchObject* > affectedObjectGroups;
    if (!springs.empty()) {
        if (PhysicsUtilities::springForcesFromList(
                    springs,affectedGroups,affectedObjectGroups))
        {
            PhysicsUtilities::setLastLocation(objs,affectedObjectGroups);
            PhysicsUtilities::applyEulerToListAndGroups(
                        objs,affectedObjectGroups,dt,true);
            if (doCollisionCheck)
                applyPoseModeCollisionResponse(objs,affectedGroups,
                                               affectedObjectGroups,dt,this);
            affectedGroups.clear();
        }
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
        QList< Connector* >& lHand, QList< Connector* >& rHand,
        QList< Connector* >& physicsSprings, bool doPhysicsSprings,
        QList< SketchObject* >& objects, double dt, bool doCollisionCheck)
{
    binaryCollisionSearch(rHand,objects,dt,doCollisionCheck,this);
    binaryCollisionSearch(lHand,objects,dt,doCollisionCheck,this);
    if (doPhysicsSprings) {
        binaryCollisionSearch(physicsSprings,objects,dt,doCollisionCheck,this);
    }
}

//######################################################################################
void BinaryCollisionSearchStrategy::respondToCollision(
        SketchObject* o1, SketchObject* o2, PQP_CollideResult* cr, int pqp_flags)
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
        QList< Connector* >& lHand, QList< Connector* >& rHand,
        QList< Connector* >& physicsSprings, bool doPhysicsSprings,
        QList< SketchObject* >& objects, double dt, bool doCollisionCheck)
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
void PoseModePCAPhysicsStrategy::respondToCollision(SketchObject* o1, SketchObject* o2,
                                                    PQP_CollideResult* cr, int pqp_flags)
{
    if (pqp_flags == PQP_ALL_CONTACTS) {
        applyPCACollisionResponseForce(o1,o2,cr,affectedGroups);
    }
}
//##################################################################################################
// -helper function applies the forces from the list of springs to the objects and does pose-mode
//   collision response on them.
void PoseModePCAPhysicsStrategy::poseModePCAForSprings(QList< Connector* >& springs,
                                                       QList< SketchObject* >& objs,
                                                       double dt,
                                                       bool doCollisionCheck)
{
    QSet< SketchObject *> affectedObjectGroups;
    if (!springs.empty()) {
        if (PhysicsUtilities::springForcesFromList(
                    springs,affectedGroups,affectedObjectGroups))
        {
            PhysicsUtilities::setLastLocation(objs,affectedObjectGroups);
            PhysicsUtilities::applyEulerToListAndGroups(
                        objs,affectedObjectGroups,dt,true);
            if (doCollisionCheck)
                applyPoseModeCollisionResponse(objs,affectedGroups,affectedObjectGroups,
                                               dt,this);
            affectedGroups.clear();
        }
    }
}
}
