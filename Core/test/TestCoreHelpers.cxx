#include "TestCoreHelpers.h"

#include <iostream>

#include <QString>

#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

#include <sketchtests.h>
#include <sketchioconstants.h>
#include <transformmanager.h>
#include <modelutilities.h>
#include <sketchmodel.h>
#include <keyframe.h>
#include <sketchobject.h>
#include <objectgroup.h>

namespace TestCoreHelpers
{
//###############################################################################
//###############################################################################
SketchModel *getCubeModel()
{
    vtkSmartPointer< vtkCubeSource > cube =
        vtkSmartPointer< vtkCubeSource >::New();
    cube->SetBounds(-1,1,-1,1,-1,1);
    cube->Update();
    QString fname = ModelUtilities::createFileFromVTKSource(cube, "cube_test_model");
    SketchModel *model = new SketchModel(DEFAULT_INVERSE_MASS,
                                         DEFAULT_INVERSE_MOMENT);
    model->addConformation(fname, fname);
    return model;
}

//###############################################################################
//###############################################################################
SketchModel *getSphereModel()
{
    vtkSmartPointer< vtkSphereSource > source =
            vtkSmartPointer< vtkSphereSource >::New();
    source->SetRadius(4);
    source->SetThetaResolution(40);
    source->SetPhiResolution(40);
    source->Update();
    vtkSmartPointer< vtkTransformPolyDataFilter > sourceData =
            vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    vtkSmartPointer< vtkTransform > transform =
            vtkSmartPointer< vtkTransform >::New();
    transform->Identity();
    sourceData->SetInputConnection(source->GetOutputPort());
    sourceData->SetTransform(transform);
    sourceData->Update();
    QString fileName = ModelUtilities::createFileFromVTKSource(
                sourceData,"tests_sphere_model");
    SketchModel *model = new SketchModel(1.0,1.0);
    model->addConformation(fileName,fileName);
    return model;
}
//#########################################################################
//#########################################################################
void setTrackerWorldPosition(TransformManager &t, SketchBioHandId::Type side,
                             const q_vec_type pos)
{
    vtkSmartPointer< vtkMatrix4x4 > identity =
            vtkSmartPointer< vtkMatrix4x4 >::New();
    identity->Identity();
    t.setTrackerToRoomMatrix(identity);
    t.setWorldToRoomMatrix(identity);
    q_xyz_quat_type location;
    q_vec_copy(location.xyz,pos);
    q_make(location.quat,1,0,0,0);
    t.setHandTransform(&location,side);
}

//#########################################################################
//#########################################################################
#define PRINT_ERROR(M) std::cout << M << std::endl
#define ObjectTestNewMacro(TYPE) static ObjectTest* New(void*) { return new TYPE; }
#define ObjectTestNameMacro(NAME) virtual const char* getTestName() { return NAME; }

static int test_assert_true(bool condition, const char* error_msg)
{
    if (!condition)
    {
        PRINT_ERROR(error_msg);
        return 1;
    }
    return 0;
}

static inline int vectors_should_be_equal(const q_vec_type v1,
                                          const q_vec_type v2,
                                          const char* error_to_print)
{
    return test_assert_true(q_vec_equals(v1,v2),error_to_print);
}

static inline int quats_should_be_equal(const q_type q1,
                                        const q_type q2,
                                        const char* error_to_print)
{
    return test_assert_true(q_equals(q1,q2),error_to_print);
}

class PositionTest : public ObjectTest
{
public:
    virtual ~PositionTest() {}
    ObjectTestNameMacro("Position")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type place = {4004,5.827,Q_PI};
        q_vec_type oldPlace,oldLPlace;
        const q_vec_type nullVector = Q_NULL_VECTOR, oneVector = {1,1,1};
        q_vec_type tmp1, tmp2;
        obj->getPosition(oldPlace);
        obj->getLastPosition(oldLPlace);
        if (q_vec_equals(place,oldPlace) || q_vec_equals(place,oldLPlace))
        {
            PRINT_ERROR("Bad starting object" << __FILE__ << ":" << __LINE__);
            return 1; // if starting object invalid, just exit
        }
        obj->setPosition(place);
        obj->getPosition(tmp1);
        errors += vectors_should_be_equal(place,tmp1,"Get/set position failed");

