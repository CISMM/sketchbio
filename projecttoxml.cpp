#include "projecttoxml.h"
#include <ios>
#include <sketchioconstants.h>
#include <sketchtests.h>
#include <QScopedPointer>


#define ID_ATTRIBUTE_NAME                       "id"
#define POSITION_ATTRIBUTE_NAME                 "position"
#define ROTATION_ATTRIBUTE_NAME                 "orientation"
#define PROPERTIES_ELEMENT_NAME                 "properties"
#define TRANSFORM_ELEMENT_NAME                  "transform"

#define ROOT_ELEMENT_NAME                       "sketchbio"
#define VERSION_ATTRIBUTE_NAME                  "version"
#define SAVE_MAJOR_VERSION                      1
#define SAVE_MINOR_VERSION                      4
#define SAVE_VERSION_NUM                        (QString::number(SAVE_MAJOR_VERSION) + "." + QString::number(SAVE_MINOR_VERSION))

#define MODEL_MANAGER_ELEMENT_NAME              "models"

#define MODEL_ELEMENT_NAME                      "model"
#define MODEL_NUM_CONFORMATIONS_ATTR_NAME       "numConformations"
#define MODEL_CONFORMATION_ELEMENT_NAME         "conformation"
#define MODEL_CONFORMATION_NUMBER_ATTR_NAME     "confNum"
#define MODEL_RESOLUTION_ELEMENT_NAME           "resolution"
#define MODEL_FILENAME_ATTRIBUTE_NAME           "filename"
#define MODEL_SOURCE_ELEMENT_NAME               "source"
#define MODEL_SCALE_ATTRIBUTE_NAME              "scale"
#define MODEL_TRANSLATE_ATTRIBUTE_NAME          "translate"
#define MODEL_ROTATE_ATTRIBUTE_NAME             "rotation"
#define MODEL_ROTATE_ENABLED_ATTR_NAME          "canRotate"
#define MODEL_IMASS_ATTRIBUTE_NAME              "inverseMass"
#define MODEL_IMOMENT_ATTRIBUTE_NAME            "inverseMomentOfInertia"

#define TRANSFORM_MANAGER_ELEMENT_NAME          "viewpoint"
#define TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME    "worldToRoom"
#define TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME      "roomToEye"
#define TRANSFORM_MATRIX_ATTRIBUTE_NAME         "matrix"

#define OBJECTLIST_ELEMENT_NAME                 "objectlist"
#define OBJECT_ELEMENT_NAME                     "object"
#define OBJECT_MODELID_ATTRIBUTE_NAME           "modelid"
#define OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME     "numInstances"
#define OBJECT_VISIBILITY_ATTRIBUTE_NAME        "visibility"
#define OBJECT_ACTIVE_ATTRIBUTE_NAME            "active"
#define OBJECT_REPLICA_NUM_ATTRIBUTE_NAME       "replicaNum"
#define OBJECT_KEYFRAME_LIST_ELEMENT_NAME       "keyframeList"
#define OBJECT_KEYFRAME_ELEMENT_NAME            "keyframe"
#define OBJECT_KEYFRAME_TIME_ATTRIBUTE_NAME     "time"
#define OBJECT_KEYFRAME_VIS_AF_ATTR_NAME        "visibility"
#define OBJECT_KEYFRAME_ACTIVE_ATTR_NAME        "active"

#define REPLICATOR_LIST_ELEMENT_NAME            "replicatorList"
#define REPLICATOR_ELEMENT_NAME                 "replicator"
#define REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME     "numReplicas"
#define REPLICATOR_OBJECT1_ATTRIBUTE_NAME       "original1"
#define REPLICATOR_OBJECT2_ATTRIBUTE_NAME       "original2"

#define SPRING_LIST_ELEMENT_NAME                "springList"
#define SPRING_ELEMENT_NAME                     "spring"
#define SPRING_OBJECT_END_ELEMENT_NAME          "objConnection"
#define SPRING_POINT_END_ELEMENT_NAME           "pointConnection"
#define SPRING_OBJECT_ID_ATTRIBUTE_NAME         "objId"
#define SPRING_CONNECTION_POINT_ATTRIBUTE_NAME  "connectionPoint"
#define SPRING_STIFFNESS_ATTRIBUTE_NAME         "stiffness"
#define SPRING_MIN_REST_ATTRIBUTE_NAME          "minRestLen"
#define SPRING_MAX_REST_ATTRIBUTE_NAME          "maxRestLen"

#define TRANSFORM_OP_LIST_ELEMENT_NAME          "transformOpList"
#define TRANSFORM_OP_ELEMENT_NAME               "transformOp"
#define TRANSFORM_OP_PAIR_ELEMENT_NAME          "objectPair"
#define TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME  "firstObject"
#define TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME "secondObject"

inline void setPreciseVectorAttribute(vtkXMLDataElement *elem, const double *vect, int len, const char *attrName) {
    QString data;
    for (int i = 0; i < len; i++) {
        int precision = (int) floor(0.5+log(vect[i])-log(Q_EPSILON));
        data = data + QString::number(vect[i],'g',max(11,precision)) + " ";
    }
    elem->SetAttribute(attrName,data.trimmed().toStdString().c_str());
}

vtkXMLDataElement *ProjectToXML::projectToXML(const SketchProject *project) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(ROOT_ELEMENT_NAME);
    element->SetAttribute(VERSION_ATTRIBUTE_NAME,SAVE_VERSION_NUM.toStdString().c_str());

    QHash<const SketchModel *, QString> modelIds;
    QHash<const SketchObject *, QString> objectIds;

    vtkSmartPointer<vtkXMLDataElement> child =modelManagerToXML(project->getModelManager(),project->getProjectDir(),modelIds);
    element->AddNestedElement(child);
    child->Delete();
    child = transformManagerToXML(project->getTransformManager());
    element->AddNestedElement(child);
    child->Delete();
    child = objectListToXML(project->getWorldManager(),modelIds,objectIds);
    element->AddNestedElement(child);
    child->Delete();
    child = replicatorListToXML(project->getReplicas(),modelIds,objectIds);
    element->AddNestedElement(child);
    child->Delete();
    child = springListToXML(project->getWorldManager(),objectIds);
    element->AddNestedElement(child);
    child->Delete();
    child = transformOpListToXML(project->getTransformOps(),objectIds);
    element->AddNestedElement(child);
    child->Delete();

    return element;
}

vtkXMLDataElement *ProjectToXML::modelManagerToXML(const ModelManager *models, const QString &dir,
                                                   QHash<const SketchModel *, QString> &modelIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(MODEL_MANAGER_ELEMENT_NAME);
    modelIds.reserve(models->getNumberOfModels());

    int id= 0;

    QVectorIterator<SketchModel *> it = models->getModelIterator();
    while (it.hasNext()) {
        const SketchModel *model = it.next();
        QString idStr = QString("M%1").arg(id);
        vtkSmartPointer<vtkXMLDataElement> child = modelToXML(model, dir,idStr);
        element->AddNestedElement(child);
        child->Delete();
        modelIds.insert(model,idStr);
        std:: cout << "Saving model " << id << "." << std::endl;
        id++;
    }

    return element;
}

