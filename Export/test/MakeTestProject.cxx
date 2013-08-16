#include "MakeTestProject.h"

#include <cmath>

#include <quat.h>

#include <sketchmodel.h>
#include <modelmanager.h>
#include <keyframe.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <structurereplicator.h>
#include <transformequals.h>
#include <sketchproject.h>

#include <test/TestCoreHelpers.h>

#define MODEL_SOURCE "1m1j"
#define MODEL_WITH_MULTIPLE_CONFORMATIONS_SOURCE "1m1j2"

namespace MakeTestProject
{

void setUpProject(SketchProject *proj)
{
    if (proj->getModelManager()->hasModel(MODEL_SOURCE))
        return;
    using std::sqrt;
    QString filename = "models/1m1j.obj";

    SketchModel *m1 = proj->addModelFromFile(MODEL_SOURCE,filename,
                                             3*sqrt(12.0),4*sqrt(13.0));
    SketchModel *m2 = TestCoreHelpers::getCubeModel();
    SketchModel *m3 = proj->addModelFromFile(
                m2->getSource(0),
                m2->getFileNameFor(0,ModelResolution::FULL_RESOLUTION),
                m2->getInverseMass(),m2->getInverseMomentOfInertia());
    proj->getModelManager()->addConformation(
                m3,MODEL_WITH_MULTIPLE_CONFORMATIONS_SOURCE,
                m1->getFileNameFor(0,ModelResolution::FULL_RESOLUTION));
    delete m2;
}

inline void setObjectPosAndOrient(SketchObject *obj, int num)
{
    q_vec_type pos1 = {3.14 - num,
                       1.59 * (num % 2 ? 1 : -1),
                       2.65 / (num + 1)}; // pi
    q_vec_scale(pos1,sqrt(Q_PI) * num,pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71 / (num+1),
                      8.28 - num,
                      1.82,
                      85 * num); // e
    obj->setPosAndOrient(pos1,orient1);
}

inline SketchObject *makeObject(SketchProject *proj, int num,
                                int conformation = 0)
{
    if (conformation < 0 || conformation > 1)
    {
        conformation = 1;
    }
    setUpProject(proj);
    SketchModel *m1 = proj->getModelManager()->getModel(
                MODEL_WITH_MULTIPLE_CONFORMATIONS_SOURCE);
    SketchObject *obj = new ModelInstance(m1,conformation);
    setObjectPosAndOrient(obj,num);
    setColorMapForObject(obj);
    return obj;
}

SketchObject *addObjectToProject(SketchProject *proj, int conformation)
{
    int num = proj->getWorldManager()->getNumberOfObjects();
    SketchObject *obj = proj->addObject(makeObject(proj,num,conformation));
    return obj;
}

SketchObject *addCameraToProject(SketchProject *proj)
{
    int num = proj->getWorldManager()->getNumberOfObjects();
    q_vec_type pos1 = {3.14 - num,
                       1.59 * (num % 2 ? 1 : -1),
                       2.65 / (num + 1)}; // pi
    q_vec_scale(pos1,sqrt(Q_PI) * num,pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85 * num); // e
    return proj->addCamera(pos1,orient1);
}

SketchObject *addGroupToProject(SketchProject *proj, int numItemsInGroup)
{
    if (numItemsInGroup < 1)
    {
        numItemsInGroup = 1;
    }
    ObjectGroup *group = new ObjectGroup();
    for (int i = 0; i < numItemsInGroup; i++)
    {
        group->addObject(makeObject(proj,i));
    }
    int num = proj->getWorldManager()->getNumberOfObjects();
    setObjectPosAndOrient(group,num);
    proj->addObject(group);
    return group;
}

StructureReplicator * addReplicationToProject(SketchProject *proj, int numReplicas)
{
    SketchObject *o1 = addObjectToProject(proj);
    SketchObject *o2 = addObjectToProject(proj);
    return proj->addReplication(o1,o2,numReplicas);
}

void addSpringToProject(SketchProject *proj, SketchObject *o1, SketchObject *o2)
{
    int num = proj->getWorldManager()->getNumberOfSprings();
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0) + num,p1);
    if (num % 2 == 1)
    {
        p1[2] = -p1[2];
    }
    q_vec_set(p2,2.23,-6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0) + num,p2);
    proj->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),
                     3.56*sqrt(11.0),p1,p2); // sqrt(2)
}

