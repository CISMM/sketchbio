#include "projecttoxml.h"

#define ID_ATTRIBUTE_NAME                       "id"

#define ROOT_ELEMENT_NAME                       "sketchbio"
#define VERSION_STRING                          "version"
#define SAVE_VERSION_NUM                        "1.0"

#define MODEL_MANAGER_ELEMENT_NAME              "models"

#define MODEL_ELEMENT_NAME                      "model"
#define MODEL_SOURCE_ELEMENT_NAME               "source"
#define MODEL_TRANSFORM_ELEMENT_NAME            "transform"
#define MODEL_SCALE_ATTRIBUTE_NAME              "scale"
#define MODEL_TRANSLATE_ATTRIBUTE_NAME          "translate"
#define MODEL_PHYSICAL_PROPERTIES_ELEMENT_NAME  "properties"
#define MODEL_IMASS_ATTRIBUTE_NAME              "inverseMass"
#define MODEL_IMOMENT_ATTRIBUTE_NAME            "inverseMomentOfInertia"

#define TRANSFORM_MANAGER_ELEMENT_NAME          "viewpoint"
#define TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME    "worldToRoom"
#define TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME      "roomToEye"
#define TRANSFORM_MATRIX_ATTRIBUTE_NAME         "matrix"

#define OBJECTLIST_ELEMENT_NAME                 "objectlist"
#define OBJECT_ELEMENT_NAME                     "object"
#define OBJECT_PROPERTIES_ELEMENT_NAME          "properties"
#define OBJECT_MODELID_ATTRIBUTE_NAME           "modelid"
#define OBJECT_PHYSICS_ENABLED_ATTRIBUTE_NAME   "physicsEnabled"
#define OBJECT_REPLICA_NUM_ATTRIBUTE_NAME       "replicaNum"
#define OBJECT_TRANSFORM_ELEMENT_NAME           "transform"
#define OBJECT_POSITION_ATTRIBUTE_NAME          "position"
#define OBJECT_ROTATION_ATTRIBUTE_NAME          "orientation"

#define REPLICATOR_LIST_ELEMENT_NAME            "replicatorList"
#define REPLICATOR_ELEMENT_NAME                 "replicator"
#define REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME     "numReplicas"
#define REPLICATOR_OBJECT1_ATTRIBUTE_NAME       "original1"
#define REPLICATOR_OBJECT2_ATTRIBUTE_NAME       "original2"

vtkXMLDataElement *projectToXML(const SketchProject *project) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(ROOT_ELEMENT_NAME);
    element->SetAttribute(VERSION_STRING,SAVE_VERSION_NUM);

    QHash<const SketchModel *, QString> modelIds;
    QHash<const SketchObject *, QString> objectIds;

    element->AddNestedElement(modelManagerToXML(project->getModelManager(),project->getProjectDir(),modelIds));
    element->AddNestedElement(transformManagerToXML(project->getTransformManager()));
    element->AddNestedElement(objectListToXML(project->getWorldManager(),modelIds,objectIds));
    element->AddNestedElement(replicatorListToXML(project->getReplicas(),modelIds,objectIds));

    return element;
}

vtkXMLDataElement *modelManagerToXML(const ModelManager *models, const QString &dir,
                                     QHash<const SketchModel *, QString> &modelIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(MODEL_MANAGER_ELEMENT_NAME);
    modelIds.reserve(models->getNumberOfModels());

    int id= 0;

    QHashIterator<QString,SketchModel *> it = models->getModelIterator();
    while (it.hasNext()) {
        const SketchModel *model = it.next().value();
        QString idStr = QString("M%1").arg(id);
        element->AddNestedElement(modelToXML(model, dir,idStr));
        modelIds.insert(model,idStr);
        id++;
    }

    return element;
}

vtkXMLDataElement *modelToXML(const SketchModel *model, const QString &dir, const QString &id) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(MODEL_ELEMENT_NAME);
    element->SetAttribute(ID_ATTRIBUTE_NAME,id.toStdString().c_str());

    // the source of the model TODO - fix filenames to relative, do something about vtk classes...
    vtkXMLDataElement *child = vtkXMLDataElement::New();
    child->SetName(MODEL_SOURCE_ELEMENT_NAME);
    QString source = model->getDataSource();
    if (source.indexOf(dir) != -1) {
        source = source.mid(source.indexOf(dir) + dir.length() + 1);
    }
    child->SetCharacterData(source.toStdString().c_str(),source.length());
    element->AddNestedElement(child);

    // physical properties (mass, etc..)
    child = vtkXMLDataElement::New();
    child->SetName(MODEL_PHYSICAL_PROPERTIES_ELEMENT_NAME);
    child->SetDoubleAttribute(MODEL_IMASS_ATTRIBUTE_NAME,model->getInverseMass());
    child->SetDoubleAttribute(MODEL_IMOMENT_ATTRIBUTE_NAME,model->getInverseMomentOfInertia());
    element->AddNestedElement(child);

    // transformations from source to model
    child = vtkXMLDataElement::New();
    child->SetName(MODEL_TRANSFORM_ELEMENT_NAME);
    double translate[3];
    model->getTranslate(translate);
    child->SetVectorAttribute(MODEL_TRANSLATE_ATTRIBUTE_NAME,3,translate);
    child->SetDoubleAttribute(MODEL_SCALE_ATTRIBUTE_NAME,model->getScale());
    element->AddNestedElement(child);

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
    child->SetVectorAttribute(TRANSFORM_MATRIX_ATTRIBUTE_NAME,16,mat);
    return child;
}