inline const char *getResolutionString(ModelResolution::ResolutionType res)
{
    if (res == ModelResolution::FULL_RESOLUTION)
        return "full_resolution";
    else if (res == ModelResolution::SIMPLIFIED_FULL_RESOLUTION)
        return "simplified_full_resolution";
    else if (res == ModelResolution::SIMPLIFIED_5000)
        return "5000_tris";
    else if (res == ModelResolution::SIMPLIFIED_2000)
        return "2000_tris";
    else if (res == ModelResolution::SIMPLIFIED_1000)
        return "1000_tris";
    else
    {
        std::cout << "ERROR: Unknown resolution type in model save." << std::endl;
        return NULL;
    }
}

inline ModelResolution::ResolutionType getResolutionTypeFromId(const char *r)
{
    if (strcmp(r,"full_resolution") == 0)
        return ModelResolution::FULL_RESOLUTION;
    if (strcmp(r, "simplified_full_resolution") == 0)
        return ModelResolution::SIMPLIFIED_FULL_RESOLUTION;
    if (strcmp(r, "5000_tris") == 0)
        return ModelResolution::SIMPLIFIED_5000;
    if (strcmp(r, "2000_tris") == 0)
        return ModelResolution::SIMPLIFIED_2000;
    if (strcmp(r, "1000_tris") == 0)
        return ModelResolution::SIMPLIFIED_1000;
    else
    {
        std::cout << "ERROR: Unknown resolution type in model load." << std::endl;
        throw "Failed to find resolution";
    }
}

vtkXMLDataElement *ProjectToXML::modelToXML(const SketchModel *model,
                                            const QString &dir,
                                            const QString &id)
{
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(MODEL_ELEMENT_NAME);
    element->SetAttribute(ID_ATTRIBUTE_NAME,id.toStdString().c_str());
    element->SetIntAttribute(MODEL_NUM_CONFORMATIONS_ATTR_NAME,
                             model->getNumberOfConformations());
    vtkSmartPointer< vtkXMLDataElement > properties =
            vtkSmartPointer< vtkXMLDataElement >::New();
    properties->SetName(PROPERTIES_ELEMENT_NAME);
    properties->SetAttribute(MODEL_ROTATE_ENABLED_ATTR_NAME,
                          model->shouldRotate() ? "true" : "false");
    properties->SetDoubleAttribute(MODEL_IMASS_ATTRIBUTE_NAME,
                                   model->getInverseMass());
    properties->SetDoubleAttribute(MODEL_IMOMENT_ATTRIBUTE_NAME,
                                   model->getInverseMomentOfInertia());
    element->AddNestedElement(properties);

    for (int i = 0; i < model->getNumberOfConformations(); i++)
    {
        vtkSmartPointer< vtkXMLDataElement > conformationElt
                = vtkSmartPointer< vtkXMLDataElement >::New();
        conformationElt->SetName(MODEL_CONFORMATION_ELEMENT_NAME);
        conformationElt->SetIntAttribute(MODEL_CONFORMATION_NUMBER_ATTR_NAME,i);
        vtkSmartPointer< vtkXMLDataElement > child
                = vtkSmartPointer< vtkXMLDataElement >::New();
        child->SetName(MODEL_SOURCE_ELEMENT_NAME);
        const char *data = model->getSource(i).toStdString().c_str();
        child->SetCharacterData(data,strlen(data));
        conformationElt->AddNestedElement(child);
        QString filename;
        filename = model->getFileNameFor(i,ModelResolution::FULL_RESOLUTION);
        if (filename.startsWith(dir))
            filename = filename.mid(dir.length()+1);
        child = vtkSmartPointer< vtkXMLDataElement >::New();
        child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
        child->SetAttribute(ID_ATTRIBUTE_NAME,
                            getResolutionString(ModelResolution::FULL_RESOLUTION));
        child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                            filename.toStdString().c_str());
        conformationElt->AddNestedElement(child);
        filename = model->getFileNameFor(i,ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
        if (filename.length() > 0)
        {
            if (filename.startsWith(dir))
                filename = filename.mid(dir.length()+1);
            child = vtkSmartPointer< vtkXMLDataElement >::New();
            child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
            child->SetAttribute(ID_ATTRIBUTE_NAME,
                                getResolutionString(
                                    ModelResolution::SIMPLIFIED_FULL_RESOLUTION));
            child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                                filename.toStdString().c_str());
            conformationElt->AddNestedElement(child);
        }
        filename = model->getFileNameFor(i,ModelResolution::SIMPLIFIED_5000);
        if (filename.length() > 0)
        {
            if (filename.startsWith(dir))
                filename = filename.mid(dir.length()+1);
            child = vtkSmartPointer< vtkXMLDataElement >::New();
            child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
            child->SetAttribute(ID_ATTRIBUTE_NAME,
                                getResolutionString(ModelResolution::SIMPLIFIED_5000));
            child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                                filename.toStdString().c_str());
            conformationElt->AddNestedElement(child);
        }
        filename = model->getFileNameFor(i,ModelResolution::SIMPLIFIED_2000);
        if (filename.length() > 0)
        {
            if (filename.startsWith(dir))
                filename = filename.mid(dir.length()+1);
            child = vtkSmartPointer< vtkXMLDataElement >::New();
            child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
            child->SetAttribute(ID_ATTRIBUTE_NAME,
                                getResolutionString(ModelResolution::SIMPLIFIED_2000));
            child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                                filename.toStdString().c_str());
            conformationElt->AddNestedElement(child);
        }
        filename = model->getFileNameFor(i,ModelResolution::SIMPLIFIED_1000);
        if (filename.length() > 0)
        {
            if (filename.startsWith(dir))
                filename = filename.mid(dir.length()+1);
            child = vtkSmartPointer< vtkXMLDataElement >::New();
            child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
            child->SetAttribute(ID_ATTRIBUTE_NAME,
                                getResolutionString(ModelResolution::SIMPLIFIED_1000));
            child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                                filename.toStdString().c_str());
            conformationElt->AddNestedElement(child);
        }
        element->AddNestedElement(conformationElt);
    }

    return element;
}

inline vtkXMLDataElement *matrixToXML(const char *elementName,const vtkMatrix4x4 *matrix) {
    double mat[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mat[i*4+j] = matrix->GetElement(i,j);
        }
    }
    vtkXMLDataElement *child = vtkXMLDataElement::New();
    child->SetName(elementName);
    setPreciseVectorAttribute(child,mat,16,TRANSFORM_MATRIX_ATTRIBUTE_NAME);
    return child;
}