        obj->getLastPosition(tmp2);
        errors += vectors_should_be_equal(
                    oldLPlace,tmp2,
                    "Last location changed when position updated.");
        obj->getLocalTransform()->GetPosition(tmp1);
        errors += vectors_should_be_equal(
                    place,tmp1,"Set position incorrectly updated transform position");
        obj->getLocalTransform()->GetOrientation(tmp2);
        errors += vectors_should_be_equal(
                    nullVector,tmp2,
                    "Set position incorrectly updated transform rotation.");
        obj->getLocalTransform()->GetScale(tmp2);
        errors += vectors_should_be_equal(
                    oneVector,tmp2,
                    "Set position incorrectly updated transform scale.");

        return errors;
    }

    ObjectTestNewMacro(PositionTest)
};

class OrientationTest : public ObjectTest
{
public:
    virtual ~OrientationTest() {}
    ObjectTestNameMacro("Orientation")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_type orient;
        q_type oldOrient, oldLOrient;
        const q_vec_type oneVec = {1,1,1};
        q_vec_type pos, tmpV;
        q_type tmp1, tmp2;
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        obj->getOrientation(oldOrient);
        obj->getLastOrientation(oldLOrient);
        obj->getPosition(pos);
        if (q_equals(orient,oldOrient) || q_equals(orient,oldLOrient))
        {
            PRINT_ERROR("Bad starting object" << __FILE__ << ":" << __LINE__);
            return 1;
        }
        obj->setOrientation(orient);
        obj->getOrientation(tmp1);
        errors += quats_should_be_equal(
                    orient,tmp1,"Get/set orientation failed.");
        obj->getLastOrientation(tmp2);
        errors += quats_should_be_equal(
                    oldLOrient,tmp2,"Set orientation affected last orientation.");
        obj->getLocalTransform()->GetPosition(tmpV);
        errors += vectors_should_be_equal(
                    pos,tmpV,"Set orientation updated transform position.");
        double *wxyz = obj->getLocalTransform()->GetOrientationWXYZ();
        q_from_axis_angle(tmp2,wxyz[1],wxyz[2],wxyz[3],(Q_PI / 180) * wxyz[0]);
        errors += quats_should_be_equal(
                    orient,tmp2,"Set orientation incorrectly updated transform position.");
        obj->getLocalTransform()->GetScale(tmpV);
        errors += vectors_should_be_equal(
                    tmpV,oneVec,"Set orientation updated transform scale.");
        return errors;
    }

    ObjectTestNewMacro(OrientationTest)
};

class SaveAndRestoreTest: public ObjectTest
{
public:
    virtual ~SaveAndRestoreTest() {}
    ObjectTestNameMacro("SaveAndRestore")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type oldLPos, lPos, nPos, tmpV;
        q_type oldLOrient, lOrient, nOrient, tmpQ;
        q_vec_set(lPos,4004,5.827,Q_PI);
        q_vec_set(nPos,5,9332.222,.1*Q_PI);
        q_make(lOrient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(lOrient,lOrient);
        q_make(nOrient,0,-1,0,-0.45);
        obj->getLastPosition(oldLPos);
        obj->getLastOrientation(oldLOrient);
        if (q_vec_equals(oldLPos,lPos) || q_equals(oldLOrient,lOrient))
        {
            PRINT_ERROR("Bad starting object" << __FILE__ << ":" << __LINE__);
            return 1;
        }
        obj->setPosAndOrient(lPos,lOrient);

        // test setLastLocation
        obj->setLastLocation();
        obj->getLastPosition(tmpV);
        errors += vectors_should_be_equal(
                    lPos,tmpV,
                    "setPosAndOrient or setLastLocation failed for position.");
        obj->getLastOrientation(tmpQ);
        errors += quats_should_be_equal(
                    lOrient,tmpQ,
                    "setPosAndOrient or setLastLocation failed for orientation.");

        // test setting a new position afterwards
        obj->setPosAndOrient(nPos,nOrient);
        obj->getPosition(tmpV);
        errors += vectors_should_be_equal(
                    nPos,tmpV,"setPosAndOrient failed for position.");
        obj->getOrientation(tmpQ);
        errors += quats_should_be_equal(
                    nOrient,tmpQ,"setPosAndOrient failed for orientation.");
        obj->getLastPosition(tmpV);
        errors += vectors_should_be_equal(
                    lPos,tmpV,
                    "setPosAndOrient overwrote last position");
        obj->getLastOrientation(tmpQ);
        errors += quats_should_be_equal(
                    lOrient,tmpQ,
                    "setPosAndOrient overwrote last orientation.");

        // test restoring to the last location
        obj->restoreToLastLocation();
        obj->getPosition(tmpV);
        errors += vectors_should_be_equal(
                    lPos,tmpV,
                    "restoreToLastLocation failed for position");
        obj->getOrientation(tmpQ);
        errors += quats_should_be_equal(
                    lOrient,tmpQ,
                    "restoreToLastLocation failed for orientation");
        return errors;
    }

