#include "TestCoreHelpers.h"

#include <iostream>
using std::cout;
using std::endl;

#include <quat.h>

#include <QString>

#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>

#include <sketchtests.h>
#include <sketchioconstants.h>
#include <modelutilities.h>
#include <sketchmodel.h>
#include <keyframe.h>
#include <sketchobject.h>

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
//###############################################################################
//###############################################################################
int testSketchObjectActions(SketchObject *obj)
{
    int errors = 0;
    q_vec_type v1, v2, nVec, oneVec;
    q_vec_set(nVec,0,0,0);
    q_vec_set(oneVec,1,1,1);
    q_vec_set(v1,4004,5.827,Q_PI);
    // test set position
    obj->setPosition(v1);
    obj->getPosition(v2);
    if (!q_vec_equals(v1,v2))
    {
        errors++;
        cout << "Get/set position failed" << endl;
    }
    obj->getLastPosition(v2);
    if (!q_vec_equals(v2,nVec))
    {
        errors++;
        cout << "Last location updated when position changed" << endl;
    }
    obj->getLocalTransform()->GetPosition(v2);
    if (!q_vec_equals(v1,v2))
    {
        errors++;
        cout << "Set position incorrectly updated transform position" << endl;
    }
    obj->getLocalTransform()->GetOrientation(v2);
    if (!q_vec_equals(v2,nVec))
    {
        errors++;
        cout << "Set position incorrectly updated transform rotation" << endl;
    }
    obj->getLocalTransform()->GetScale(v2);
    if (!q_vec_equals(v2,oneVec))
    {
        errors++;
        cout << "Set position incorrectly updated transform scale" << endl;
    }
    q_type q1, q2, idQ = Q_ID_QUAT;
    q_make(q1,5.63453894834839,7.23232232323232,5.4*Q_PI,Q_PI*.7);
    q_normalize(q1,q1);
    // test set orientation
    obj->setOrientation(q1);
    obj->getOrientation(q2);
    if (!q_equals(q1,q2))
    {
        errors++;
        cout << "Get/set orientation failed" << endl;
    }
    obj->getLastOrientation(q2);
    if (!q_equals(q2,idQ))
    {
        errors++;
        cout << "Set orientation affected last orientation" << endl;
    }
    obj->getLocalTransform()->GetPosition(v2);
    if (!q_vec_equals(v1,v2))
    {
        errors++;
        cout << "Set orientation incorrectly updated transform position" << endl;
    }
    double *wxyz = obj->getLocalTransform()->GetOrientationWXYZ();
    q_from_axis_angle(q2,wxyz[1],wxyz[2],wxyz[3],(Q_PI / 180) * wxyz[0]);
    if (!q_equals(q1,q2))
    {
        errors++;
        cout << "Set orientation incorrectly updated transform orientation" << endl;
    }
    obj->getLocalTransform()->GetScale(v2);
    if (!q_vec_equals(v2,oneVec))
    {
        errors++;
        cout << "Set orientation incorrectly updated transform scale" << endl;
    }
    // test save/restore position
    q_vec_type v3 = { 5, 9332.222, .1 * Q_PI};
    q_type q3;
    q_from_axis_angle(q3,0,-1,0,-.45);
    obj->setLastLocation();
    obj->setPosAndOrient(v3,q3);
    obj->getPosition(v2);
    if (!q_vec_equals(v2,v3))
    {
        errors++;
        cout << "Set position/orientation at same time failed for position" << endl;
    }
    obj->getOrientation(q2);
    if (!q_equals(q2,q3))
    {
        errors++;
        cout << "Set position/orientation at same time failed for orientation" << endl;
    }
    obj->getLastPosition(v2);
    if (!q_vec_equals(v1,v2))
    {
        errors++;
        cout << "Set last location failed on position" << endl;
    }
    obj->getLastOrientation(q2);
    if (!q_equals(q1,q2))
    {
        errors++;
        cout << "Set last location failed on orientation" << endl;
    }
    obj->restoreToLastLocation();
    obj->getPosition(v2);
    if (!q_vec_equals(v1,v2))
    {
        errors++;
        cout << "Restore last location failed on position" << endl;
    }
    obj->getOrientation(q2);
    if (!q_vec_equals(q1,q2))
    {
        errors++;
        cout << "Restore last location failed on orientation" << endl;
    }
    // test setPrimaryCollisionGroupNum
    obj->setPrimaryCollisionGroupNum(45);
    if (obj->getPrimaryCollisionGroupNum() != 45)
    {
        errors++;
        cout << "Set primary collision group num failed" << endl;
    }
    if (!obj->isInCollisionGroup(45))
    {
        errors++;
        cout << "Is in collision group failed" << endl;
    }
    // test modelSpacePointInWorldCoords
    obj->getModelSpacePointInWorldCoordinates(nVec,v2);
    if (!q_vec_equals(v1,v2))
    {
        errors++;
        cout << "Model point in world coords failed on null vector" << endl;
    }
    obj->getModelSpacePointInWorldCoordinates(v3,v2);
    q_vec_subtract(v2,v2,v1);
    q_invert(q2,q1);
    q_xform(v2,q2,v2);
    if (!q_vec_equals(v2,v3))
    {
        errors++;
        cout << "Model point in world coords failed on interesting point" << endl;
    }
    // test worldVectorInModelSpace
    obj->getWorldVectorInModelSpace(v3,v2);
    q_xform(v2,q1,v2);
    if (!q_vec_equals(v2,v3))
    {
        errors++;
        cout << "World vector to modelspace failed" << endl;
    }
    // test forces methods
    q_vec_type f1 = {3,4,5}, f2 = {7,-2,-6.37};
    obj->addForce(nVec,f1);
    obj->getForce(v2);
    if (!q_vec_equals(v2,f1))
    {
        errors++;
        cout << "Applying linear force failed" << endl;
    }
    obj->getTorque(v2);
    if (!q_vec_equals(v2,nVec))
    {
        errors++;
        cout << "Applying force at center caused torque" << endl;
    }
    obj->addForce(v3,f2);
    obj->getForce(v2);
    q_vec_subtract(v2,v2,f2);
    if (!q_vec_equals(v2,f1))
    {
        errors++;
        cout << "Combining forces failed." << endl;
    }
    obj->getTorque(v2);
    q_vec_type tmp, t2;
    obj->getWorldVectorInModelSpace(f2,tmp);
    q_vec_cross_product(t2,v3,tmp);
    if (!q_vec_equals(t2,v2))
    {
        errors++;
        cout << "Torque calculations failed." << endl;
    }
    obj->clearForces();
    obj->getForce(v2);
    if (!q_vec_equals(v2,nVec))
    {
        errors++;
        cout << "Clearing force failed." << endl;
    }
    obj->getTorque(v2);
    if (!q_vec_equals(v2,nVec))
    {
        errors++;
        cout << "Clearing torques failed" << endl;
    }
    // test keyframing
    obj->addKeyframeForCurrentLocation(3.0);
    if (!obj->hasKeyframes())
    {
        errors++;
        cout << "Has keyframes is false when there is a keyframe" << endl;
    }
    if (obj->getNumKeyframes() != 1)
    {
        errors++;
        cout << "Num keyframes is wrong... 1" << endl;
    }
    Keyframe f = obj->getKeyframes()->value(3.0);
    f.getPosition(v2);
    if (!q_vec_equals(v2,v1))
    {
        errors++;
        cout << "Keyframe position is wrong" << endl;
    }
    f.getOrientation(q2);
    if (!q_equals(q2,q1))
    {
        errors++;
        cout << "Keyframe orientation is wrong" << endl;
    }
    if (!f.isVisibleAfter())
    {
        errors++;
        cout << "Keyframe visibility is wrong" << endl;
    }
    if (f.isActive())
    {
        errors++;
        cout << "Keyframe active status is wrong" << endl;
    }
    obj->setPosAndOrient(v3,q3);
    obj->setIsVisible(false);
    if (obj->isVisible())
    {
        errors++;
        cout << "Object is not invisible when set invisible" << endl;
    }
    obj->setActive(true);
    if (!obj->isActive())
    {
        errors++;
        cout << "Object is not active when set active" << endl;
    }
    obj->addKeyframeForCurrentLocation(5.0);
    if (obj->getNumKeyframes() != 2)
    {
        errors++;
        cout << "Num keyframes is wrong... 2" << endl;
    }
    f = obj->getKeyframes()->value(3.0);
    f.getPosition(v2);
    if (!q_vec_equals(v2,v1))
    {
        errors++;
        cout << "Keyframe position is wrong" << endl;
    }
    f.getOrientation(q2);
    if (!q_equals(q2,q1))
    {
        errors++;
        cout << "Keyframe orientation is wrong" << endl;
    }
    f = obj->getKeyframes()->value(5.0);
    f.getPosition(v2);
    if (!q_vec_equals(v2,v3))
    {
        errors++;
        cout << "Keyframe position is wrong" << endl;
    }
    f.getOrientation(q2);
    if (!q_equals(q2,q3))
    {
        errors++;
        cout << "Keyframe orientation is wrong" << endl;
    }
    if (!f.isActive())
    {
        errors++;
        cout << "Keyframe is not set active when object was." << endl;
    }
    if (f.isVisibleAfter())
    {
        errors++;
        cout << "Keyframe is visible when object was not." << endl;
    }
    // test interpolation between keyframes
    obj->setPositionByAnimationTime(4.0);
    obj->getPosition(v2);
    q_vec_type vtmp;
    q_type qtmp;
    q_vec_add(vtmp,v3,v1);
    q_vec_scale(vtmp,.5,vtmp);
    if (!q_vec_equals(vtmp,v2))
    {
        errors++;
        cout << "Interpolation wrong between 2 keyframes position" << endl;
        // TODO may change test later
    }
    q_slerp(qtmp,q1,q3,.5);
    obj->getOrientation(q2);
    if (!q_equals(qtmp,q2))
    {
        errors++;
        cout << "Interpolation wrong between 2 keyframes orientation" << endl;
    }
    if (!obj->isVisible())
    {
        errors++;
        cout << "Keyframe set did not update object visibility status" << endl;
    }
    if (obj->isActive())
    {
        errors++;
        cout << "Keyframe set did not update object active status" << endl;
    }
    // test corner case: go to time after last keyframe
    obj->setPositionByAnimationTime(6.0);
    obj->getPosition(v2);
    obj->getOrientation(q2);
    if (!q_vec_equals(v2,v3))
    {
        errors++;
        cout << "Incorrect handling of go to position after last keyframe." << endl;
    }
    if (!q_equals(q2,q3))
    {
        errors++;
        cout << "Incorrect handling of go to orientation after last keyframe." << endl;
    }
    if (obj->isVisible())
    {
        errors++;
        cout << "Incorrect handing of visibility after last keyframe." << endl;
    }
    if (!obj->isActive())
    {
        errors++;
        cout << "Incorrect handling of active status after last keyframe." << endl;
    }
    // test corner case: go to time before first keyframe
    obj->setPositionByAnimationTime(0.5);
    obj->getPosition(v2);
    obj->getOrientation(q2);
    if (!q_vec_equals(v2,v1))
    {
        errors++;
        cout << "Incorrect handling of go to position before first keyframe." << endl;
    }
    if (!q_equals(q2,q1))
    {
        errors++;
        cout << "Incorrect handling of go to orientation before first keyframe." << endl;
    }
    if (!obj->isVisible())
    {
        errors++;
        cout << "Incorrect handing of visibility before first keyframe." << endl;
    }
    if (obj->isActive())
    {
        errors++;
        cout << "Incorrect handling of active status before first keyframe." << endl;
    }
    // test corner case: go to time of keyframe exactly
    obj->setPositionByAnimationTime(5.0);
    obj->getPosition(v2);
    obj->getOrientation(q2);
    if (!q_vec_equals(v2,v3))
    {
        errors++;
        cout << "Incorrect handling of go to position when landing on keyframe." << endl;
    }
    if (!q_equals(q2,q3))
    {
        errors++;
        cout << "Incorrect handling of go to orientation when landing on keyframe." << endl;
    }
    if (obj->isVisible())
    {
        errors++;
        cout << "Incorrect handing of visibility when landing on keyframe." << endl;
    }
    if (!obj->isActive())
    {
        errors++;
        cout << "Incorrect handling of active status when landing on keyframe." << endl;
    }
    return errors;
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
    if (!q_vec_equals(nVec,testVec))
    {
        errors++;
        cout << "Initial position wrong." << endl;
    }
    if (!q_equals(idQuat,testQ))
    {
        errors++;
        cout << "Initial orientation wrong" << endl;
    }
    obj->getLastPosition(testVec);
    obj->getLastOrientation(testQ);
    if (!q_vec_equals(nVec,testVec))
    {
        errors++;
        cout << "Initial last position wrong" << endl;
    }
    if (!q_equals(idQuat,testQ))
    {
        errors++;
        cout << "Initial last orientation wrong" << endl;
    }
    obj->getForce(testVec);
    if (!q_vec_equals(nVec,testVec))
    {
        errors++;
        cout << "Initial state has a force" << endl;
    }
    obj->getTorque(testVec);
    if (!q_vec_equals(nVec,testVec))
    {
        errors++;
        cout << "Initial state has a torque" << endl;
    }
    if (obj->getParent() != NULL)
    {
        errors++;
        cout << "Initial state has a parent" << endl;
    }
    vtkTransform *t = obj->getLocalTransform();
    if (t == NULL)
    {
        errors++;
        cout << "Local transform not created" << endl;
    }
    if (!obj->isVisible())
    {
        errors++;
        cout << "Object is invisible, should be visible by default." << endl;
    }
    if (obj->isActive())
    {
        errors++;
        cout << "Object is active, they should never be active by default." << endl;
    }
    vtkMatrix4x4 *tm = t->GetMatrix();
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int k = tm->GetElement(i,j);
            if (i == j)
            {
                if (k != 1)
                {
                    errors++;
                    cout << "Local transform not identity" << endl;
                }
            }
            else if (k != 0)
            {
                errors++;
                cout << "Local transform not identity" << endl;
            }
        }
    }
    vtkPolyDataAlgorithm *obb = obj->getOrientedBoundingBoxes();
    if (obb == NULL)
    {
        errors++;
        cout << "Oriented Bounding Box(es) is(/are) null" << endl;
    }
    if (obj->hasKeyframes())
    {
        errors++;
        cout << "New object has keyframes" << endl;
    }
    if (obj->getNumKeyframes() != 0)
    {
        errors++;
        cout << "New object already has "<< obj->getNumKeyframes() << " keyframes?!?" << endl;
    }
    if (obj->getKeyframes() != NULL)
    {
        errors++;
        cout << "Keyframe map on new object not null" << endl;
    }
    // collision group and calculate local transform are likely to be changed
    // in subclasses, so let subclasses handle those tests
    return errors;
}
}