vtkXMLDataElement *ProjectToXML::transformManagerToXML(const TransformManager *transforms) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(TRANSFORM_MANAGER_ELEMENT_NAME);

    vtkSmartPointer<vtkXMLDataElement> child = matrixToXML(TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME,
                                                           transforms->getWorldToRoomMatrix());
    element->AddNestedElement(child);
    child->Delete();
    child = matrixToXML(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME,
                        transforms->getRoomToEyeMatrix());
    element->AddNestedElement(child);
    child->Delete();

    return element;
}

vtkXMLDataElement *ProjectToXML::objectListToXML(const WorldManager *world,
                                                 const QHash<const SketchModel *, QString> &modelIds,
                                                 QHash<const SketchObject *, QString> &objectIds) {
    return objectListToXML(world->getObjects(),modelIds,objectIds);
}

vtkXMLDataElement *ProjectToXML::objectListToXML(const QList<SketchObject *> *objectList,
                                                 const QHash<const SketchModel *, QString> &modelIds,
                                                 QHash<const SketchObject *, QString> &objectIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(OBJECTLIST_ELEMENT_NAME);
    for (QListIterator<SketchObject *> it(*objectList); it.hasNext();) {
        const SketchObject *obj = it.next();
        const ReplicatedObject *rObj = dynamic_cast<const ReplicatedObject *>(obj);
        if (rObj == NULL) {
            QString idStr = QString("O%1").arg(objectIds.size());
            vtkSmartPointer<vtkXMLDataElement> child = objectToXML(obj,modelIds,objectIds,idStr);
            element->AddNestedElement(child);
            child->Delete();
        }
    }
    return element;
}

vtkXMLDataElement *ProjectToXML::objectToXML(const SketchObject *object,
                                             const QHash<const SketchModel *,QString> &modelIds,
                                             QHash<const SketchObject *, QString> &objectIds, const QString &id) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(OBJECT_ELEMENT_NAME);
    element->SetAttribute(ID_ATTRIBUTE_NAME,id.toStdString().c_str());
    objectIds.insert(object,id); // do this here now so object and its children don't have same id
    bool isGroup = object->numInstances() != 1;
    element->SetIntAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,object->numInstances());

    vtkSmartPointer<vtkXMLDataElement> child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(PROPERTIES_ELEMENT_NAME);
    if (!isGroup) {
        QString modelId("#");
        QHash< const SketchModel *, QString >::const_iterator itr =
                modelIds.constFind(object->getModel());
        if (itr == modelIds.constEnd())
            std::cout << "Error finding model." << std::endl;
        QString idx = itr.value();
        modelId = modelId.append(idx);
        child->SetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME,modelId.toStdString().c_str());
    }
    // Current thoughts: collision groups should be recreated from effects placed on objects being recreated.
    //   There is no reason that they should need to be saved.
    child->SetAttribute(OBJECT_VISIBILITY_ATTRIBUTE_NAME,object->isVisible() ? "true" : "false");
    child->SetAttribute(OBJECT_ACTIVE_ATTRIBUTE_NAME, object->isActive() ? "true" : "false");

    const ReplicatedObject *rObj = dynamic_cast<const ReplicatedObject *>(object);
    if (rObj != NULL) {
        child->SetIntAttribute(OBJECT_REPLICA_NUM_ATTRIBUTE_NAME,rObj->getReplicaNum());
    }
    element->AddNestedElement(child);

    child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(TRANSFORM_ELEMENT_NAME);
    q_vec_type pos;
    q_type orient;
    object->getPosition(pos);
    object->getOrientation(orient);
    setPreciseVectorAttribute(child,pos,3,POSITION_ATTRIBUTE_NAME);
    setPreciseVectorAttribute(child,orient,4,ROTATION_ATTRIBUTE_NAME);

    element->AddNestedElement(child);

    if (object->getNumKeyframes() > 0) {
        // object has keyframes, save them
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(OBJECT_KEYFRAME_LIST_ELEMENT_NAME);
        const QMap<double, Keyframe> *keyframeMap = object->getKeyframes();
        QMapIterator<double, Keyframe> it(*keyframeMap);
        while (it.hasNext()) {
            double time = it.peekNext().key();
            const Keyframe &frame = it.next().value();
            q_vec_type p;
            q_type o;
            frame.getPosition(p);
            frame.getOrientation(o);
            vtkSmartPointer<vtkXMLDataElement> frameElement = vtkSmartPointer<vtkXMLDataElement>::New();
            frameElement->SetName(OBJECT_KEYFRAME_ELEMENT_NAME);
            frameElement->SetDoubleAttribute(OBJECT_KEYFRAME_TIME_ATTRIBUTE_NAME,time);
            vtkSmartPointer<vtkXMLDataElement> tfrm = vtkSmartPointer<vtkXMLDataElement>::New();
            tfrm->SetName(TRANSFORM_ELEMENT_NAME);
            setPreciseVectorAttribute(tfrm,p,3,POSITION_ATTRIBUTE_NAME);
            setPreciseVectorAttribute(tfrm,o,4,ROTATION_ATTRIBUTE_NAME);
            frameElement->AddNestedElement(tfrm);
            vtkSmartPointer<vtkXMLDataElement> visibility = vtkSmartPointer<vtkXMLDataElement>::New();
            visibility->SetName(PROPERTIES_ELEMENT_NAME);
            visibility->SetAttribute(OBJECT_KEYFRAME_VIS_AF_ATTR_NAME,(frame.isVisibleAfter()?"true":"false"));
            visibility->SetAttribute(OBJECT_KEYFRAME_ACTIVE_ATTR_NAME,(frame.isActive() ? "true" : "false"));
            frameElement->AddNestedElement(visibility);
            child->AddNestedElement(frameElement);
        }
        element->AddNestedElement(child);
    }

    if (object->numInstances() > 1) {
        vtkXMLDataElement *list = ProjectToXML::objectListToXML(object->getSubObjects(),modelIds,objectIds);
        element->AddNestedElement(list);
        list->Delete();
    }

    return element;
}