    ObjectTestNewMacro(SaveAndRestoreTest)
};


class TestCollisionGroups : public ObjectTest
{
public:
    virtual ~TestCollisionGroups() {}
    ObjectTestNameMacro("CollisionGroups")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        int cGroup = obj->getPrimaryCollisionGroupNum();
        int newCGroup = 45;
        int otherCGroup = 397;
        obj->setPrimaryCollisionGroupNum(newCGroup);
        obj->addToCollisionGroup(otherCGroup);
        if (cGroup != OBJECT_HAS_NO_GROUP)
        {
            errors += test_assert_true(obj->isInCollisionGroup(cGroup),
                                       "Object was removed from collision"
                                       " group by setting primary collision group"
                                       " or adding it to a collision group.");
        }
        errors += test_assert_true(obj->isInCollisionGroup(newCGroup),
                                   "Object was not added to collision "
                                   "group by setPrimaryCollisionGroup");
        errors += test_assert_true(obj->isInCollisionGroup(otherCGroup),
                                   "Object was not added to collision "
                                   "group by addToCollisionGroup");
        errors += test_assert_true(
                    obj->getPrimaryCollisionGroupNum() == newCGroup,
                    "setPrimaryCollisionGroup did not set the primary group.");
        return errors;
    }

    ObjectTestNewMacro(TestCollisionGroups)
};

class PointTransformsTest : public ObjectTest
{
public:
    virtual ~PointTransformsTest() {}
    ObjectTestNameMacro("PointTransforms")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type pos, point, tmp1, tmp2, nullVector = Q_NULL_VECTOR;
        q_type orient, tmpQ;
        q_vec_set(pos,4004,5.827,Q_PI);
        q_vec_set(point,5,9332.222,.1*Q_PI);
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        obj->setPosAndOrient(pos,orient);
        obj->getModelSpacePointInWorldCoordinates(nullVector,tmp1);
        errors += vectors_should_be_equal(
                    pos,tmp1,"Model point in world coords failed on null vector");
        obj->getModelSpacePointInWorldCoordinates(point,tmp1);
        q_vec_subtract(tmp2,tmp1,pos);
        q_invert(tmpQ,orient);
        q_xform(tmp2,tmpQ,tmp2);
        errors += vectors_should_be_equal(
                    point,tmp2,
                    "Model point in world coords failed on interesting point.");
        obj->getWorldVectorInModelSpace(point,tmp1);
        q_xform(tmp2,orient,tmp1);
        errors += vectors_should_be_equal(
                    point,tmp2,"World vector to modelspace failed.");
        return errors;
    }
    ObjectTestNewMacro(PointTransformsTest)
};

class ForcesTest : public ObjectTest
{
public:
    virtual ~ForcesTest() {}
    ObjectTestNameMacro("Forces")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type pos, point, tmp1, tmp2, nullVector = Q_NULL_VECTOR;
        q_type orient;
        q_vec_set(pos,4004,5.827,Q_PI);
        q_vec_set(point,5,9332.222,.1*Q_PI);
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        obj->setPosAndOrient(pos,orient);
        q_vec_type f1 = {3,4,5}, f2 = {7,-2,-6.37};
        obj->addForce(nullVector,f1);
        obj->getForce(tmp1);
        errors += vectors_should_be_equal(tmp1,f1,"Applying linear force failed");
        obj->getTorque(tmp2);
        errors += vectors_should_be_equal(
                    tmp2,nullVector,"Applying force at center caused torque");
        obj->addForce(point,f2);
        obj->getForce(tmp1);
        q_vec_add(tmp2,f1,f2);
        errors += vectors_should_be_equal(
                    tmp1,tmp2,"Combining forces failed");
        obj->getTorque(tmp2);
        obj->getWorldVectorInModelSpace(f2,tmp1);
        q_vec_cross_product(tmp1,point,tmp1);
        errors += vectors_should_be_equal(
                    tmp1,tmp2,"Torque calculations failed");
        obj->clearForces();
        obj->getForce(tmp1);
        errors += vectors_should_be_equal(tmp1,nullVector,"Clearing forces failed.");
        obj->getTorque(tmp2);
        errors += vectors_should_be_equal(tmp2,nullVector,"Clearning torque failed.");
        return errors;
    }
    ObjectTestNewMacro(ForcesTest)
};