vtkXMLDataElement *transformManagerToXML(const TransformManager *transforms) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(TRANSFORM_MANAGER_ELEMENT_NAME);

    element->AddNestedElement(matrixToXML(TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME,
                                          transforms->getWorldToRoomMatrix()));
    element->AddNestedElement(matrixToXML(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME,
                                          transforms->getRoomToEyeMatrix()));

    return element;
}

vtkXMLDataElement *objectListToXML(const WorldManager *world,
                              const QHash<const SketchModel *, QString> &modelIds,
                              QHash<const SketchObject *, QString> &objectIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(OBJECTLIST_ELEMENT_NAME);
    for (QListIterator<SketchObject *> it = world->getObjectIterator(); it.hasNext();) {
        const SketchObject *obj = it.next();
        const ReplicatedObject *rObj = dynamic_cast<const ReplicatedObject *>(obj);
        if (rObj == NULL) {
            QString idStr = QString("O%1").arg(objectIds.size());
            element->AddNestedElement(objectToXML(obj,modelIds,idStr));
            objectIds.insert(obj,idStr);
        }
    }
    return element;
}

vtkXMLDataElement *objectToXML(const SketchObject *object,
                               const QHash<const SketchModel *,QString> &modelIds, const QString &id) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(OBJECT_ELEMENT_NAME);
    element->SetAttribute(ID_ATTRIBUTE_NAME,id.toStdString().c_str());

    vtkXMLDataElement *child = vtkXMLDataElement::New();
    child->SetName(OBJECT_PROPERTIES_ELEMENT_NAME);
    QString modelId = "#" + modelIds.constFind(object->getConstModel()).value();
    child->SetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME,modelId.toStdString().c_str());
    child->SetAttribute(OBJECT_PHYSICS_ENABLED_ATTRIBUTE_NAME,
                          QString(object->doPhysics()?"true":"false").toStdString().c_str());

    const ReplicatedObject *rObj = dynamic_cast<const ReplicatedObject *>(object);
    if (rObj != NULL) {
        child->SetIntAttribute(OBJECT_REPLICA_NUM_ATTRIBUTE_NAME,rObj->getReplicaNum());
    }
    element->AddNestedElement(child);

    child = vtkXMLDataElement::New();
    child->SetName(OBJECT_TRANSFORM_ELEMENT_NAME);
    q_vec_type pos;
    q_type orient;
    object->getPosition(pos);
    object->getOrientation(orient);
    child->SetVectorAttribute(OBJECT_POSITION_ATTRIBUTE_NAME,3,pos);
    child->SetVectorAttribute(OBJECT_ROTATION_ATTRIBUTE_NAME,4,orient);

    element->AddNestedElement(child);
    return element;
}
vtkXMLDataElement *replicatorListToXML(const QList<StructureReplicator *> *replicaList,
                                       const QHash<const SketchModel *, QString> &modelIds,
                                       QHash<const SketchObject *, QString> &objectIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(REPLICATOR_LIST_ELEMENT_NAME);
    for (QListIterator<StructureReplicator *> it(*replicaList); it.hasNext();) {
        StructureReplicator *rep = it.next();
        vtkXMLDataElement *repElement = vtkXMLDataElement::New();
        repElement->SetName(REPLICATOR_ELEMENT_NAME);
        repElement->SetAttribute(REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME,
                                 QString::number(rep->getNumShown()).toStdString().c_str());
        // i'm not checking contains, it had better be in there
        repElement->SetAttribute(REPLICATOR_OBJECT1_ATTRIBUTE_NAME,
                                 ("#" + objectIds.value(rep->getFirstObject())).toStdString().c_str());
        repElement->SetAttribute(REPLICATOR_OBJECT2_ATTRIBUTE_NAME,
                                 ("#" + objectIds.value(rep->getSecondObject())).toStdString().c_str());

        vtkXMLDataElement *list = vtkXMLDataElement::New();
        list->SetName(OBJECTLIST_ELEMENT_NAME);
        for (QListIterator<SketchObject *> tr = rep->getReplicaIterator(); tr.hasNext();) {
            const SketchObject *obj = tr.next();
            QString idStr = QString("O%1").arg(objectIds.size());
            list->AddNestedElement(objectToXML(obj,modelIds,idStr));
            objectIds.insert(obj,idStr);
        }
        repElement->AddNestedElement(list);

        element->AddNestedElement(repElement);
    }
    return element;
}