vtkXMLDataElement *ProjectToXML::replicatorListToXML(const QList<StructureReplicator *> *replicaList,
                                                     const QHash<const SketchModel *, QString> &modelIds,
                                                     QHash<const SketchObject *, QString> &objectIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(REPLICATOR_LIST_ELEMENT_NAME);
    for (QListIterator<StructureReplicator *> it(*replicaList); it.hasNext();) {
        StructureReplicator *rep = it.next();
        vtkSmartPointer<vtkXMLDataElement> repElement = vtkSmartPointer<vtkXMLDataElement>::New();
        repElement->SetName(REPLICATOR_ELEMENT_NAME);
        repElement->SetAttribute(REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME,
                                 QString::number(rep->getNumShown()).toStdString().c_str());
        // i'm not checking contains, it had better be in there
        repElement->SetAttribute(REPLICATOR_OBJECT1_ATTRIBUTE_NAME,
                                 ("#" + objectIds.value(rep->getFirstObject())).toStdString().c_str());
        repElement->SetAttribute(REPLICATOR_OBJECT2_ATTRIBUTE_NAME,
                                 ("#" + objectIds.value(rep->getSecondObject())).toStdString().c_str());

        vtkSmartPointer<vtkXMLDataElement> list = vtkSmartPointer<vtkXMLDataElement>::New();
        list->SetName(OBJECTLIST_ELEMENT_NAME);
        for (QListIterator<SketchObject *> tr = rep->getReplicaIterator(); tr.hasNext();) {
            const SketchObject *obj = tr.next();
            QString idStr = QString("O%1").arg(objectIds.size());
            vtkSmartPointer<vtkXMLDataElement> child = objectToXML(obj,modelIds,objectIds,idStr);
            list->AddNestedElement(child);
            child->Delete();
            objectIds.insert(obj,idStr);
        }
        repElement->AddNestedElement(list);

        element->AddNestedElement(repElement);
    }
    return element;
}

vtkXMLDataElement *ProjectToXML::springListToXML(const WorldManager *world, const QHash<const SketchObject *, QString> &objectIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(SPRING_LIST_ELEMENT_NAME);
    for (QListIterator<SpringConnection *> it = world->getSpringsIterator(); it.hasNext();) {
        const SpringConnection *spring = it.next();
        vtkSmartPointer<vtkXMLDataElement> child = springToXML(spring,objectIds);
        if (child != NULL) {
            element->AddNestedElement(child);
            child->Delete();
        }
    }
    return element;
}

vtkXMLDataElement *ProjectToXML::springToXML(const SpringConnection *spring, const QHash<const SketchObject *, QString> &objectIds) {
    if (spring->getObject1() == spring->getObject2()) {
        return NULL;
    }
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(SPRING_ELEMENT_NAME);
    vtkSmartPointer<vtkXMLDataElement> child = vtkSmartPointer<vtkXMLDataElement>::New();
    q_vec_type pos1;
    // spring properties
    child->SetName(PROPERTIES_ELEMENT_NAME);
    double v = spring->getStiffness();
    setPreciseVectorAttribute(child,&v,1,SPRING_STIFFNESS_ATTRIBUTE_NAME);
    v = spring->getMinRestLength();
    setPreciseVectorAttribute(child,&v,1,SPRING_MIN_REST_ATTRIBUTE_NAME);
    v = spring->getMaxRestLength();
    setPreciseVectorAttribute(child,&v,1,SPRING_MAX_REST_ATTRIBUTE_NAME);
    element->AddNestedElement(child);
    // second end, depends on spring type
    if (spring->getObject1() != NULL && spring->getObject2() != NULL) {
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(SPRING_OBJECT_END_ELEMENT_NAME);
        child->SetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME,
                    ("#" + objectIds.value(spring->getObject1())).toStdString().c_str());
        spring->getObject1ConnectionPosition(pos1);
        setPreciseVectorAttribute(child,pos1,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
        element->AddNestedElement(child);
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(SPRING_OBJECT_END_ELEMENT_NAME);
        child->SetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME,
                            ("#" +objectIds.value(spring->getObject2())).toStdString().c_str());
        q_vec_type pos2;
        spring->getObject2ConnectionPosition(pos2);
        setPreciseVectorAttribute(child,pos2,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
        element->AddNestedElement(child);
    } else if ((spring->getObject1() != NULL) ^ (spring->getObject2() != NULL)) {
        q_vec_type pos2;
        QString objId;
        if (spring->getObject1() != NULL) {
            spring->getObject1ConnectionPosition(pos1);
            spring->getEnd2WorldPosition(pos2);
            objId = "#" + objectIds.value(spring->getObject1());
        } else {
            spring->getObject2ConnectionPosition(pos1);
            spring->getEnd1WorldPosition(pos2);
            objId = "#" + objectIds.value(spring->getObject2());
        }
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(SPRING_OBJECT_END_ELEMENT_NAME);
        child->SetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME,
                    objId.toStdString().c_str());
        setPreciseVectorAttribute(child,pos1,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
        element->AddNestedElement(child);
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(SPRING_POINT_END_ELEMENT_NAME);
        setPreciseVectorAttribute(child,pos2,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
        element->AddNestedElement(child);
    }
    return element;
}

vtkXMLDataElement *ProjectToXML::transformOpListToXML(const QVector<QSharedPointer<TransformEquals> > *ops,
                                                   const QHash<const SketchObject *, QString> &objectIds)
{
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(TRANSFORM_OP_LIST_ELEMENT_NAME);

    for (int i = 0; i < ops->size(); i++) {
        QSharedPointer<TransformEquals> op(ops->at(i));
        if (!op) continue;
        vtkSmartPointer<vtkXMLDataElement> child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(TRANSFORM_OP_ELEMENT_NAME);
        const QVector<ObjectPair> *v = op->getPairsList();
        for (int j = 0; j < v->size(); j++) {
            vtkSmartPointer<vtkXMLDataElement> pair = vtkSmartPointer<vtkXMLDataElement>::New();
            pair->SetName(TRANSFORM_OP_PAIR_ELEMENT_NAME);
            pair->SetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME,
                               ("#" + objectIds.value(v->at(j).o1)).toStdString().c_str());
            pair->SetAttribute(TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME,
                               ("#" + objectIds.value(v->at(j).o2)).toStdString().c_str());
            child->AddNestedElement(pair);
        }
        element->AddNestedElement(child);
    }
    return element;
}

ProjectToXML::XML_Read_Status ProjectToXML::convertToCurrent(vtkXMLDataElement *root) {
    if (QString(root->GetName()) == QString(ROOT_ELEMENT_NAME)) {
        const char *fV = root->GetAttribute(VERSION_ATTRIBUTE_NAME);
        QString fileVersion(fV);
        QString readerVersion = SAVE_VERSION_NUM;
        if (fileVersion != readerVersion) {
            bool ok = false;
            int fileMajor = fileVersion.left(fileVersion.indexOf(QString("."))).toInt(&ok);
            if (!ok) {
                // failed to parse file version
                return XML_TO_DATA_FAILURE;
            }
            int readerMajor = SAVE_MAJOR_VERSION;
            if (fileMajor != readerMajor) {
                // if the major version changed, incompatible so give up
                return XML_TO_DATA_FAILURE;
            }
            int fileMinor = fileVersion.mid(fileVersion.indexOf(QString("."))+1).toInt(&ok);
            if (!ok) {
                // failed to parse file version
                return XML_TO_DATA_FAILURE;
            }
            int readerMinor = SAVE_MINOR_VERSION;
            if (readerMinor < fileMinor) {
                // old reader cannot read new file
                return XML_TO_DATA_FAILURE;
            } else if (readerMinor == fileMinor) {
                // shouldn't get here, but check anyway.  No need to do extra parsing on files
                // that are the right version
                return XML_TO_DATA_SUCCESS;
            } else {
                // if the reader is newer than the save version, then convert the file to the current
                // format (assuming default values for new variables)
                return convertToCurrentVersion(root,fileMinor);
            }
        } else {
            // versions match so we are ok
            return XML_TO_DATA_SUCCESS;
        }
    } else {
        // not valid xml, so can't do anything
        return XML_TO_DATA_FAILURE;
    }
}