static inline int testKeyframe(const Keyframe& kf,
                               const q_vec_type pos,
                               const q_type orient,
                               bool visible,
                               bool active)
{
    int errors = 0;
    q_vec_type p;
    q_type o;
    kf.getPosition(p);
    kf.getOrientation(o);
    errors += vectors_should_be_equal(p,pos,"Keyframe position is wrong");
    errors += quats_should_be_equal(o,orient,"Keyframe orientation is wrong");
    errors += test_assert_true(active == kf.isActive(),
                               "Keyframe active status is wrong");
    errors += test_assert_true(visible == kf.isVisibleAfter(),
                               "Keyframe visibility is wrong");
    return errors;
}

static inline int testNumKeyframes(SketchObject *obj, int expected)
{
    int errors = test_assert_true((expected == 0) ^ obj->hasKeyframes(),
                                  "Object has keyframes status wrong");
    errors += test_assert_true(obj->getNumKeyframes() == expected,
                               "Object has wrong number of keyframes");
    return errors;
}

class KeyframeColorTest
{
public:
    virtual void setKeyframe1Color(SketchObject* obj) {}
    virtual void setKeyframe2Color(SketchObject* obj) {}
    virtual int testKeyframeColor(int frameNum, const Keyframe& f) { return 0; }
    virtual int testObjectColor(int frameNum, SketchObject* obj) { return 0; }
    virtual int testChangedSinceKeyframe(double kfTime, SketchObject* obj) { return 0; }
};

#define ObjectKeyframeTestStateAndCtor(NAME) \
private:\
    QScopedPointer< KeyframeColorTest > colors;\
public:\
    NAME(KeyframeColorTest* color) : colors(color) {}

#define ObjectKeyframeTestNewMacro(NAME) static ObjectTest* New(void* arg) { \
    KeyframeColorTest* color = (KeyframeColorTest*)arg;\
    if (arg == NULL) { color = new KeyframeColorTest(); }\
    return new NAME(color);\
}

class KeyframeCreationTest : public ObjectTest
{
    ObjectKeyframeTestStateAndCtor(KeyframeCreationTest)
public:
    virtual ~KeyframeCreationTest() {}
    ObjectTestNameMacro("KeyframeCreation")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type pos, pos2;
        q_type orient, orient2;
        q_vec_set(pos,4004,5.827,Q_PI);
        q_vec_set(pos2,5,9332.222,.1*Q_PI);
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        q_from_axis_angle(orient2,0,-1,0,-.45);
        errors += testNumKeyframes(obj,0);
        obj->setPosAndOrient(pos,orient);
        obj->setIsVisible(true);
        obj->setActive(false);
        colors->setKeyframe1Color(obj);
        obj->addKeyframeForCurrentLocation(3.0);
        errors += testNumKeyframes(obj,1);
        errors += testKeyframe(obj->getKeyframes()->value(3.0),pos,orient,
                               true,false);
        errors += colors->testKeyframeColor(1,obj->getKeyframes()->value(3.0));
        obj->setPosAndOrient(pos2,orient2);
        obj->setIsVisible(false);
        obj->setActive(true);
        colors->setKeyframe2Color(obj);
        obj->addKeyframeForCurrentLocation(5.0);
        errors += testNumKeyframes(obj,2);
        errors += testKeyframe(obj->getKeyframes()->value(3.0),pos,orient,
                               true,false);
        errors += colors->testKeyframeColor(1,obj->getKeyframes()->value(3.0));
        errors += testKeyframe(obj->getKeyframes()->value(5.0),pos2,orient2,
                               false,true);
        errors += colors->testKeyframeColor(2,obj->getKeyframes()->value(5.0));
        return errors;
    }
    ObjectKeyframeTestNewMacro(KeyframeCreationTest)
};

class VisibilityAndActiveTest : public ObjectTest
{
public:
    virtual ~VisibilityAndActiveTest() {}
    ObjectTestNameMacro("VisibilityAndActive")
    virtual int testObject(SketchObject *obj)
    {
        obj->setIsVisible(false);
        int errors = test_assert_true(!obj->isVisible(),
                                      "Object is visible when set invisible");
        obj->setIsVisible(true);
        errors += test_assert_true(obj->isVisible(),
                                   "Object is invisible when set visible");
        obj->setActive(true);
        errors += test_assert_true(obj->isActive(),
                                   "Object is not active when set active");
        obj->setActive(false);
        errors += test_assert_true(!obj->isActive(),
                                   "Object is active when set not active");
        return errors;
    }
    ObjectTestNewMacro(VisibilityAndActiveTest)
};

