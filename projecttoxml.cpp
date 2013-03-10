#include "projecttoxml.h"
#include <ios>
#include <sketchtests.h>


#define ID_ATTRIBUTE_NAME                       "id"
#define POSITION_ATTRIBUTE_NAME                 "position"
#define ROTATION_ATTRIBUTE_NAME                 "orientation"
#define PROPERTIES_ELEMENT_NAME                 "properties"
#define TRANSFORM_ELEMENT_NAME                  "transform"

#define ROOT_ELEMENT_NAME                       "sketchbio"
#define VERSION_ATTRIBUTE_NAME                          "version"
#define SAVE_MAJOR_VERSION                      1
#define SAVE_MINOR_VERSION                      1
#define SAVE_VERSION_NUM                        (QString::number(SAVE_MAJOR_VERSION) + "." + SAVE_MINOR_VERSION)

#define MODEL_MANAGER_ELEMENT_NAME              "models"

#define MODEL_ELEMENT_NAME                      "model"
#define MODEL_SOURCE_ELEMENT_NAME               "source"
#define MODEL_SCALE_ATTRIBUTE_NAME              "scale"
#define MODEL_TRANSLATE_ATTRIBUTE_NAME          "translate"
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
#define OBJECT_REPLICA_NUM_ATTRIBUTE_NAME       "replicaNum"

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

    return element;
}

vtkXMLDataElement *ProjectToXML::modelManagerToXML(const ModelManager *models, const QString &dir,
                                                   QHash<const SketchModel *, QString> &modelIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(MODEL_MANAGER_ELEMENT_NAME);
    modelIds.reserve(models->getNumberOfModels());

    int id= 0;

    QHashIterator<QString,SketchModel *> it = models->getModelIterator();
    while (it.hasNext()) {
        const SketchModel *model = it.next().value();
        QString idStr = QString("M%1").arg(id);
        vtkSmartPointer<vtkXMLDataElement> child = modelToXML(model, dir,idStr);
        element->AddNestedElement(child);
        child->Delete();
        modelIds.insert(model,idStr);
        id++;
    }

    return element;
}

vtkXMLDataElement *ProjectToXML::modelToXML(const SketchModel *model, const QString &dir, const QString &id) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(MODEL_ELEMENT_NAME);
    element->SetAttribute(ID_ATTRIBUTE_NAME,id.toStdString().c_str());

    // the source of the model TODO - fix filenames to relative, do something about vtk classes...
    vtkSmartPointer<vtkXMLDataElement> child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(MODEL_SOURCE_ELEMENT_NAME);
    QString source = model->getDataSource();
    if (source.indexOf(dir) != -1) {
        source = source.mid(source.indexOf(dir) + dir.length() + 1);
    }
    child->SetCharacterData(source.toStdString().c_str(),source.length());
    element->AddNestedElement(child);

    // physical properties (mass, etc..)
    child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(PROPERTIES_ELEMENT_NAME);
    double v = model->getInverseMass();
    setPreciseVectorAttribute(child,&v,1,MODEL_IMASS_ATTRIBUTE_NAME);
    v = model->getInverseMomentOfInertia();
    setPreciseVectorAttribute(child,&v,1,MODEL_IMOMENT_ATTRIBUTE_NAME);
    element->AddNestedElement(child);

    // transformations from source to model
    child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(TRANSFORM_ELEMENT_NAME);
    double translate[3];
    model->getTranslate(translate);
    setPreciseVectorAttribute(child,translate,3,MODEL_TRANSLATE_ATTRIBUTE_NAME);
    v = model->getScale();
    setPreciseVectorAttribute(child,&v,1,MODEL_SCALE_ATTRIBUTE_NAME);
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
            objectIds.insert(obj,idStr);
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
    element->SetIntAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,object->numInstances());

    vtkSmartPointer<vtkXMLDataElement> child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(PROPERTIES_ELEMENT_NAME);
    QString modelId = "#" + modelIds.constFind(object->getModel()).value();
    child->SetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME,modelId.toStdString().c_str());

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
        element->AddNestedElement(child);
        child->Delete();
    }
    return element;
}