ProjectToXML::XML_Read_Status ProjectToXML::convertToCurrentVersion(vtkXMLDataElement *root,int minorVersion) {
    switch(minorVersion) {
    case 0: // in 1 we added object groups... objects need numInstances parameter: default to 1
    {
        vtkXMLDataElement *objList = root->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
        if (objList == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        for (int i = 0; i < objList->GetNumberOfNestedElements(); i++) {
            vtkXMLDataElement *obj = objList->GetNestedElement(i);
            obj->SetAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,"1");
        }
    }
    case 1: // in 2 we added a transform op list that is assumed to be present.  So add an empty one.
    {
        vtkSmartPointer<vtkXMLDataElement> transOpList = vtkSmartPointer<vtkXMLDataElement>::New();
        transOpList->SetName(TRANSFORM_OP_LIST_ELEMENT_NAME);
        root->AddNestedElement(transOpList);
    }
    case 2: // in 3 we changed how the camera works and must update the transformation matrices that are saved
        // notably the room to eye matrix
    {
        vtkXMLDataElement *transforms = root->FindNestedElementWithName(TRANSFORM_MANAGER_ELEMENT_NAME);
        if (transforms == NULL) return XML_TO_DATA_FAILURE;
        vtkXMLDataElement *roomToEye = transforms->FindNestedElementWithName(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME);
        if (roomToEye == NULL) return XML_TO_DATA_FAILURE;
        transforms->RemoveNestedElement(roomToEye);  // This takes care of deleting it, no need to do it explicitly
        roomToEye = NULL; // just delete the old one
        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
        trans->Identity();
        trans->RotateWXYZ(180,0,1,0);
        trans->Translate(0,0,-STARTING_CAMERA_POSITION);
        roomToEye = matrixToXML(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME,trans->GetMatrix());
        // create a new one with the correct room to eye matrix and add it
        transforms->AddNestedElement(roomToEye);
        roomToEye->Delete();
    }
    case 3: // in 4 we completely changed how models are saved (and stored in the program)
        // rewrite all models
    {
        vtkXMLDataElement *modelMgr =
                root->FindNestedElementWithName(MODEL_MANAGER_ELEMENT_NAME);
        if (modelMgr == NULL) return XML_TO_DATA_FAILURE;
        for (int i = 0; i < modelMgr->GetNumberOfNestedElements(); i++)
        {
            vtkXMLDataElement *model = modelMgr->GetNestedElement(i);
            if (QString(model->GetName()) == QString(MODEL_ELEMENT_NAME))
            {
                model->SetIntAttribute(MODEL_NUM_CONFORMATIONS_ATTR_NAME,1);
                vtkXMLDataElement *transform =
                        model->FindNestedElementWithName(TRANSFORM_ELEMENT_NAME);
                if (transform == NULL) return XML_TO_DATA_FAILURE;
                // even though we are taking it out, it should have been there
                // if it wasn't there, the file was corrupted
                model->RemoveNestedElement(transform);
                vtkSmartPointer< vtkXMLDataElement > props =
                        model->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
                props->SetAttribute(MODEL_ROTATE_ENABLED_ATTR_NAME,"false");
                vtkSmartPointer< vtkXMLDataElement > source =
                        model->FindNestedElementWithName(MODEL_SOURCE_ELEMENT_NAME);
                // if source is null, this is doomed.  Source's information has to
                // be formatted slightly differently in the new version.
                if (source == NULL) return XML_TO_DATA_FAILURE;
                const char *src = source->GetCharacterData();
                if (QString(src).startsWith("VTK"))
                {
                    modelMgr->RemoveNestedElement(model);
                    i--;
                }
                else
                {
                    model->RemoveNestedElement(source);
                    vtkSmartPointer< vtkXMLDataElement > conformationElt
                            = vtkSmartPointer< vtkXMLDataElement >::New();
                    conformationElt->SetName(MODEL_CONFORMATION_ELEMENT_NAME);
                    conformationElt->SetIntAttribute(MODEL_CONFORMATION_NUMBER_ATTR_NAME,0);
                    conformationElt->AddNestedElement(source);
                    vtkSmartPointer< vtkXMLDataElement > resElt
                            = vtkSmartPointer< vtkXMLDataElement >::New();
                    resElt->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
                    resElt->SetAttribute(ID_ATTRIBUTE_NAME,
                                         getResolutionString(ModelResolution::FULL_RESOLUTION));
                    resElt->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                                         src);
                    conformationElt->AddNestedElement(resElt);
                    model->AddNestedElement(conformationElt);
                    vtkXMLDataElement *props =
                            model->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
                    props->SetAttribute(MODEL_ROTATE_ENABLED_ATTR_NAME, "false");
                }
            }
        }

    }
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToProject(SketchProject *proj, vtkXMLDataElement *elem) {
    if (QString(elem->GetName()) == QString(ROOT_ELEMENT_NAME)) {
        if (convertToCurrent(elem) == XML_TO_DATA_FAILURE) {
            // if failed to convert file to readable format version, exit we cannot do any more
            return XML_TO_DATA_FAILURE;
        }
        vtkXMLDataElement *models = elem->FindNestedElementWithName(MODEL_MANAGER_ELEMENT_NAME);
        if (models == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        QHash<QString,SketchModel *> modelIds;
        if (xmlToModelManager(proj,models,modelIds) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
        }
        vtkXMLDataElement *view = elem->FindNestedElementWithName(TRANSFORM_MANAGER_ELEMENT_NAME);
        if (view == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        if (xmlToTransforms(proj,view) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
        }
        QHash<QString,SketchObject *> objectIds;
        vtkXMLDataElement *objs = elem->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
        if (objs == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        if (xmlToObjectList(proj,objs,modelIds,objectIds) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
        }
        vtkXMLDataElement *reps = elem->FindNestedElementWithName(REPLICATOR_LIST_ELEMENT_NAME);
        if (reps == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        if (xmlToReplicatorList(proj,reps,objectIds) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
        }
        vtkXMLDataElement *springs = elem->FindNestedElementWithName(SPRING_LIST_ELEMENT_NAME);
        if (springs == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        if (xmlToSpringList(proj,springs,objectIds) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
        }
        vtkXMLDataElement *transformOps = elem->FindNestedElementWithName(TRANSFORM_OP_LIST_ELEMENT_NAME);
        if (transformOps == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        if (xmlToTransformOpList(proj,transformOps,objectIds) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
        }
    } else {
        return XML_TO_DATA_FAILURE;
    }
    return XML_TO_DATA_SUCCESS;
}


ProjectToXML::XML_Read_Status ProjectToXML::xmlToModelManager(SketchProject *proj, vtkXMLDataElement *elem, QHash<QString,SketchModel *> &modelIds) {
    if (QString(elem->GetName()) != QString(MODEL_MANAGER_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    for(int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        if (QString(child->GetName()) == QString(MODEL_ELEMENT_NAME)) {
            if (xmlToModel(proj,child,modelIds) != XML_TO_DATA_SUCCESS) {
                return XML_TO_DATA_FAILURE;
            }
        }
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToModel(SketchProject *proj, vtkXMLDataElement *elem, QHash<QString,SketchModel *> &modelIds) {
    if (QString(elem->GetName()) != QString(MODEL_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    // need addmodel method on project...
    QString id;
    int numConformations;
    double invMass, invMoment;
    bool shouldRotate;
    id = elem->GetAttribute(ID_ATTRIBUTE_NAME);
    if (id == NULL) return XML_TO_DATA_FAILURE;
    if (! elem->GetScalarAttribute(MODEL_NUM_CONFORMATIONS_ATTR_NAME,numConformations))
        return XML_TO_DATA_FAILURE;

    vtkXMLDataElement *props = elem->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
    if (props == NULL) return XML_TO_DATA_FAILURE;
    if (! props->GetScalarAttribute(MODEL_IMASS_ATTRIBUTE_NAME,invMass) )
        return XML_TO_DATA_FAILURE;
    if (! props->GetScalarAttribute(MODEL_IMOMENT_ATTRIBUTE_NAME,invMoment) )
        return XML_TO_DATA_FAILURE;
    const char *c = props->GetAttribute(MODEL_ROTATE_ENABLED_ATTR_NAME);
    if (c == NULL) return XML_TO_DATA_FAILURE;
    if (QString(c).toLower() == QString("true"))
        shouldRotate = true;
    else
        shouldRotate = false;

    QScopedPointer< SketchModel > model(new SketchModel(invMass, invMoment, shouldRotate));

    int conformations = 0;
    for (int i = 0; i < numConformations; i++)
    {
        // the relationship between conformation numbers and conformations needs
        // to be kept since only the conformation number is saved with each object
        vtkXMLDataElement *conf = elem->FindNestedElementWithNameAndAttribute(
                    MODEL_CONFORMATION_ELEMENT_NAME,
                    MODEL_CONFORMATION_NUMBER_ATTR_NAME,
                    QString::number(i).toStdString().c_str()
                    );
        if (conf == NULL) return XML_TO_DATA_FAILURE;
        conformations++;
        vtkXMLDataElement *src =
                conf->FindNestedElementWithName(MODEL_SOURCE_ELEMENT_NAME);
        if (src == NULL) return XML_TO_DATA_FAILURE;
        const char *srcString = src->GetCharacterData();
        if (srcString == NULL) return XML_TO_DATA_FAILURE;

        vtkXMLDataElement *fullRes =
                conf->FindNestedElementWithNameAndAttribute(
                    MODEL_RESOLUTION_ELEMENT_NAME,
                    ID_ATTRIBUTE_NAME,
                    getResolutionString(ModelResolution::FULL_RESOLUTION));
        if (fullRes == NULL) return XML_TO_DATA_FAILURE;
        const char *fileString = fullRes->GetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME);
        if (fileString == NULL) return XML_TO_DATA_FAILURE;

        QString filename = proj->getProjectDir() + "/" + fileString;
        QString source = srcString;
        if (source == CAMERA_MODEL_KEY) {
            model.reset(proj->getCameraModel());
            break;
        } else {
            model->addConformation(source,filename);
        }

        for (int j = 0; j < conf->GetNumberOfNestedElements(); j++)
        {
            vtkXMLDataElement *elt = conf->GetNestedElement(j);
            if (elt != src && elt != fullRes &&
                    elt->GetName() == QString(MODEL_RESOLUTION_ELEMENT_NAME))
            {
                const char *f = elt->GetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME);
                if (f == NULL) return XML_TO_DATA_FAILURE;
                const char *r = elt->GetAttribute(ID_ATTRIBUTE_NAME);
                if (r == NULL) return XML_TO_DATA_FAILURE;
                ModelResolution::ResolutionType res = getResolutionTypeFromId(r);
                QString fName = proj->getProjectDir() + "/" + f;
                model->addSurfaceFileForResolution(model->getNumberOfConformations()-1,
                                                   res,fName);
            }
        }
    }

    if (model.data() != NULL && conformations == numConformations)
    {
        modelIds.insert("#" + id,model.data());
    }
    else
    {
        return XML_TO_DATA_FAILURE;
    }

    if (model.data() != proj->getCameraModel())
    {
        proj->addModel(model.data());
    }
    model.take();
    return XML_TO_DATA_SUCCESS;
}

inline void matrixFromDoubleArray(vtkMatrix4x4 *mat, double *data) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mat->SetElement(i,j,data[i*4+j]);
        }
    }
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToTransforms(SketchProject *proj, vtkXMLDataElement *elem) {
    if (QString(elem->GetName()) != TRANSFORM_MANAGER_ELEMENT_NAME) {
        return XML_TO_DATA_FAILURE;
    }
    double roomWorld[16];
    double roomEye[16];
    vtkXMLDataElement *rtW = elem->FindNestedElementWithName(TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME);
    if (rtW == NULL) {
        return XML_TO_DATA_FAILURE;
    }
    int err = rtW->GetVectorAttribute(TRANSFORM_MATRIX_ATTRIBUTE_NAME,16,roomWorld);
    if (err != 16) {
        return XML_TO_DATA_FAILURE;
    }
    vtkXMLDataElement *rtE = elem->FindNestedElementWithName(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME);
    if (rtE == NULL) {
        return XML_TO_DATA_FAILURE;
    }
    err = rtE->GetVectorAttribute(TRANSFORM_MATRIX_ATTRIBUTE_NAME,16,roomEye);
    if (err != 16) {
        return XML_TO_DATA_FAILURE;
    }
    vtkSmartPointer<vtkMatrix4x4> rwMat = vtkSmartPointer<vtkMatrix4x4>::New();
    matrixFromDoubleArray(rwMat,roomWorld);
    vtkSmartPointer<vtkMatrix4x4> reMat = vtkSmartPointer<vtkMatrix4x4>::New();
    matrixFromDoubleArray(reMat,roomEye);
    proj->setViewpoint(rwMat,reMat);
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::readObjectList(QList<SketchObject *> &list,
                                                            vtkXMLDataElement *elem,
                                                            QHash<QString, SketchModel *> &modelIds,
                                                            QHash<QString, SketchObject *> &objectIds) {
    if (QString(elem->GetName()) != QString(OBJECTLIST_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    if (!list.empty()) { // if the list is already populated, give up.
        return XML_TO_DATA_FAILURE;
    }
    // this function processes whole list not single object
    // need function that gets object and returns it for each object in list
    // this function needs to be changed to return list of objects for caller to do something with
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        SketchObject *object = readObject(child,modelIds,objectIds);
        if (object == NULL) {
            // if read failed, delete progress and return
            for (int i = 0; i < list.size(); i++) {
                delete list[i];
            }
            list.clear();
            return XML_TO_DATA_FAILURE;
        }
        list.append(object);
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::readKeyframe(SketchObject *object, vtkXMLDataElement *frame) {
    if (QString(OBJECT_KEYFRAME_ELEMENT_NAME) != QString(frame->GetName())) {
        return XML_TO_DATA_FAILURE;
    }
    q_vec_type pos;
    q_type orient;
    bool visA, active;
    double time;
    int numRead = 0;
    numRead = frame->GetScalarAttribute(OBJECT_KEYFRAME_TIME_ATTRIBUTE_NAME,time);
    if (numRead != 1) return XML_TO_DATA_FAILURE;// if frame has no time, then don't know what to do with it
    vtkXMLDataElement *kTrans = frame->FindNestedElementWithName(TRANSFORM_ELEMENT_NAME);
    if (kTrans == NULL) return XML_TO_DATA_FAILURE;// if no transform, fail
    numRead = kTrans->GetVectorAttribute(POSITION_ATTRIBUTE_NAME,3,pos);
    if (numRead != 3) return XML_TO_DATA_FAILURE;// position wrong length.. fail
    numRead = kTrans->GetVectorAttribute(ROTATION_ATTRIBUTE_NAME,4,orient);
    if (numRead != 4)  return XML_TO_DATA_FAILURE;// orientation wrong length... fail
    vtkXMLDataElement *properties = frame->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
    if (properties == NULL) return XML_TO_DATA_FAILURE; // if no visiblitly status.. fail
    // TODO ... once there in an interface for it on object, do something with visibility
    const char *c = properties->GetAttribute(OBJECT_KEYFRAME_VIS_AF_ATTR_NAME);
    if (c != NULL) {
        QString strA(c);
        if (strA.toLower() == QString("true")) {
            visA = true;
        } else if (strA.toLower() == QString("false")) {
            visA = false;
        } else {
            return XML_TO_DATA_FAILURE;
        }
    } else {
        return XML_TO_DATA_FAILURE;
    }
    c = properties->GetAttribute(OBJECT_KEYFRAME_ACTIVE_ATTR_NAME);
    if (c != NULL) {
        QString strA(c);
        if (strA.toLower() == QString("true")) {
            active = true;
        } else {
            active = false;
        }
    } else {
        return XML_TO_DATA_FAILURE;
    }
    object->setLastLocation(); // so we don't change the object's position
    object->setPosAndOrient(pos,orient);
    object->setIsVisible(visA);
    object->setActive(active);
    object->addKeyframeForCurrentLocation(time); // TODO -- once visibility is done, set it too.
    object->restoreToLastLocation(); // restore the object's position
    return XML_TO_DATA_SUCCESS;
}

SketchObject *ProjectToXML::readObject(vtkXMLDataElement *elem,
                                       QHash<QString, SketchModel *> &modelIds,
                                       QHash<QString, SketchObject *> &objectIds) {
    if (QString(elem->GetName()) == QString(OBJECT_ELEMENT_NAME)) {
        QString mId, oId;
        q_vec_type pos;
        q_type orient;
        bool visibility, active;
        oId = QString("#") + elem->GetAttribute(ID_ATTRIBUTE_NAME);
        int numInstances;
        if (!elem->GetScalarAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,numInstances)) {
            return NULL;
        }
        vtkXMLDataElement *props = elem->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
        if (props == NULL) {
            return NULL;
        }
        if (numInstances == 1) {
            const char *c = props->GetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME);
            if (c == NULL) {
                return NULL;
            }
            mId = c;
        } else {
            mId = "";
        }
        const char *c = props->GetAttribute(OBJECT_VISIBILITY_ATTRIBUTE_NAME);
        if (!c) {
            visibility = true; // default to visible
        } else if (QString(c) == QString("true")) {
            visibility = true;
        } else {
            visibility = false;
        }
        c = props->GetAttribute(OBJECT_ACTIVE_ATTRIBUTE_NAME);
        if (!c) {
            active = false;
        } else if (QString(c) == QString("true")) {
            active = true;
        } else {
            active = false;
        }
        vtkXMLDataElement *trans = elem->FindNestedElementWithName(TRANSFORM_ELEMENT_NAME);
        if (trans == NULL) {
            return NULL;
        }
        int err = trans->GetVectorAttribute(POSITION_ATTRIBUTE_NAME,3,pos);
        err = err + trans->GetVectorAttribute(ROTATION_ATTRIBUTE_NAME,4,orient);
        if (err != 7) {
            return NULL;
        }
        q_normalize(orient,orient);
        QScopedPointer<SketchObject> object(NULL);
        if (numInstances == 1) {
            // modelInstace
            object.reset(new ModelInstance(modelIds.value(mId)));
            object->setPosAndOrient(pos,orient);
        } else {
            // group
            QScopedPointer<ObjectGroup> group(new ObjectGroup());
            vtkXMLDataElement *childList = elem->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
            if (childList == NULL) {
                return NULL;
            }
            QList<SketchObject *> childObjects;
            if (readObjectList(childObjects,childList,modelIds,objectIds) == XML_TO_DATA_FAILURE) {
                return NULL;
            }
            for (int i = 0; i < childObjects.size(); i++) {
                group->addObject(childObjects[i]);
            }
            object.reset( group.take() );
            // since each object will be saved in its actual position and not its group relative position,
            // we can simply let addObject's averaging take care of the group position/orientation
        }
        vtkXMLDataElement *keyframes = elem->FindNestedElementWithName(OBJECT_KEYFRAME_LIST_ELEMENT_NAME);
        // keyframes list will only exist on objects that have keyframes so this being NULL simply
        // means that the object has no keyframes
        if (keyframes != NULL) { // if we have keyframes... read them in
            for (int i = 0; i < keyframes->GetNumberOfNestedElements(); i++) {
                vtkXMLDataElement *frame = keyframes->GetNestedElement(i);
                if (QString(OBJECT_KEYFRAME_ELEMENT_NAME) == QString(frame->GetName())) {
                    if (readKeyframe(object.data(),frame) == XML_TO_DATA_FAILURE) {
                        return NULL;
                    }
                } // in xml: ignore extra stuff
            }
        } // end if we have keyframes... not having keyframes is fine too, so don't fail in this case
        objectIds.insert(oId,object.data());
        object->setIsVisible(visibility); // set visibility after keyframes so frames can store visibility state
        object->setActive(active); // similar to reason above
        return object.take();
    }
    return NULL;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToObjectList(SketchProject *proj, vtkXMLDataElement *elem,
                                           QHash<QString,SketchModel *> &modelIds,
                                           QHash<QString,SketchObject *> &objectIds) {
    QList<SketchObject *> objects;
    if (readObjectList(objects,elem,modelIds,objectIds) == XML_TO_DATA_FAILURE) {
        return XML_TO_DATA_FAILURE;
    }
    for (int i = 0; i < objects.size(); i++) {
        proj->addObject(objects[i]);
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToReplicatorList(SketchProject *proj, vtkXMLDataElement *elem,
                                                                QHash<QString, SketchObject *> &objectIds) {
    if (QString(elem->GetName()) != QString(REPLICATOR_LIST_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        if (QString(child->GetName()) == QString(REPLICATOR_ELEMENT_NAME)) {
            QString obj1Id, obj2Id;
            int numReplicas;
            const char *c = child->GetAttribute(REPLICATOR_OBJECT1_ATTRIBUTE_NAME);
            if (c == NULL) {
                return XML_TO_DATA_FAILURE;
            }
            obj1Id = c;
            c = child->GetAttribute(REPLICATOR_OBJECT2_ATTRIBUTE_NAME);
            if (c == NULL) {
                return XML_TO_DATA_FAILURE;
            }
            obj2Id = c;
            int err = child->GetScalarAttribute(REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME,numReplicas);
            if (!err) {
                return XML_TO_DATA_FAILURE;
            }
            proj->addReplication(objectIds.value(obj1Id),objectIds.value(obj2Id),numReplicas);
            // Eventually if there is special data about each replica, read it in here and apply it
        }
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToSpringList(SketchProject *proj, vtkXMLDataElement *elem,
                                                            QHash<QString, SketchObject *> &objectIds) {
    if (QString(elem->GetName()) != QString(SPRING_LIST_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        if (QString(child->GetName()) == QString(SPRING_ELEMENT_NAME)) {
            if (xmlToSpring(proj,child,objectIds) != XML_TO_DATA_SUCCESS) {
                return XML_TO_DATA_FAILURE;
            }
        }
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToSpring(SketchProject *proj, vtkXMLDataElement *elem,
                                                        QHash<QString, SketchObject *> &objectIds) {
    if (QString(elem->GetName()) != QString(SPRING_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    QString obj1Id, obj2Id;
    int objCount = 0;
    bool seenWPoint = false;
    double k, minRLen, maxRLen;
    q_vec_type o1Pos, o2Pos, wPos;
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        QString name = child->GetName();
        if (name == QString(SPRING_OBJECT_END_ELEMENT_NAME)) {
            if (objCount == 0) {
                obj1Id = child->GetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME);
                int err = child->GetVectorAttribute(SPRING_CONNECTION_POINT_ATTRIBUTE_NAME,3,o1Pos);
                if (err != 3) {
                    return XML_TO_DATA_FAILURE;
                }
                if (obj1Id.length() > 0) {
                    objCount++;
                }
            } else if (objCount == 1) {
                obj2Id = child->GetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME);
                int err = child->GetVectorAttribute(SPRING_CONNECTION_POINT_ATTRIBUTE_NAME,3,o2Pos);
                if (err != 3) {
                    return XML_TO_DATA_FAILURE;
                }
                if (obj2Id.length() > 0) {
                    objCount++;
                }
            }
        } else if (name == QString(PROPERTIES_ELEMENT_NAME)) {
            int err = child->GetScalarAttribute(SPRING_STIFFNESS_ATTRIBUTE_NAME,k);
            err += child->GetScalarAttribute(SPRING_MIN_REST_ATTRIBUTE_NAME,minRLen);
            err += child->GetScalarAttribute(SPRING_MAX_REST_ATTRIBUTE_NAME,maxRLen);
            if (err != 3) {
                return XML_TO_DATA_FAILURE;
            }
        } else if (name == QString(SPRING_POINT_END_ELEMENT_NAME)) {
            int err = child->GetVectorAttribute(SPRING_CONNECTION_POINT_ATTRIBUTE_NAME,3,wPos);
            if (err != 3) {
                return XML_TO_DATA_FAILURE;
            }
            seenWPoint = true;
        }
    }
    SpringConnection *spring = NULL;
    if ((seenWPoint ? 1 : 0) + objCount != 2) {
        // we have too few or too many endpoints
        return XML_TO_DATA_FAILURE;
    } else if (seenWPoint) {
        spring = new SpringConnection(objectIds.value(obj1Id),NULL,minRLen,maxRLen,k,o1Pos,wPos);
        proj->addSpring(spring);
    } else if (objCount == 2) {
        spring = proj->addSpring(objectIds.value(obj1Id),
                                 objectIds.value(obj2Id),minRLen,maxRLen,k,o1Pos,o2Pos);
    } else {
        return XML_TO_DATA_FAILURE;
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToTransformOpList(SketchProject *proj, vtkXMLDataElement *elem,
                                            QHash<QString, SketchObject *> &objectIds)
{
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        if (QString(elem->GetNestedElement(i)->GetName()) == QString(TRANSFORM_OP_ELEMENT_NAME)) {
            if (xmlToTransformOp(proj,elem->GetNestedElement(i),objectIds) == XML_TO_DATA_FAILURE) {
                return XML_TO_DATA_FAILURE;
            }
        }
    }
    return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToTransformOp(SketchProject *proj, vtkXMLDataElement *elem,
                                            QHash<QString, SketchObject *> &objectIds)
{
    if (elem->GetNumberOfNestedElements() == 0) {
        return XML_TO_DATA_FAILURE;
    }
    vtkXMLDataElement *pair = elem->GetNestedElement(0);
    if (QString(pair->GetName()) != QString(TRANSFORM_OP_PAIR_ELEMENT_NAME) ||
            pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME) == NULL ||
            pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME) == NULL)
    {
        return XML_TO_DATA_FAILURE;
    }
    QString o1(pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME)),
               o2(pair->GetAttribute(TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME));
    QWeakPointer<TransformEquals> eq = proj->addTransformEquals(objectIds.value(o1), objectIds.value(o2));
    QSharedPointer<TransformEquals> eqS(eq);
    if (eqS) {
        for (int i = 1; i < elem->GetNumberOfNestedElements(); i++) {
            pair = elem->GetNestedElement(i);
            if (QString(pair->GetName()) == QString(TRANSFORM_OP_PAIR_ELEMENT_NAME)) {
                QString obj1(pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME));
                QString obj2(pair->GetAttribute(TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME));
                if (!objectIds.contains(obj1) || !objectIds.contains(obj2)) {
                    return XML_TO_DATA_FAILURE;
                }
                eqS->addPair(objectIds.value(obj1), objectIds.value(obj2));
            }
        }
        return XML_TO_DATA_SUCCESS;
    } else {
        return XML_TO_DATA_FAILURE;
    }
}