static inline int testObjectAttrs(SketchObject* obj,
                                  const q_vec_type pos,
                                  const q_type orient,
                                  bool visible,
                                  bool active)
{
    int errors = 0;
    q_vec_type p;
    q_type o;
    obj->getPosition(p);
    obj->getOrientation(o);
    errors += vectors_should_be_equal(p,pos,"Keyframe position is wrong");
    errors += quats_should_be_equal(o,orient,"Keyframe orientation is wrong");
    errors += test_assert_true(active == obj->isActive(),
                               "Keyframe active status is wrong");
    errors += test_assert_true(visible == obj->isVisible(),
                               "Keyframe visibility is wrong");
    return errors;
}

class KeyframeInterpolationTest : public ObjectTest
{
    ObjectKeyframeTestStateAndCtor(KeyframeInterpolationTest)
    public:
    virtual ~KeyframeInterpolationTest() {}
    ObjectTestNameMacro("KeyframeInterpolation")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type pos, pos2, tmpV;
        q_type orient, orient2, tmpQ;
        q_vec_set(pos,4004,5.827,Q_PI);
        q_vec_set(pos2,5,9332.222,.1*Q_PI);
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        q_from_axis_angle(orient2,0,-1,0,-.45);
        errors += testNumKeyframes(obj,0);
        obj->setPosAndOrient(pos,orient);
        obj->setIsVisible(true);
        obj->setActive(false);
        colors->setKeyframe1Color(obj);
        obj->addKeyframeForCurrentLocation(3.0);
        obj->setPosAndOrient(pos2,orient2);
        obj->setIsVisible(false);
        obj->setActive(true);
        colors->setKeyframe2Color(obj);
        obj->addKeyframeForCurrentLocation(5.0);
        obj->setPositionByAnimationTime(4.0);
        // with spline interpolation, middle points here are unknown
        // so give the result as the expected value so no error thrown
        obj->getPosition(tmpV);
        obj->getOrientation(tmpQ);
        errors += testObjectAttrs(obj,tmpV,tmpQ,true,false);
        errors += colors->testObjectColor(1,obj);
        obj->setPositionByAnimationTime(6.0);
        errors += testObjectAttrs(obj,pos2,orient2,false,true);
        errors += colors->testObjectColor(2,obj);
        obj->setPositionByAnimationTime(1.0);
        errors += testObjectAttrs(obj,pos,orient,true,false);
        errors += colors->testObjectColor(1,obj);
        obj->setPositionByAnimationTime(5.0);
        errors += testObjectAttrs(obj,pos2,orient2,false,true);
        errors += colors->testObjectColor(2,obj);
        return errors;
    }
    ObjectKeyframeTestNewMacro(KeyframeInterpolationTest)
};

class HasChangedSinceKeyframeTest : public ObjectTest
{
    ObjectKeyframeTestStateAndCtor(HasChangedSinceKeyframeTest)
public:
    virtual ~HasChangedSinceKeyframeTest() {}
    ObjectTestNameMacro("HasChangedSinceKeyframe")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
        q_vec_type pos, pos2;
        q_type orient, orient2;
        q_vec_set(pos,4004,5.827,Q_PI);
        q_vec_set(pos2,5,9332.222,.1*Q_PI);
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        q_from_axis_angle(orient2,0,-1,0,-.45);
        errors += testNumKeyframes(obj,0);
        obj->setPosAndOrient(pos,orient);
        obj->setIsVisible(true);
        obj->setActive(false);
        colors->setKeyframe1Color(obj);
        obj->addKeyframeForCurrentLocation(3.0);
        obj->addKeyframeForCurrentLocation(3.0);
        obj->setPosAndOrient(pos2,orient2);
        obj->setIsVisible(false);
        obj->setActive(true);
        colors->setKeyframe2Color(obj);
        obj->addKeyframeForCurrentLocation(3.0);
        obj->addKeyframeForCurrentLocation(5.0);
        errors += test_assert_true(
                    !obj->hasChangedSinceKeyframe(5.0),
                    "Changed since keyframe when keyframe just set");
        obj->setPosition(pos);
        errors += test_assert_true(
                    obj->hasChangedSinceKeyframe(5.0),
                    "Position changed but has not changed since keyframe");
        obj->setPositionByAnimationTime(5.0);
        obj->setOrientation(orient);
        errors += test_assert_true(
                    obj->hasChangedSinceKeyframe(5.0),
                    "Orientation changed but has not changed since keyframe");
        obj->setPositionByAnimationTime(5.0);
        obj->setActive(!obj->isActive());
        errors += test_assert_true(
                    obj->hasChangedSinceKeyframe(5.0),
                    "Active changed but has not changed since keyframe");
        obj->setPositionByAnimationTime(5.0);
        obj->setIsVisible(!obj->isVisible());
        errors += test_assert_true(
                    obj->hasChangedSinceKeyframe(5.0),
                    "Visibility changed but has not changed since keyframe");
        errors += colors->testChangedSinceKeyframe(5.0,obj);
        return errors;
    }
    ObjectKeyframeTestNewMacro(HasChangedSinceKeyframeTest)
};

