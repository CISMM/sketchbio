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

void setUpProject(SketchBio::Project *proj)
{
    if (proj->getModelManager().hasModel(MODEL_SOURCE))
        return;
    using std::sqrt;
    QString filename = "models/1m1j.obj";
    QString newFile;
    proj->getFileInProjDir(filename,newFile);

    SketchModel *m1 = new SketchModel(3*sqrt(12.0),4*sqrt(13.0));
    m1->addConformation(MODEL_SOURCE,newFile);
    proj->getModelManager().addModel(m1);
    SketchModel *m2 = TestCoreHelpers::getCubeModel();
    SketchModel *m3 = new SketchModel(m2->getInverseMass(),
                                      m2->getInverseMomentOfInertia());
    proj->getFileInProjDir(m2->getFileNameFor(0,ModelResolution::FULL_RESOLUTION),
                           newFile);
    m3->addConformation(m2->getSource(0),newFile);
    proj->getModelManager().addModel(m3);
    proj->getModelManager().addConformation(
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

inline SketchObject *makeObject(SketchBio::Project *proj, int num,
                                int conformation = 0)
{
    if (conformation < 0 || conformation > 1)
    {
        conformation = 1;
    }
    setUpProject(proj);
    SketchModel *m1 = proj->getModelManager().getModel(
                MODEL_WITH_MULTIPLE_CONFORMATIONS_SOURCE);
    SketchObject *obj = new ModelInstance(m1,conformation);
    setObjectPosAndOrient(obj,num);
    setColorMapForObject(obj);
    return obj;
}

SketchObject *addObjectToProject(SketchBio::Project *proj, int conformation)
{
    int num = proj->getWorldManager().getNumberOfObjects();
    SketchObject *obj = proj->getWorldManager().addObject(makeObject(proj,num,conformation));
    return obj;
}

SketchObject *addCameraToProject(SketchBio::Project *proj)
{
    int num = proj->getWorldManager().getNumberOfObjects();
    q_vec_type pos1 = {3.14 - num,
                       1.59 * (num % 2 ? 1 : -1),
                       2.65 / (num + 1)}; // pi
    q_vec_scale(pos1,sqrt(Q_PI) * num,pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85 * num); // e
    return proj->addCamera(pos1,orient1);
}

SketchObject *addGroupToProject(SketchBio::Project *proj, int numItemsInGroup)
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
    int num = proj->getWorldManager().getNumberOfObjects();
    setObjectPosAndOrient(group,num);
    proj->getWorldManager().addObject(group);
    return group;
}

StructureReplicator * addReplicationToProject(SketchBio::Project *proj, int numReplicas)
{
    SketchObject *o1 = addObjectToProject(proj);
    SketchObject *o2 = addObjectToProject(proj);
    return proj->addReplication(o1,o2,numReplicas);
}

void addSpringToProject(SketchBio::Project *proj, SketchObject *o1, SketchObject *o2)
{
    int num = proj->getWorldManager().getNumberOfConnectors();
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0) + num,p1);
    if (num % 2 == 1)
    {
        p1[2] = -p1[2];
    }
    q_vec_set(p2,2.23,-6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0) + num,p2);
    proj->getWorldManager().addSpring(o1,o2,p1,p2,1.41*sqrt(8.0),4.21*sqrt(10.0),
                     3.56*sqrt(11.0)); // sqrt(2)
}

void addConnectorToProject(SketchBio::Project *proj, SketchObject *o1, SketchObject *o2)
{
    int num = proj->getWorldManager().getNumberOfConnectors();
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0) + num,p1);
    if (num % 2 == 1)
    {
        p1[2] = -p1[2];
    }
    q_vec_set(p2,2.23,-6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0) + num,p2);
    Connector* conn = new Connector(o1,o2,p1,p2,0.67 / num,2.45 * num);
    proj->getWorldManager().addConnector(conn);
}

void addTransformEqualsToProject(SketchBio::Project *proj, int numPairs)
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
    QString array;
    ColorMapType::Type colorMap;
    obj->getPosition(pos);
    obj->getOrientation(orient);
    colorMap = obj->getColorMapType();
    array = obj->getArrayToColorBy();
    for (int i = 0; i < numKeyframes; i++)
    {
		double time = (double)i * std::sqrt(27.0);
        setObjectPosAndOrient(obj,numKeyframes * i + 37);
        setColorMapForObject(obj);
        obj->addKeyframeForCurrentLocation(time);
    }
    obj->setPosAndOrient(pos,orient);
    obj->setArrayToColorBy(array);
    obj->setColorMapType(colorMap);
}

void setColorMapForObject(SketchObject *obj)
{
#define NUMBER_OF_ARRAYS 2
    static ColorMapType::Type nextType = ColorMapType::SOLID_COLOR_RED;
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
    case ColorMapType::SOLID_COLOR_RED:
        nextType = ColorMapType::SOLID_COLOR_BLUE;
        break;
    case ColorMapType::SOLID_COLOR_BLUE:
        nextType = ColorMapType::SOLID_COLOR_GREEN;
        break;
    case ColorMapType::SOLID_COLOR_GREEN:
        nextType = ColorMapType::SOLID_COLOR_YELLOW;
        break;
    case ColorMapType::SOLID_COLOR_YELLOW:
        nextType = ColorMapType::SOLID_COLOR_CYAN;
        break;
    case ColorMapType::SOLID_COLOR_CYAN:
        nextType = ColorMapType::SOLID_COLOR_PURPLE;
        break;
    case ColorMapType::SOLID_COLOR_PURPLE:
        nextType = ColorMapType::DIM_SOLID_COLOR_RED;
        break;
    case ColorMapType::DIM_SOLID_COLOR_RED:
        nextType = ColorMapType::DIM_SOLID_COLOR_BLUE;
        break;
    case ColorMapType::DIM_SOLID_COLOR_BLUE:
        nextType = ColorMapType::DIM_SOLID_COLOR_GREEN;
        break;
    case ColorMapType::DIM_SOLID_COLOR_GREEN:
        nextType = ColorMapType::DIM_SOLID_COLOR_YELLOW;
        break;
    case ColorMapType::DIM_SOLID_COLOR_YELLOW:
        nextType = ColorMapType::DIM_SOLID_COLOR_CYAN;
        break;
    case ColorMapType::DIM_SOLID_COLOR_CYAN:
        nextType = ColorMapType::DIM_SOLID_COLOR_PURPLE;
        break;
    case ColorMapType::DIM_SOLID_COLOR_PURPLE:
        nextType = ColorMapType::BLUE_TO_RED;
        break;
    case ColorMapType::BLUE_TO_RED:
        nextType = ColorMapType::SOLID_COLOR_GRAY;
        break;
    case ColorMapType::SOLID_COLOR_GRAY:
        nextType = ColorMapType::SOLID_COLOR_RED;
        break;
    }
    arrayToColorBy = (arrayToColorBy + 1) % NUMBER_OF_ARRAYS;
}

}