void addTransformEqualsToProject(SketchProject *proj, int numPairs)
{
    if (numPairs < 1)
    {
        numPairs = 1;
    }
    SketchObject *o1 = addObjectToProject(proj);
    SketchObject *o2 = addObjectToProject(proj);
    QSharedPointer< TransformEquals > transformEquals =
            proj->addTransformEquals(o1,o2);
    for (int i = 1; i < numPairs; i++)
    {
        o1 = addObjectToProject(proj);
        o2 = addObjectToProject(proj);
        transformEquals->addPair(o1,o2);
    }
}

void addKeyframesToObject(SketchObject *obj, int numKeyframes)
{
    using std::sqrt;
    q_vec_type pos;
    q_type orient;
    obj->getPosition(pos);
    obj->getOrientation(orient);
    for (int i = 0; i < numKeyframes; i++)
    {
        double time = (double)i * sqrt(27);
        setObjectPosAndOrient(obj,numKeyframes * i + 37);
        obj->addKeyframeForCurrentLocation(time);
    }
    obj->setPosAndOrient(pos,orient);
}

void setColorMapForObject(SketchObject *obj)
{
#define NUMBER_OF_ARRAYS 2
    typedef SketchObject::ColorMapType CMapType;
    static CMapType::Type nextType = CMapType::SOLID_COLOR_RED;
    static int arrayToColorBy = 0;

    QString array;
    switch (arrayToColorBy)
    {
    case 0:
        array = "modelNum";
        break;
    case 1:
        array = "chainPosition";
        break;
    default:
        array = "xyz";
        break;
    }

    obj->setColorMapType(nextType);
    obj->setArrayToColorBy(array);
    switch (nextType)
    {
    case CMapType::SOLID_COLOR_RED:
        nextType = CMapType::SOLID_COLOR_BLUE;
        break;
    case CMapType::SOLID_COLOR_BLUE:
        nextType = CMapType::SOLID_COLOR_GREEN;
        break;
    case CMapType::SOLID_COLOR_GREEN:
        nextType = CMapType::SOLID_COLOR_YELLOW;
        break;
    case CMapType::SOLID_COLOR_YELLOW:
        nextType = CMapType::SOLID_COLOR_CYAN;
        break;
    case CMapType::SOLID_COLOR_CYAN:
        nextType = CMapType::SOLID_COLOR_PURPLE;
        break;
    case CMapType::SOLID_COLOR_PURPLE:
        nextType = CMapType::DIM_SOLID_COLOR_RED;
        break;
    case CMapType::DIM_SOLID_COLOR_RED:
        nextType = CMapType::DIM_SOLID_COLOR_BLUE;
        break;
    case CMapType::DIM_SOLID_COLOR_BLUE:
        nextType = CMapType::DIM_SOLID_COLOR_GREEN;
        break;
    case CMapType::DIM_SOLID_COLOR_GREEN:
        nextType = CMapType::DIM_SOLID_COLOR_YELLOW;
        break;
    case CMapType::DIM_SOLID_COLOR_YELLOW:
        nextType = CMapType::DIM_SOLID_COLOR_CYAN;
        break;
    case CMapType::DIM_SOLID_COLOR_CYAN:
        nextType = CMapType::DIM_SOLID_COLOR_PURPLE;
        break;
    case CMapType::DIM_SOLID_COLOR_PURPLE:
        nextType = CMapType::BLUE_TO_RED;
        break;
    case CMapType::BLUE_TO_RED:
        nextType = CMapType::SOLID_COLOR_RED;
        break;
    }
    arrayToColorBy = (arrayToColorBy + 1) % NUMBER_OF_ARRAYS;
}

}