class RemoveFromGroupPosAndOrientTest : public ObjectTest
{
public:
    virtual ~RemoveFromGroupPosAndOrientTest() {}
    ObjectTestNameMacro("RemoveFromGroupPosAndOrient")
    virtual int testObject(SketchObject *obj)
    {
        int errors = 0;
		//3 keyframes. Two in group, final one out of group
		SketchObject* obj2(obj->deepCopy());
        q_vec_type pos, obj2pos, pos2, objPos2, framePos, removedPos, pos3;
        q_type orient, orient2, objOr2, removedOr, orient3;
        q_vec_set(pos,4004,5.827,Q_PI);
		q_vec_set(obj2pos,3800,5,Q_PI);
		q_vec_set(pos2,3000,5.827,.5*Q_PI);
        q_vec_set(pos3,5,9332.222,.1*Q_PI);
        q_make(orient,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
        q_normalize(orient,orient);
        q_from_axis_angle(orient2,0,-1,0,-.45);
		q_from_axis_angle(orient3,1,-1,0,-.45);
		obj->setPosAndOrient(pos,orient);
		obj2->setPosAndOrient(obj2pos,orient);
        obj->setIsVisible(true);
		obj2->setIsVisible(true);
		ObjectGroup *grp = new ObjectGroup();
        grp->addObject(obj2);
		grp->addObject(obj);
		grp->addKeyframeForCurrentLocation(0.0);
		grp->setPosAndOrient(pos2,orient2);
		obj->getPosition(objPos2);
		obj->getOrientation(objOr2);
		grp->addKeyframeForCurrentLocation(5.0);
		const QMap<double, Keyframe> *frames = obj->getKeyframes();
		Keyframe f = frames->value(5.0);
		f.getAbsolutePosition(framePos);
		errors += vectors_should_be_equal(
                    objPos2,framePos,"Keyframe absolute position is wrong");

		grp->removeObject(obj);
		obj->setPosAndOrient(pos3, orient3);
		obj->addKeyframeForCurrentLocation(10.0);
		grp->addKeyframeForCurrentLocation(10.0);
		obj->getPosAndOrFromSpline(removedPos, removedOr, 5.016);

		//Test position before, during, and after keyframe of group removal
		q_vec_type pos0, pos5, posAfterRemove, pos10;
		q_type orient0, orient5, orAfterRemove, orient10;
		obj->setPosAndOrient(pos,orient);
		grp->setPositionByAnimationTime(0.0);
		grp->addObject(obj);
		obj->getPosition(pos0);
		obj->getOrientation(orient0);
		errors += vectors_should_be_equal(
                    pos,pos0,"Position wrong on first (grouped) keyframe.");
		errors += quats_should_be_equal(
					orient,orient0,"Orientation wrong on first (grouped) keyframe.");

		grp->setPositionByAnimationTime(5.0);
		obj->getPosition(pos5);
		obj->getOrientation(orient5);
		errors += vectors_should_be_equal(
                    objPos2,pos5,"Position wrong on second (group removal) keyframe.");
		errors += quats_should_be_equal(
					objOr2,orient5,"Orientation wrong on second (group removal) keyframe.");

		grp->removeObject(obj);
		grp->setPositionByAnimationTime(5.016);
		obj->setPositionByAnimationTime(5.016);
		obj->getPosition(posAfterRemove);
		obj->getOrientation(orAfterRemove);
		errors += vectors_should_be_equal(
                    removedPos,posAfterRemove,"Position wrong after removal from group.");
		errors += quats_should_be_equal(
					removedOr,orAfterRemove,"Orientation wrong after removal from group.");

		grp->setPositionByAnimationTime(10.0);
		obj->setPositionByAnimationTime(10.0);
		obj->getPosition(pos10);
		obj->getOrientation(orient10);
		errors += vectors_should_be_equal(
                    pos3,pos10,"Position wrong on third (ungrouped) keyframe.");
		errors += quats_should_be_equal(
					orient3,orient10,"Orientation wrong on third (ungrouped) keyframe.");
		obj2->~SketchObject();
        return errors;
    }

    ObjectTestNewMacro(RemoveFromGroupPosAndOrientTest)
};

static ObjectActionTest positionActionTest(PositionTest::New,NULL);
static ObjectActionTest orientationActionTest(OrientationTest::New,NULL);
static ObjectActionTest saveAndRestoreTest(SaveAndRestoreTest::New,NULL);
static ObjectActionTest collisionGroupTest(TestCollisionGroups::New,NULL);
static ObjectActionTest pointTransformsTest(PointTransformsTest::New,NULL);
static ObjectActionTest forcesTest(ForcesTest::New,NULL);
static ObjectActionTest keyframeCreationTest(KeyframeCreationTest::New,NULL);
static ObjectActionTest visibilityAndActiveTest(VisibilityAndActiveTest::New,NULL);
static ObjectActionTest interpolationTest(KeyframeInterpolationTest::New,NULL);
static ObjectActionTest changeSinceKeyframeTest(HasChangedSinceKeyframeTest::New,NULL);
static ObjectActionTest GroupRemovalPosAndOrientTest(RemoveFromGroupPosAndOrientTest::New,NULL);
//###############################################################################
//###############################################################################
int testSketchObjectActions(SketchObject *obj)
{
    // for objectGroup only, move there
    return ObjectActionTest::runTests(obj);
}

//###############################################################################
//###############################################################################
int testNewSketchObject(SketchObject *obj)
{
    int errors = 0;
    q_vec_type nVec = Q_NULL_VECTOR;
    q_type idQuat = Q_ID_QUAT;
    q_vec_type testVec;
    q_type testQ;
    obj->getPosition(testVec);
    obj->getOrientation(testQ);
    errors += vectors_should_be_equal(nVec,testVec,"Initial position wrong.");
    errors += quats_should_be_equal(idQuat,testQ,"Initial orientation wrong.");

    obj->getLastPosition(testVec);
    obj->getLastOrientation(testQ);
    errors += vectors_should_be_equal(nVec,testVec,"Initial last position wrong.");
    errors += quats_should_be_equal(idQuat,testQ,"Initial last orientation wrong.");

    obj->getForce(testVec);
    errors += vectors_should_be_equal(nVec,testVec,"Initial state has a force.");
    obj->getTorque(testVec);
    errors += vectors_should_be_equal(nVec,testVec,"Initial state has a torque.");

    errors += test_assert_true(obj->getParent() == NULL,
                               "Initial state has a parent");
    vtkTransform *t = obj->getLocalTransform();
    errors += test_assert_true(t != NULL,
                               "Local transform not created");
    errors += test_assert_true(obj->isVisible(),
                               "Object is invisible, should be visible by default.");
    errors += test_assert_true(!obj->isActive(),
                               "Object is active, should not be active by default.");
    vtkMatrix4x4 *tm = t->GetMatrix();
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            double k = tm->GetElement(i,j);
            int correctValue = (i == j) ? 1 : 0;
            errors += test_assert_true( k == correctValue, "Local transform not identity");
        }
    }

    vtkPolyDataAlgorithm *obb = obj->getOrientedBoundingBoxes();
    errors += test_assert_true(obb != NULL, "Oriented Bounding Box(es) is/are null");

    errors += test_assert_true(!obj->hasKeyframes(), "New object has keyframes");
    errors += test_assert_true(obj->getNumKeyframes() == 0, "New object has keyframes");
    // collision group and calculate local transform are likely to be changed
    // in subclasses, so let subclasses handle those tests
    return errors;
}