vtkXMLDataElement *ProjectToXML::springToXML(const SpringConnection *spring, const QHash<const SketchObject *, QString> &objectIds) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(SPRING_ELEMENT_NAME);
    // object 1 connection
    vtkSmartPointer<vtkXMLDataElement> child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(SPRING_OBJECT_END_ELEMENT_NAME);
    child->SetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME,
                        ("#" + objectIds.value(spring->getObject1())).toStdString().c_str());
    q_vec_type pos1;
    spring->getObject1ConnectionPosition(pos1);
    setPreciseVectorAttribute(child,pos1,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
    element->AddNestedElement(child);
    // spring properties
    child = vtkSmartPointer<vtkXMLDataElement>::New();
    child->SetName(PROPERTIES_ELEMENT_NAME);
    double v = spring->getStiffness();
    setPreciseVectorAttribute(child,&v,1,SPRING_STIFFNESS_ATTRIBUTE_NAME);
    v = spring->getMinRestLength();
    setPreciseVectorAttribute(child,&v,1,SPRING_MIN_REST_ATTRIBUTE_NAME);
    v = spring->getMaxRestLength();
    setPreciseVectorAttribute(child,&v,1,SPRING_MAX_REST_ATTRIBUTE_NAME);
    element->AddNestedElement(child);
    // second end, depends on spring type
    const InterObjectSpring *ispring = dynamic_cast<const InterObjectSpring *>(spring);
    const ObjectPointSpring *pspring = dynamic_cast<const ObjectPointSpring *>(spring);
    if (ispring != NULL) {
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(SPRING_OBJECT_END_ELEMENT_NAME);
        child->SetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME,
                            ("#" +objectIds.value(ispring->getObject2())).toStdString().c_str());
        q_vec_type pos2;
        ispring->getObject2ConnectionPosition(pos2);
        setPreciseVectorAttribute(child,pos2,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
        element->AddNestedElement(child);
    } else if (pspring != NULL) {
        child = vtkSmartPointer<vtkXMLDataElement>::New();
        child->SetName(SPRING_POINT_END_ELEMENT_NAME);
        q_vec_type pos2;
        pspring->getEnd2WorldPosition(pos2);
        setPreciseVectorAttribute(child,pos2,3,SPRING_CONNECTION_POINT_ATTRIBUTE_NAME);
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
        vtkXMLDataElement *objList = root->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
        if (objList == NULL) {
            return XML_TO_DATA_FAILURE;
        }
        for (int i = 0; i < objList->GetNumberOfNestedElements(); i++) {
            vtkXMLDataElement *obj = objList->GetNestedElement(i);
            obj->SetAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,"1");
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
        // TODO do something with version... test if newer... something like that
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
    QString source, id;
    double invMass, invMoment, scale;
    q_vec_type trans; // calculated to center object... we really don't need it (yet)
    id = elem->GetAttribute(ID_ATTRIBUTE_NAME);
    if (id == NULL) {
        return XML_TO_DATA_FAILURE;
    }
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        if (QString(child->GetName()) == QString(MODEL_SOURCE_ELEMENT_NAME)) {
            source = child->GetCharacterData();
            if (source == NULL) {
                return XML_TO_DATA_FAILURE;
            }
            source = source.trimmed();
        } else if (QString(child->GetName()) == QString(TRANSFORM_ELEMENT_NAME)) {
            int err = child->GetVectorAttribute(MODEL_TRANSLATE_ATTRIBUTE_NAME,3,trans);
            if (err != 3) {
                return XML_TO_DATA_FAILURE;
            }
            if (!child->GetScalarAttribute(MODEL_SCALE_ATTRIBUTE_NAME,scale) )
                return XML_TO_DATA_FAILURE;
        } else if (QString(child->GetName()) == QString(PROPERTIES_ELEMENT_NAME)) {
            if (! child->GetScalarAttribute(MODEL_IMASS_ATTRIBUTE_NAME,invMass) )
                return XML_TO_DATA_FAILURE;
            if (! child->GetScalarAttribute(MODEL_IMOMENT_ATTRIBUTE_NAME,invMoment) )
                return XML_TO_DATA_FAILURE;
        }
    }
    SketchModel *model = NULL;
    QString src = source;
    if (!src.startsWith("VTK")) {
        // trans not used right now
        model = proj->addModelFromFile(proj->getProjectDir() + "/" + source,invMass,invMoment,scale);
    } else {
        return XML_TO_DATA_SUCCESS;
    }
    if (model != NULL) {
        modelIds.insert("#" + id,model);
        return XML_TO_DATA_SUCCESS;
    } else {
        return XML_TO_DATA_FAILURE;
    }
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

ProjectToXML::XML_Read_Status ProjectToXML::xmlToObjectList(SketchProject *proj, vtkXMLDataElement *elem,
                                                            QHash<QString, SketchModel *> &modelIds, QHash<QString, SketchObject *> &objectIds) {
    if (QString(elem->GetName()) != QString(OBJECTLIST_ELEMENT_NAME)) {
        return XML_TO_DATA_FAILURE;
    }
    // this function processes whole list not single object
    // need function that gets object and returns it for each object in list
    // this function needs to be changed to return list of objects for caller to do something with
    for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement *child = elem->GetNestedElement(i);
        if (QString(child->GetName()) == QString(OBJECT_ELEMENT_NAME)) {
            QString mId, oId;
            q_vec_type pos;
            q_type orient;
            oId = QString("#") + child->GetAttribute(ID_ATTRIBUTE_NAME);
            vtkXMLDataElement *props = child->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
            if (props == NULL) {
                return XML_TO_DATA_FAILURE;
            }
            const char *c = props->GetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME);
            if (c == NULL) {
                return XML_TO_DATA_FAILURE;
            }
            mId = c;
            vtkXMLDataElement *trans = child->FindNestedElementWithName(TRANSFORM_ELEMENT_NAME);
            if (trans == NULL) {
                return XML_TO_DATA_FAILURE;
            }
            int err = trans->GetVectorAttribute(POSITION_ATTRIBUTE_NAME,3,pos);
            err = err + trans->GetVectorAttribute(ROTATION_ATTRIBUTE_NAME,4,orient);
            if (err != 7) {
                return XML_TO_DATA_FAILURE;
            }
            q_normalize(orient,orient);
            SketchObject *obj = proj->addObject(modelIds.value(mId),pos,orient);
            objectIds.insert(oId,obj);
        }
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
            } else if (objCount == 1) {
                obj2Id = child->GetAttribute(SPRING_OBJECT_ID_ATTRIBUTE_NAME);
                int err = child->GetVectorAttribute(SPRING_CONNECTION_POINT_ATTRIBUTE_NAME,3,o2Pos);
                if (err != 3) {
                    return XML_TO_DATA_FAILURE;
                }
            }
            objCount++;
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
        spring = new ObjectPointSpring(objectIds.value(obj1Id),minRLen,maxRLen,k,o1Pos,wPos);
        proj->addSpring(spring);
    } else if (objCount == 2) {
        spring = proj->addSpring(objectIds.value(obj1Id),
                                 objectIds.value(obj2Id),minRLen,maxRLen,k,o1Pos,o2Pos);
    } else {
        return XML_TO_DATA_FAILURE;
    }
    return XML_TO_DATA_SUCCESS;
}