//#########################################################################
int testNewModelInstance(SketchObject *obj, bool testSubObjects) {
    int errors = TestCoreHelpers::testNewSketchObject(obj);
    if (obj->numInstances() != 1) {
        errors++;
        cout << "How is one instance more than one instance?" << endl;
    }
    if (obj->getActor() == NULL) {
        errors++;
        cout << "Actor for object is null!" << endl;
    }
    if (obj->getModel() == NULL) {
        errors++;
        cout << "Model for object is null" << endl;
    }
    if (testSubObjects && obj->getSubObjects() != NULL) {
        errors++;
        cout << "List of sub-objects isn't null" << endl;
    }
    return errors;
}

class ModelInstanceTest : public KeyframeColorTest
{
public:
    virtual void setKeyframe1Color(SketchObject* obj)
    {
        obj->setArrayToColorBy("abc");
        obj->setColorMapType(ColorMapType::SOLID_COLOR_RED);
    }
    virtual void setKeyframe2Color(SketchObject* obj)
    {
        obj->setArrayToColorBy("DEF");
        obj->setColorMapType(ColorMapType::BLUE_TO_RED);
    }
    virtual int testKeyframeColor(int frameNum, const Keyframe& f)
    {
        int errors = 0;
        ColorMapType::Type t = frameNum == 1 ? ColorMapType::SOLID_COLOR_RED :
                                               ColorMapType::BLUE_TO_RED;
        const char* array = frameNum == 1 ? "abc" : "DEF";
        errors += test_assert_true(t == f.getColorMapType(),"Color map changed");
        errors += test_assert_true(array == f.getArrayToColorBy(),"Array to color by changed");
        return errors;
    }
    virtual int testObjectColor(int frameNum, SketchObject* obj)
    {
        int errors = 0;
        ColorMapType::Type t = frameNum == 1 ? ColorMapType::SOLID_COLOR_RED :
                                               ColorMapType::BLUE_TO_RED;
        const char* array = frameNum == 1 ? "abc" : "DEF";
        errors += test_assert_true(t == obj->getColorMapType(),"Color map changed");
        errors += test_assert_true(array == obj->getArrayToColorBy(),"Array to color by changed");
        return errors;
    }
    virtual int testChangedSinceKeyframe(double kfTime, SketchObject* obj)
    {
        int errors = 0;
        obj->setPositionByAnimationTime(kfTime);
        obj->setColorMapType(ColorMapType::SOLID_COLOR_YELLOW);
        errors += test_assert_true(obj->hasChangedSinceKeyframe(kfTime),
                         "Object has not changed when color changed");
        obj->setPositionByAnimationTime(kfTime);
        obj->setArrayToColorBy("GhI");
        errors += test_assert_true(obj->hasChangedSinceKeyframe(kfTime),
                         "Object has not chaned when array to color by changed");
        return errors;
    }
};


//#########################################################################
int testModelInstanceActions(SketchObject *obj) {
    ObjectActionTest keyframeCreationTest(KeyframeCreationTest::New,new ModelInstanceTest);
    ObjectActionTest interpolationTest(KeyframeInterpolationTest::New,new ModelInstanceTest);
    ObjectActionTest changeSinceKeyframeTest(HasChangedSinceKeyframeTest::New,new ModelInstanceTest);
    int errors = TestCoreHelpers::testSketchObjectActions(obj);
    // test set wireframe / set solid... can only test if mapper changed easily, no testing
    // the vtk pipeline to see if it is actually wireframed

    // test the bounding box .... assumes a radius 4 sphere
    double bb[6];
    q_vec_type pos;
    obj->getPosition(pos); // position set in getSketchObjectActions
    obj->getBoundingBox(bb);
    for (int i = 0; i < 3; i++) {
        // due to tessalation, bounding box has some larger fluctuations.
        if (Q_ABS(bb[2*i+1] - bb[2*i] - 8) > .025) {
            errors++;
            cout << "Object bounding box wrong" << endl;
        }
        // average of fluctuations has larger fluctuation
        // note model instance bounding box is now an OBB that fits the
        // untransformed model...
        if (Q_ABS((bb[2*i+1] +bb[2*i])/2) > .05) {
            errors++;
            cout << "Object bounding box position wrong" << endl;
        }
    }
    // TODO - test keyframing color map and array
    return errors;
}
//#########################################################################
//#########################################################################
int SetOfObjectTests::runTests(Node *n, SketchObject *testObject)
{
    int errors = 0;
    QScopedPointer< SketchObject > obj(testObject->deepCopy());
    Node* current = n;
    while (current != NULL)
    {
        QScopedPointer< ObjectTest > test(current->test(current->arg));
        std::cout << "Starting test: " << test->getTestName() << std::endl;
        errors += test->testObject(obj.data());
        obj.reset(testObject->deepCopy());
        current = current->next;
    }
    return errors;
}

//#########################################################################
//#########################################################################
SetOfObjectTests::Node* ObjectActionTest::actionHead = NULL;
// Adds the given function ptr to the test framework to create a test from
ObjectActionTest::ObjectActionTest(ObjectTest::Factory testMaker, void *arg)
{
    n.next = actionHead;
    n.test = testMaker;
    n.arg = arg;
    actionHead = &n;
}

ObjectActionTest::~ObjectActionTest()
{
    if (actionHead == &n)
    {
        actionHead = n.next;
    }
    else
    {
        Node* l = actionHead;
        Node* c = actionHead->next;
        while (c != NULL && c != &n)
        {
            l = c;
            c = c->next;
        }
        if (c == &n)
        {
            l->next = c->next;
        }
    }
}

int ObjectActionTest::runTests(SketchObject* testObject)
{
    return SetOfObjectTests::runTests(actionHead,testObject);
}

}
