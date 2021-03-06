#include "projecttoxml.h"

#include <ios>
#include <cstring>
using std::strcmp;

#include <vtkMatrix4x4.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

#include <QScopedPointer>
#include <QDebug>
#include <QFile>
#include <QDir>

#include <quazip/JlCompress.h>
#include <zlib.h>

#include <sketchioconstants.h>
#include <transformmanager.h>
#include <keyframe.h>
#include <sketchmodel.h>
#include <modelmanager.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <springconnection.h>
#include <measuringtape.h>
#include <worldmanager.h>
#include <structurereplicator.h>
#include <transformequals.h>
#include <sketchproject.h>
#include <sketchtests.h>

#define ID_ATTRIBUTE_NAME "id"
#define POSITION_ATTRIBUTE_NAME "position"
#define ROTATION_ATTRIBUTE_NAME "orientation"
#define PROPERTIES_ELEMENT_NAME "properties"
#define TRANSFORM_ELEMENT_NAME "transform"

#define ROOT_ELEMENT_NAME "sketchbio"
#define VERSION_ATTRIBUTE_NAME "version"
#define SAVE_MAJOR_VERSION 1
#define SAVE_MINOR_VERSION 5
#define SAVE_VERSION_NUM                       \
  (QString::number(SAVE_MAJOR_VERSION) + "." + \
   QString::number(SAVE_MINOR_VERSION))

#define MIN_LUMINANCE_ATTRIBUTE_NAME "minLuminance"
#define MAX_LUMINANCE_ATTRIBUTE_NAME "maxLuminance"

#define MODEL_MANAGER_ELEMENT_NAME "models"

#define MODEL_ELEMENT_NAME "model"
#define MODEL_NUM_CONFORMATIONS_ATTR_NAME "numConformations"
#define MODEL_CONFORMATION_ELEMENT_NAME "conformation"
#define MODEL_CONFORMATION_NUMBER_ATTR_NAME "confNum"
#define MODEL_RESOLUTION_ELEMENT_NAME "resolution"
#define MODEL_FILENAME_ATTRIBUTE_NAME "filename"
#define MODEL_SOURCE_ELEMENT_NAME "source"
#define MODEL_ROTATE_ENABLED_ATTR_NAME "shouldRotateModel"
#define MODEL_IMASS_ATTRIBUTE_NAME "inverseMass"
#define MODEL_IMOMENT_ATTRIBUTE_NAME "inverseMomentOfInertia"

#define TRANSFORM_MANAGER_ELEMENT_NAME "viewpoint"
#define TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME "worldToRoom"
#define TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME "roomToEye"
#define TRANSFORM_MATRIX_ATTRIBUTE_NAME "matrix"

#define OBJECTLIST_ELEMENT_NAME "objectlist"
#define OBJECT_ELEMENT_NAME "object"
#define OBJECT_MODELID_ATTRIBUTE_NAME "modelid"
#define OBJECT_COLOR_MAP_ATTRIBUTE_NAME "colorMap"
#define OBJECT_LUMINANCE_ATTRIBUTE_NAME "luminance"
#define OBJECT_ARRAY_TO_COLOR_BY_ATTR_NAME "arrayToColorBy"
#define OBJECT_MODEL_CONF_NUM_ATTR_NAME "conformation"
#define OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME "numInstances"
#define OBJECT_VISIBILITY_ATTRIBUTE_NAME "visibility"
#define OBJECT_ACTIVE_ATTRIBUTE_NAME "active"
#define OBJECT_REPLICA_NUM_ATTRIBUTE_NAME "replicaNum"
#define OBJECT_KEYFRAME_LIST_ELEMENT_NAME "keyframeList"
#define OBJECT_KEYFRAME_ELEMENT_NAME "keyframe"
#define OBJECT_KEYFRAME_TIME_ATTRIBUTE_NAME "time"
#define OBJECT_KEYFRAME_ABSTRFM_ELEMENT_NAME "absTransform"
#define OBJECT_KEYFRAME_VIS_AF_ATTR_NAME "visibility"
#define OBJECT_KEYFRAME_ACTIVE_ATTR_NAME "active"
#define OBJECT_KEYFRAME_LEVEL_ATTRIBUTE_NAME "level"
#define OBJECT_KEYFRAME_PARENT_ID_ATTRIBUTE_NAME "parentId"

#define REPLICATOR_LIST_ELEMENT_NAME "replicatorList"
#define REPLICATOR_ELEMENT_NAME "replicator"
#define REPLICAS_GROUP_ATTRIBUTE_NAME "replicaGroup"
#define REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME "numReplicas"
#define REPLICATOR_OBJECT1_ATTRIBUTE_NAME "original1"
#define REPLICATOR_OBJECT2_ATTRIBUTE_NAME "original2"
#define REPLICA_ID_ELEMENT_NAME "replica"
#define REPLICA_OBJECT_ID_ATTRIBUTE_NAME "objId"

// kept for backwards compatibility for use in converting old
// formats to current
#define SPRING_LIST_ELEMENT_NAME "springList"
#define SPRING_ELEMENT_NAME "spring"

#define CONNECTOR_LIST_ELEMENT_NAME "connectorList"
#define CONNECTOR_ELEMENT_NAME "connector"
#define CONNECTOR_OBJECT_END_ELEMENT_NAME "objConnection"
#define CONNECTOR_POINT_END_ELEMENT_NAME "pointConnection"
#define CONNECTOR_OBJECT_ID_ATTRIBUTE_NAME "objId"
#define CONNECTOR_CONNECTION_POINT_ATTR_NAME "connectionPoint"
#define CONNECTOR_ALPHA_ATTRIBUTE_NAME "alpha"
#define CONNECTOR_RADIUS_ATTRIBUTE_NAME "radius"
// fields unique to springs that connectors will not have
#define SPRING_STIFFNESS_ATTRIBUTE_NAME "stiffness"
#define SPRING_MIN_REST_ATTRIBUTE_NAME "minRestLen"
#define SPRING_MAX_REST_ATTRIBUTE_NAME "maxRestLen"
// unique to measuring tapes
#define MEASURING_TAPE_ATTRIBUTE_NAME "measruingTape"

#define TRANSFORM_OP_LIST_ELEMENT_NAME "transformOpList"
#define TRANSFORM_OP_ELEMENT_NAME "transformOp"
#define TRANSFORM_OP_PAIR_ELEMENT_NAME "objectPair"
#define TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME "firstObject"
#define TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME "secondObject"

inline void setPreciseVectorAttribute(vtkXMLDataElement* elem,
                                      const double* vect, int len,
                                      const char* attrName)
{
  QString data;
  for (int i = 0; i < len; i++) {
    int precision = (int)floor(0.5 + log(vect[i]) - log(Q_EPSILON));
    data = data + QString::number(vect[i], 'g', max(11, precision)) + " ";
  }
  elem->SetAttribute(attrName, data.trimmed().toStdString().c_str());
}

vtkXMLDataElement* ProjectToXML::projectToXML(const SketchBio::Project* project)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(ROOT_ELEMENT_NAME);
  element->SetAttribute(VERSION_ATTRIBUTE_NAME,
                        SAVE_VERSION_NUM.toStdString().c_str());
  double minLum = project->getWorldManager().getMinLuminance();
  setPreciseVectorAttribute(element, &minLum, 1, MIN_LUMINANCE_ATTRIBUTE_NAME);
  double maxLum = project->getWorldManager().getMaxLuminance();
  setPreciseVectorAttribute(element, &maxLum, 1, MAX_LUMINANCE_ATTRIBUTE_NAME);

  QHash< const SketchModel*, QString > modelIds;
  QHash< const SketchObject*, QString > objectIds;

  vtkSmartPointer< vtkXMLDataElement > child =
      vtkSmartPointer< vtkXMLDataElement >::Take(modelManagerToXML(
          project->getModelManager(), project->getProjectDir(), modelIds));
  element->AddNestedElement(child);
  child.TakeReference(transformManagerToXML(project->getTransformManager()));
  element->AddNestedElement(child);
  child.TakeReference(
      objectListToXML(project->getWorldManager(), modelIds, objectIds));
  element->AddNestedElement(child);
  child.TakeReference(replicatorListToXML(project->getCrystalByExamples(), objectIds));
  element->AddNestedElement(child);
  child.TakeReference(springListToXML(project->getWorldManager(), objectIds));
  element->AddNestedElement(child);
  child.TakeReference(
      transformOpListToXML(project->getTransformOps(), objectIds));
  element->AddNestedElement(child);

  return element;
}

vtkXMLDataElement* ProjectToXML::objectToClipboardXML(
    const SketchObject* object)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(ROOT_ELEMENT_NAME);
  element->SetAttribute(VERSION_ATTRIBUTE_NAME,
                        SAVE_VERSION_NUM.toStdString().c_str());

  QHash< const SketchModel*, QString > modelIds;
  QHash< const SketchObject*, QString > objectIds;

  vtkSmartPointer< vtkXMLDataElement > child =
      vtkSmartPointer< vtkXMLDataElement >::Take(modelsToXML(object, modelIds));
  element->AddNestedElement(child);
  child = vtkSmartPointer< vtkXMLDataElement >::New();
  child->SetName(OBJECTLIST_ELEMENT_NAME);
  vtkSmartPointer< vtkXMLDataElement > objectXML =
      vtkSmartPointer< vtkXMLDataElement >::Take(
          objectToXML(object, modelIds, objectIds, false));
  child->AddNestedElement(objectXML);
  element->AddNestedElement(child);
  return element;
}

vtkXMLDataElement* ProjectToXML::modelManagerToXML(
    const ModelManager &models, const QString& dir,
    QHash< const SketchModel*, QString >& modelIds)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(MODEL_MANAGER_ELEMENT_NAME);
  modelIds.reserve(models.getNumberOfModels());

  int id = 0;

  QVectorIterator< SketchModel* > it = models.getModelIterator();
  while (it.hasNext()) {
    const SketchModel* model = it.next();
    QString idStr = QString("M%1").arg(id);
    vtkSmartPointer< vtkXMLDataElement > child =
        vtkSmartPointer< vtkXMLDataElement >::Take(
            modelToXML(model, dir, idStr));
    element->AddNestedElement(child);
    modelIds.insert(model, idStr);
    id++;
  }

  return element;
}

vtkXMLDataElement* ProjectToXML::modelsToXML(
    const SketchObject* object, QHash< const SketchModel*, QString >& modelIds)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(MODEL_MANAGER_ELEMENT_NAME);

  if (object->numInstances() == 1) {
    const SketchModel* model = object->getModel();
    QString idStr = QString("M%1").arg(modelIds.size());
    QString dir = "";
    vtkSmartPointer< vtkXMLDataElement > child =
        vtkSmartPointer< vtkXMLDataElement >::Take(
            modelToXML(model, dir, idStr));
    if (!modelIds.contains(model)) {
      element->AddNestedElement(child);
      modelIds.insert(model, idStr);
    }
  } else {
    const QList< SketchObject* >* subObjs = object->getSubObjects();

    for (QListIterator< SketchObject* > it(*subObjs); it.hasNext();) {
      SketchObject* obj = it.next();
      if (!modelIds.contains(obj->getModel())) {
        vtkSmartPointer< vtkXMLDataElement > child =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                modelsToXML(obj, modelIds));
        for (int i = child->GetNumberOfNestedElements() - 1; i >= 0; i--) {
          vtkSmartPointer< vtkXMLDataElement > subObj =
              child->GetNestedElement(i);
          child->RemoveNestedElement(subObj);
          element->AddNestedElement(subObj);
        }
      }
    }
  }
  return element;
}

inline const char* getResolutionString(ModelResolution::ResolutionType res)
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
  else {
    std::cout << "ERROR: Unknown resolution type in model save." << std::endl;
    return NULL;
  }
}

inline ModelResolution::ResolutionType getResolutionTypeFromId(const char* r)
{
  if (strcmp(r, "full_resolution") == 0)
    return ModelResolution::FULL_RESOLUTION;
  if (strcmp(r, "simplified_full_resolution") == 0)
    return ModelResolution::SIMPLIFIED_FULL_RESOLUTION;
  if (strcmp(r, "5000_tris") == 0) return ModelResolution::SIMPLIFIED_5000;
  if (strcmp(r, "2000_tris") == 0) return ModelResolution::SIMPLIFIED_2000;
  if (strcmp(r, "1000_tris") == 0)
    return ModelResolution::SIMPLIFIED_1000;
  else {
    std::cout << "ERROR: Unknown resolution type in model load." << std::endl;
    throw "Failed to find resolution";
  }
}

vtkXMLDataElement* ProjectToXML::modelToXML(const SketchModel* model,
                                            const QString& dir,
                                            const QString& id)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(MODEL_ELEMENT_NAME);
  element->SetAttribute(ID_ATTRIBUTE_NAME, id.toStdString().c_str());
  element->SetIntAttribute(MODEL_NUM_CONFORMATIONS_ATTR_NAME,
                           model->getNumberOfConformations());
  vtkSmartPointer< vtkXMLDataElement > properties =
      vtkSmartPointer< vtkXMLDataElement >::New();
  properties->SetName(PROPERTIES_ELEMENT_NAME);
  double d = model->getInverseMass();
  setPreciseVectorAttribute(properties, &d, 1, MODEL_IMASS_ATTRIBUTE_NAME);
  d = model->getInverseMomentOfInertia();
  setPreciseVectorAttribute(properties, &d, 1, MODEL_IMOMENT_ATTRIBUTE_NAME);
  element->AddNestedElement(properties);

  for (int i = 0; i < model->getNumberOfConformations(); i++) {
    vtkSmartPointer< vtkXMLDataElement > conformationElt =
        vtkSmartPointer< vtkXMLDataElement >::New();
    conformationElt->SetName(MODEL_CONFORMATION_ELEMENT_NAME);
    conformationElt->SetIntAttribute(MODEL_CONFORMATION_NUMBER_ATTR_NAME, i);
    vtkSmartPointer< vtkXMLDataElement > child =
        vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(MODEL_SOURCE_ELEMENT_NAME);
    QString src = model->getSource(i);
    child->SetCharacterData(src.toStdString().c_str(), src.length() + 1);
    conformationElt->AddNestedElement(child);
    QString filename;
    filename = model->getFileNameFor(i, ModelResolution::FULL_RESOLUTION);
    if (dir.size() > 0 && filename.startsWith(dir)) {
      filename = filename.mid(dir.length() + 1);
    }
    child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
    child->SetAttribute(ID_ATTRIBUTE_NAME,
                        getResolutionString(ModelResolution::FULL_RESOLUTION));
    child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                        filename.toStdString().c_str());
    conformationElt->AddNestedElement(child);
    filename =
        model->getFileNameFor(i, ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
    if (filename.length() > 0) {
      if (dir.size() > 0 && filename.startsWith(dir)) {
        filename = filename.mid(dir.length() + 1);
      }
      child = vtkSmartPointer< vtkXMLDataElement >::New();
      child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
      child->SetAttribute(
          ID_ATTRIBUTE_NAME,
          getResolutionString(ModelResolution::SIMPLIFIED_FULL_RESOLUTION));
      child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                          filename.toStdString().c_str());
      conformationElt->AddNestedElement(child);
    }
    filename = model->getFileNameFor(i, ModelResolution::SIMPLIFIED_5000);
    if (filename.length() > 0) {
      if (dir.size() > 0 && filename.startsWith(dir)) {
        filename = filename.mid(dir.length() + 1);
      }
      child = vtkSmartPointer< vtkXMLDataElement >::New();
      child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
      child->SetAttribute(
          ID_ATTRIBUTE_NAME,
          getResolutionString(ModelResolution::SIMPLIFIED_5000));
      child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                          filename.toStdString().c_str());
      conformationElt->AddNestedElement(child);
    }
    filename = model->getFileNameFor(i, ModelResolution::SIMPLIFIED_2000);
    if (filename.length() > 0) {
      if (dir.size() > 0 && filename.startsWith(dir)) {
        filename = filename.mid(dir.length() + 1);
      }
      child = vtkSmartPointer< vtkXMLDataElement >::New();
      child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
      child->SetAttribute(
          ID_ATTRIBUTE_NAME,
          getResolutionString(ModelResolution::SIMPLIFIED_2000));
      child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                          filename.toStdString().c_str());
      conformationElt->AddNestedElement(child);
    }
    filename = model->getFileNameFor(i, ModelResolution::SIMPLIFIED_1000);
    if (filename.length() > 0) {
      if (dir.size() > 0 && filename.startsWith(dir)) {
        filename = filename.mid(dir.length() + 1);
      }
      child = vtkSmartPointer< vtkXMLDataElement >::New();
      child->SetName(MODEL_RESOLUTION_ELEMENT_NAME);
      child->SetAttribute(
          ID_ATTRIBUTE_NAME,
          getResolutionString(ModelResolution::SIMPLIFIED_1000));
      child->SetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME,
                          filename.toStdString().c_str());
      conformationElt->AddNestedElement(child);
    }
    element->AddNestedElement(conformationElt);
  }

  return element;
}

inline vtkXMLDataElement* matrixToXML(const char* elementName,
                                      const vtkMatrix4x4* matrix)
{
  double mat[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat[i * 4 + j] = matrix->GetElement(i, j);
    }
  }
  vtkXMLDataElement* child = vtkXMLDataElement::New();
  child->SetName(elementName);
  setPreciseVectorAttribute(child, mat, 16, TRANSFORM_MATRIX_ATTRIBUTE_NAME);
  return child;
}

vtkXMLDataElement* ProjectToXML::transformManagerToXML(
    const TransformManager &transforms)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(TRANSFORM_MANAGER_ELEMENT_NAME);

  vtkSmartPointer< vtkXMLDataElement > child =
      vtkSmartPointer< vtkXMLDataElement >::Take(
          matrixToXML(TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME,
                      transforms.getWorldToRoomMatrix()));
  element->AddNestedElement(child);
  child.TakeReference(matrixToXML(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME,
                                  transforms.getRoomToEyeMatrix()));
  element->AddNestedElement(child);

  return element;
}

vtkXMLDataElement* ProjectToXML::objectListToXML(
    const WorldManager &world,
    const QHash< const SketchModel*, QString >& modelIds,
    QHash< const SketchObject*, QString >& objectIds)
{
  return objectListToXML(world.getObjects(), modelIds, objectIds);
}

vtkXMLDataElement* ProjectToXML::objectListToXML(
    const QList< SketchObject* >* objectList,
    const QHash< const SketchModel*, QString >& modelIds,
    QHash< const SketchObject*, QString >& objectIds,
	bool saveKeyframes)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(OBJECTLIST_ELEMENT_NAME);
  for (QListIterator< SketchObject* > it(*objectList); it.hasNext();) {
    const SketchObject* obj = it.next();
    vtkSmartPointer< vtkXMLDataElement > child =
        vtkSmartPointer< vtkXMLDataElement >::Take(
            objectToXML(obj, modelIds, objectIds, saveKeyframes));
    element->AddNestedElement(child);
  }
  return element;
}

vtkXMLDataElement* ProjectToXML::objectToXML(
    const SketchObject* object,
    const QHash< const SketchModel*, QString >& modelIds,
    QHash< const SketchObject*, QString >& objectIds,
	bool saveKeyframes)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(OBJECT_ELEMENT_NAME);
  QString id;
  if (objectIds.contains(object)) {
	  id = objectIds.value(object);
  }
  else {
	  id = QString("O%1").arg(objectIds.size());
  }
  element->SetAttribute(ID_ATTRIBUTE_NAME, (id.toStdString()).c_str());
  // do this here now so object and its children don't have same id
  objectIds.insert(object, id);
  bool isGroup = object->numInstances() != 1;
  element->SetIntAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,
                           object->numInstances());

  vtkSmartPointer< vtkXMLDataElement > child =
      vtkSmartPointer< vtkXMLDataElement >::New();
  child->SetName(PROPERTIES_ELEMENT_NAME);
  if (!isGroup) {
    QString modelId("#");
    QHash< const SketchModel*, QString >::const_iterator itr =
        modelIds.constFind(object->getModel());
    if (itr == modelIds.constEnd())
      std::cout << "Error finding model." << std::endl;
    QString idx = itr.value();
    modelId = modelId.append(idx);
    child->SetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME,
                        modelId.toStdString().c_str());
    ColorMapType::Type cmap = object->getColorMapType();
    const QString& array = object->getArrayToColorBy();
    child->SetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME,
                        ColorMapType::stringFromColorMap(cmap));
    child->SetAttribute(OBJECT_ARRAY_TO_COLOR_BY_ATTR_NAME,
                        array.toStdString().c_str());
	double lum = object->getLuminance();
	setPreciseVectorAttribute(child, &lum, 1, OBJECT_LUMINANCE_ATTRIBUTE_NAME);
  }
  child->SetIntAttribute(OBJECT_MODEL_CONF_NUM_ATTR_NAME,
                         object->getModelConformation());
  // Current thoughts: collision groups should be recreated from
  //   effects placed on objects being recreated.
  //   There is no reason that they should need to be saved.
  child->SetAttribute(OBJECT_VISIBILITY_ATTRIBUTE_NAME,
                      object->isVisible() ? "true" : "false");
  child->SetAttribute(OBJECT_ACTIVE_ATTRIBUTE_NAME,
                      object->isActive() ? "true" : "false");

  element->AddNestedElement(child);

  child = vtkSmartPointer< vtkXMLDataElement >::New();
  child->SetName(TRANSFORM_ELEMENT_NAME);
  q_vec_type pos;
  q_type orient;
  object->getPosition(pos);
  object->getOrientation(orient);
  setPreciseVectorAttribute(child, pos, 3, POSITION_ATTRIBUTE_NAME);
  setPreciseVectorAttribute(child, orient, 4, ROTATION_ATTRIBUTE_NAME);
  element->AddNestedElement(child);

  if (saveKeyframes && object->getNumKeyframes() > 0) {
    // object has keyframes, save them
    child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(OBJECT_KEYFRAME_LIST_ELEMENT_NAME);
    const QMap< double, Keyframe >* keyframeMap = object->getKeyframes();
    QMapIterator< double, Keyframe > it(*keyframeMap);
    while (it.hasNext()) {
      double time = it.peekNext().key();
      const Keyframe& frame = it.next().value();
      q_vec_type p, pabs;
      q_type o, oabs;
      frame.getPosition(p);
      frame.getOrientation(o);
	  frame.getAbsolutePosition(pabs);
	  frame.getAbsoluteOrientation(oabs);
	  int level = frame.getLevel();
      vtkSmartPointer< vtkXMLDataElement > frameElement =
          vtkSmartPointer< vtkXMLDataElement >::New();
      frameElement->SetName(OBJECT_KEYFRAME_ELEMENT_NAME);
      setPreciseVectorAttribute(frameElement, &time, 1,
                                OBJECT_KEYFRAME_TIME_ATTRIBUTE_NAME);
      vtkSmartPointer< vtkXMLDataElement > tfrm =
          vtkSmartPointer< vtkXMLDataElement >::New();
      tfrm->SetName(TRANSFORM_ELEMENT_NAME);
      setPreciseVectorAttribute(tfrm, p, 3, POSITION_ATTRIBUTE_NAME);
      setPreciseVectorAttribute(tfrm, o, 4, ROTATION_ATTRIBUTE_NAME);
      frameElement->AddNestedElement(tfrm);
	  vtkSmartPointer< vtkXMLDataElement > absTfrm =
          vtkSmartPointer< vtkXMLDataElement >::New();
      absTfrm->SetName(OBJECT_KEYFRAME_ABSTRFM_ELEMENT_NAME);
      setPreciseVectorAttribute(absTfrm, pabs, 3, POSITION_ATTRIBUTE_NAME);
      setPreciseVectorAttribute(absTfrm, oabs, 4, ROTATION_ATTRIBUTE_NAME);
      frameElement->AddNestedElement(absTfrm);
      vtkSmartPointer< vtkXMLDataElement > visibility =
          vtkSmartPointer< vtkXMLDataElement >::New();
      visibility->SetName(PROPERTIES_ELEMENT_NAME);
      visibility->SetAttribute(OBJECT_KEYFRAME_VIS_AF_ATTR_NAME,
                               (frame.isVisibleAfter() ? "true" : "false"));
      visibility->SetAttribute(OBJECT_KEYFRAME_ACTIVE_ATTR_NAME,
                               (frame.isActive() ? "true" : "false"));
      frameElement->AddNestedElement(visibility);
	  frameElement->SetIntAttribute(OBJECT_KEYFRAME_LEVEL_ATTRIBUTE_NAME, level);
	  QString parentId;
	  SketchObject* parent = frame.getParent();
	  if (objectIds.contains(parent)) {
	      parentId = objectIds.value(parent);
	  }
	  else {
		  if (parent != NULL) {
			  parentId = QString("O%1").arg(objectIds.size());
		      objectIds.insert(parent, parentId);
		  }
		  else {
		    parentId = "NULL";
		  }
	  }
	  frameElement->SetAttribute(OBJECT_KEYFRAME_PARENT_ID_ATTRIBUTE_NAME,
								("#" + parentId.toStdString()).c_str());
      if (object->numInstances() == 1) {
        frameElement->SetAttribute(
            OBJECT_COLOR_MAP_ATTRIBUTE_NAME,
            ColorMapType::stringFromColorMap(frame.getColorMapType()));
        frameElement->SetAttribute(
            OBJECT_ARRAY_TO_COLOR_BY_ATTR_NAME,
            frame.getArrayToColorBy().toStdString().c_str());
      }
      child->AddNestedElement(frameElement);
    }
    element->AddNestedElement(child);
  }

  if (object->numInstances() != 1 && object->numInstances() != 0) {
    vtkSmartPointer< vtkXMLDataElement > list =
        vtkSmartPointer< vtkXMLDataElement >::Take(
            ProjectToXML::objectListToXML(object->getSubObjects(), modelIds,
                                          objectIds, saveKeyframes));
    element->AddNestedElement(list);
  }

  return element;
}

vtkXMLDataElement* ProjectToXML::replicatorListToXML(
    const QList< StructureReplicator* > &replicaList,
    QHash< const SketchObject*, QString >& objectIds)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(REPLICATOR_LIST_ELEMENT_NAME);
  for (QListIterator< StructureReplicator* > it(replicaList); it.hasNext();) {
    StructureReplicator* rep = it.next();
    vtkSmartPointer< vtkXMLDataElement > repElement =
        vtkSmartPointer< vtkXMLDataElement >::New();
    repElement->SetName(REPLICATOR_ELEMENT_NAME);
    repElement->SetAttribute(
        REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME,
        QString::number(rep->getNumShown()).toStdString().c_str());
    // i'm not checking contains, it had better be in there
    repElement->SetAttribute(
        REPLICATOR_OBJECT1_ATTRIBUTE_NAME,
        ("#" + objectIds.value(rep->getFirstObject())).toStdString().c_str());
    repElement->SetAttribute(
        REPLICATOR_OBJECT2_ATTRIBUTE_NAME,
        ("#" + objectIds.value(rep->getSecondObject())).toStdString().c_str());
    repElement->SetAttribute(
        REPLICAS_GROUP_ATTRIBUTE_NAME,
		("#" + objectIds.value(rep->getReplicaGroup())).toStdString().c_str());
    for (QListIterator< SketchObject* > itr(rep->getReplicaIterator());
         itr.hasNext();) {
      vtkSmartPointer< vtkXMLDataElement > replicaElt =
          vtkSmartPointer< vtkXMLDataElement >::New();
      SketchObject* replica = itr.next();
      replicaElt->SetName(REPLICA_ID_ELEMENT_NAME);
      replicaElt->SetAttribute(
          REPLICA_OBJECT_ID_ATTRIBUTE_NAME,
		  ("#" + objectIds.value(replica)).toStdString().c_str());
      repElement->AddNestedElement(replicaElt);
    }

    element->AddNestedElement(repElement);
  }
  return element;
}

vtkXMLDataElement* ProjectToXML::springListToXML(
    const WorldManager &world,
    const QHash< const SketchObject*, QString >& objectIds)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(CONNECTOR_LIST_ELEMENT_NAME);
  for (QListIterator< Connector* > it = world.getSpringsIterator();
       it.hasNext();) {
    const Connector* conn = it.next();
    vtkSmartPointer< vtkXMLDataElement > child =
        vtkSmartPointer< vtkXMLDataElement >::Take(
            springToXML(conn, objectIds));
    element->AddNestedElement(child);
  }
  return element;
}

vtkXMLDataElement* ProjectToXML::springToXML(
    const Connector* conn,
    const QHash< const SketchObject*, QString >& objectIds)
{
  const MeasuringTape* tape =
	  dynamic_cast< const MeasuringTape* >(conn);
  if (conn->getObject1() == conn->getObject2()) {
    // allows measuring tapes to be saved even when not connected
	// to any objects, but not springs or transparent connectors
	if (!(tape != NULL && conn->getObject1() == NULL)) {
		return NULL;
	}
  }
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(CONNECTOR_ELEMENT_NAME);
  vtkSmartPointer< vtkXMLDataElement > child =
      vtkSmartPointer< vtkXMLDataElement >::New();
  q_vec_type pos1;
  // conn properties
  child->SetName(PROPERTIES_ELEMENT_NAME);
  const SpringConnection* spring =
      dynamic_cast< const SpringConnection* >(conn);
  double v = conn->getAlpha();
  setPreciseVectorAttribute(child, &v, 1, CONNECTOR_ALPHA_ATTRIBUTE_NAME);
  v = conn->getRadius();
  setPreciseVectorAttribute(child, &v, 1, CONNECTOR_RADIUS_ATTRIBUTE_NAME);
  ColorMapType::Type cmap = conn->getColorMapType();
  child->SetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME,
                      ColorMapType::stringFromColorMap(cmap));
  if (spring != NULL) {
    v = spring->getStiffness();
    setPreciseVectorAttribute(child, &v, 1, SPRING_STIFFNESS_ATTRIBUTE_NAME);
    v = spring->getMinRestLength();
    setPreciseVectorAttribute(child, &v, 1, SPRING_MIN_REST_ATTRIBUTE_NAME);
    v = spring->getMaxRestLength();
    setPreciseVectorAttribute(child, &v, 1, SPRING_MAX_REST_ATTRIBUTE_NAME);
  }
  if (tape != NULL) {
	  child->SetAttribute(MEASURING_TAPE_ATTRIBUTE_NAME, "true");
  }
  element->AddNestedElement(child);
  // second end, depends on conn type
  if (conn->getObject1() != NULL && conn->getObject2() != NULL) {
    child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(CONNECTOR_OBJECT_END_ELEMENT_NAME);
    child->SetAttribute(
        CONNECTOR_OBJECT_ID_ATTRIBUTE_NAME,
		("#" + objectIds.value(conn->getObject1())).toStdString().c_str());
    conn->getObject1ConnectionPosition(pos1);
    setPreciseVectorAttribute(child, pos1, 3,
                              CONNECTOR_CONNECTION_POINT_ATTR_NAME);
    element->AddNestedElement(child);
    child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(CONNECTOR_OBJECT_END_ELEMENT_NAME);
    child->SetAttribute(
        CONNECTOR_OBJECT_ID_ATTRIBUTE_NAME,
        ("#" + objectIds.value(conn->getObject2())).toStdString().c_str());
    q_vec_type pos2;
    conn->getObject2ConnectionPosition(pos2);
    setPreciseVectorAttribute(child, pos2, 3,
                              CONNECTOR_CONNECTION_POINT_ATTR_NAME);
    element->AddNestedElement(child);
  } else if ((conn->getObject1() != NULL) ^ (conn->getObject2() != NULL)) {
	q_vec_type pos2;
    QString objId;
    if (conn->getObject1() != NULL) {
      conn->getObject1ConnectionPosition(pos1);
      conn->getEnd2WorldPosition(pos2);
      objId = objectIds.value(conn->getObject1());
    } else {
      conn->getObject2ConnectionPosition(pos1);
      conn->getEnd1WorldPosition(pos2);
      objId = objectIds.value(conn->getObject2());
    }
    child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(CONNECTOR_OBJECT_END_ELEMENT_NAME);
    child->SetAttribute(CONNECTOR_OBJECT_ID_ATTRIBUTE_NAME,
                        objId.toStdString().c_str());
    setPreciseVectorAttribute(child, pos1, 3,
                              CONNECTOR_CONNECTION_POINT_ATTR_NAME);
    element->AddNestedElement(child);
    child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(CONNECTOR_POINT_END_ELEMENT_NAME);
    setPreciseVectorAttribute(child, pos2, 3,
                              CONNECTOR_CONNECTION_POINT_ATTR_NAME);
    element->AddNestedElement(child);
  } else {
    q_vec_type pos1, pos2;
	conn->getEnd1WorldPosition(pos1);
	conn->getEnd2WorldPosition(pos2);
	child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(CONNECTOR_POINT_END_ELEMENT_NAME);
    setPreciseVectorAttribute(child, pos1, 3,
                              CONNECTOR_CONNECTION_POINT_ATTR_NAME);
    element->AddNestedElement(child);
	 child = vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(CONNECTOR_POINT_END_ELEMENT_NAME);
    setPreciseVectorAttribute(child, pos2, 3,
                              CONNECTOR_CONNECTION_POINT_ATTR_NAME);
    element->AddNestedElement(child);
  }
  return element;
}

vtkXMLDataElement* ProjectToXML::transformOpListToXML(
    const QVector< QSharedPointer< TransformEquals > > &ops,
    const QHash< const SketchObject*, QString >& objectIds)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(TRANSFORM_OP_LIST_ELEMENT_NAME);

  for (int i = 0; i < ops.size(); i++) {
    QSharedPointer< TransformEquals > op(ops.at(i));
    if (!op) continue;
    vtkSmartPointer< vtkXMLDataElement > child =
        vtkSmartPointer< vtkXMLDataElement >::New();
    child->SetName(TRANSFORM_OP_ELEMENT_NAME);
    const QVector< ObjectPair >* v = op->getPairsList();
    for (int j = 0; j < v->size(); j++) {
      vtkSmartPointer< vtkXMLDataElement > pair =
          vtkSmartPointer< vtkXMLDataElement >::New();
      pair->SetName(TRANSFORM_OP_PAIR_ELEMENT_NAME);
      pair->SetAttribute(
          TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME,
          ("#" + objectIds.value(v->at(j).o1)).toStdString().c_str());
      pair->SetAttribute(
          TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME,
          ("#" + objectIds.value(v->at(j).o2)).toStdString().c_str());
      child->AddNestedElement(pair);
    }
    element->AddNestedElement(child);
  }
  return element;
}

ProjectToXML::XML_Read_Status ProjectToXML::convertToCurrent(
    vtkXMLDataElement* root)
{
  if (QString(root->GetName()) == QString(ROOT_ELEMENT_NAME)) {
    const char* fV = root->GetAttribute(VERSION_ATTRIBUTE_NAME);
    QString fileVersion(fV);
    QString readerVersion = SAVE_VERSION_NUM;
    if (fileVersion != readerVersion) {
      bool ok = false;
      int fileMajor =
          fileVersion.left(fileVersion.indexOf(QString("."))).toInt(&ok);
      if (!ok) {
        // failed to parse file version
        return XML_TO_DATA_FAILURE;
      }
      int readerMajor = SAVE_MAJOR_VERSION;
      if (fileMajor != readerMajor) {
        // if the major version changed, incompatible so give up
        return XML_TO_DATA_FAILURE;
      }
      int fileMinor =
          fileVersion.mid(fileVersion.indexOf(QString(".")) + 1).toInt(&ok);
      if (!ok) {
        // failed to parse file version
        return XML_TO_DATA_FAILURE;
      }
      int readerMinor = SAVE_MINOR_VERSION;
      if (readerMinor < fileMinor) {
        // old reader cannot read new file
        return XML_TO_DATA_FAILURE;
      } else if (readerMinor == fileMinor) {
        // shouldn't get here, but check anyway.  No need to do extra parsing on
        // files
        // that are the right version
        return XML_TO_DATA_SUCCESS;
      } else {
        // if the reader is newer than the save version, then convert the file
        // to the current
        // format (assuming default values for new variables)
        return convertToCurrentVersion(root, fileMinor);
      }
    } else {
      // versions match so we are ok
      return XML_TO_DATA_SUCCESS;
    }
  } else {
    // not valid SketchBio xml, so can't do anything
    return XML_TO_DATA_FAILURE;
  }
}

ProjectToXML::XML_Read_Status ProjectToXML::convertToCurrentVersion(
    vtkXMLDataElement* root, int minorVersion)
{
  switch (minorVersion) {
    case 0: {
      // Going from 0 to 1, the structure of StructureReplicator changed to use
      // a group
      // internally and the save state updated to match this.
      vtkXMLDataElement* replicators =
          root->FindNestedElementWithName(REPLICATOR_LIST_ELEMENT_NAME);
      if (replicators == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      vtkXMLDataElement* objects =
          root->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
      if (objects == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      for (int i = 0; i < replicators->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement* replicator = replicators->GetNestedElement(i);
        if (replicator->GetName() == QString(REPLICATOR_ELEMENT_NAME)) {
          vtkSmartPointer< vtkXMLDataElement > objList =
              replicator->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
          if (objList == NULL) {
            return XML_TO_DATA_FAILURE;
          }
          vtkSmartPointer< vtkXMLDataElement > group =
              vtkSmartPointer< vtkXMLDataElement >::New();
          group->SetName(OBJECT_ELEMENT_NAME);
          group->SetAttribute(ID_ATTRIBUTE_NAME,
								QString("R%1").arg(i).toStdString().c_str());
          group->SetIntAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,
                                 objList->GetNumberOfNestedElements());
          vtkSmartPointer< vtkXMLDataElement > child =
              vtkSmartPointer< vtkXMLDataElement >::New();
          child->SetName(PROPERTIES_ELEMENT_NAME);
          child->SetIntAttribute(OBJECT_MODEL_CONF_NUM_ATTR_NAME, -1);
          child->SetAttribute(OBJECT_VISIBILITY_ATTRIBUTE_NAME, "true");
          child->SetAttribute(OBJECT_ACTIVE_ATTRIBUTE_NAME, "false");
          group->AddNestedElement(child);
          child = vtkSmartPointer< vtkXMLDataElement >::New();
          child->SetName(TRANSFORM_ELEMENT_NAME);
          double data[4] = {0, 0, 0, 1};
          child->SetVectorAttribute(POSITION_ATTRIBUTE_NAME, 3, data);
          child->SetVectorAttribute(ROTATION_ATTRIBUTE_NAME, 4, data);
          group->AddNestedElement(child);
          replicator->RemoveNestedElement(objList);
          group->AddNestedElement(objList);
          replicator->SetAttribute(
              REPLICAS_GROUP_ATTRIBUTE_NAME,
			  QString("#R%1").arg(i).toStdString().c_str());
          objects->AddNestedElement(group);
        }
      }
    }
    case 1: {
      // Going from 1 to 2, in order to save correct info about all the replicas
      // such as color, we save the whole chain instead of just the bases and
      // group
      vtkXMLDataElement* replicators =
          root->FindNestedElementWithName(REPLICATOR_LIST_ELEMENT_NAME);
      if (replicators == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      vtkXMLDataElement* objects =
          root->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
      if (objects == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      for (int i = 0; i < replicators->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement* replicator = replicators->GetNestedElement(i);
        if (replicator->GetName() == QString(REPLICATOR_ELEMENT_NAME)) {
          const char* groupID =
              replicator->GetAttribute(REPLICAS_GROUP_ATTRIBUTE_NAME);
          if (groupID == NULL || groupID[0] != '#') {
            return XML_TO_DATA_FAILURE;
          }
          const char* obj1ID =
              replicator->GetAttribute(REPLICATOR_OBJECT1_ATTRIBUTE_NAME);
          if (obj1ID == NULL || obj1ID[0] != '#') {
            return XML_TO_DATA_FAILURE;
          }
          const char* obj2ID =
              replicator->GetAttribute(REPLICATOR_OBJECT2_ATTRIBUTE_NAME);
          if (obj2ID == NULL || obj2ID[0] != '#') {
            return XML_TO_DATA_FAILURE;
          }
          vtkXMLDataElement* group =
              objects->FindNestedElementWithNameAndAttribute(
                  OBJECT_ELEMENT_NAME, ID_ATTRIBUTE_NAME, &groupID[1]);
          if (group == NULL) {
            return XML_TO_DATA_FAILURE;
          }
          vtkXMLDataElement* list =
              group->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
          if (list == NULL) {
            return XML_TO_DATA_FAILURE;
          }
          for (int i = 0; i < list->GetNumberOfNestedElements(); i++) {
            vtkXMLDataElement* object = list->GetNestedElement(i);
            if (strcmp(object->GetName(), OBJECT_ELEMENT_NAME) == 0) {
              const char* objID = object->GetAttribute(ID_ATTRIBUTE_NAME);
              if (objID == NULL) {
                return XML_TO_DATA_FAILURE;
              }
              if (strcmp(objID, &obj1ID[1]) != 0 &&
                  strcmp(objID, &obj2ID[1]) != 0) {
                vtkSmartPointer< vtkXMLDataElement > elt =
                    vtkSmartPointer< vtkXMLDataElement >::New();
                elt->SetName(REPLICA_ID_ELEMENT_NAME);
                QString replicaID(QString("#%1").arg(objID));
                elt->SetAttribute(REPLICA_OBJECT_ID_ATTRIBUTE_NAME,
                                  replicaID.toStdString().c_str());
                replicator->AddNestedElement(elt);
              }
            }
          }
        }
      }
    }
    case 2: {
      // Going from 2 to 3, keyframes now have color information
      // but groups don't have color info, so objects without it
      // are handled
    }
    case 3:
      // Going from 3 to 4, springs are generalized to connectors
      // and have alpha and radius added as well as some names change
      {
        vtkXMLDataElement* springList =
            root->FindNestedElementWithName(SPRING_LIST_ELEMENT_NAME);
        if (springList == NULL) {
          return XML_TO_DATA_FAILURE;
        }
        springList->SetName(CONNECTOR_LIST_ELEMENT_NAME);
        const int n = springList->GetNumberOfNestedElements();
        for (int i = 0; i < n; i++) {
          vtkXMLDataElement* spring = springList->GetNestedElement(i);
          if (strcmp(spring->GetName(), SPRING_ELEMENT_NAME) == 0) {
            spring->SetName(CONNECTOR_ELEMENT_NAME);
            vtkXMLDataElement* props =
                spring->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
            if (props == NULL) {
              return XML_TO_DATA_FAILURE;
            }
            props->SetDoubleAttribute(CONNECTOR_ALPHA_ATTRIBUTE_NAME,
                                      SPRING_ALPHA_VALUE);
            props->SetDoubleAttribute(CONNECTOR_RADIUS_ATTRIBUTE_NAME,
                                      SPRING_DISPLAY_RADIUS);
          }
        }
      }
	case 4: {}
		// Going from 4 to 5, grouping is a keyframe operation so keyframes
		// now have additional information, including absolute position,
		// absolute orientation, grouping level, and a parent object, but
		// discrepancies with older versions are dealt with in readKeyrame()
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToProject(
    SketchBio::Project* proj, vtkXMLDataElement* elem)
{
  if (elem == NULL) {
    return XML_TO_DATA_FAILURE;
  }
  if (QString(elem->GetName()) == QString(ROOT_ELEMENT_NAME)) {
    if (convertToCurrent(elem) == XML_TO_DATA_FAILURE) {
      // if failed to convert file to readable format version, exit we cannot do
      // any more
      return XML_TO_DATA_FAILURE;
    }
    vtkXMLDataElement* models =
        elem->FindNestedElementWithName(MODEL_MANAGER_ELEMENT_NAME);
    if (models == NULL) {
      return XML_TO_DATA_FAILURE;
    }
//	printf("\nMADE IT HERE\n");
//	fflush(stdout);
    QHash< QPair<QString, int>, QPair<SketchModel*,int> > modelIds;
    if (xmlToModelManager(proj, models, modelIds) == XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }
//	printf("\nMADE IT PAST MODEL MANAGER\n");
//	fflush(stdout);
    vtkXMLDataElement* view =
        elem->FindNestedElementWithName(TRANSFORM_MANAGER_ELEMENT_NAME);
    if (view == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    if (xmlToTransforms(proj, view) == XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }
//	printf("\nMADE IT PAST TRANSFORMS\n");
//	fflush(stdout);
    QHash< QString, SketchObject* > objectIds;
    vtkXMLDataElement* objs =
        elem->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
    if (objs == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    if (xmlToObjectList(proj, objs, modelIds, objectIds) ==
        XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }
//	printf("\nMADE IT PAST OBJECT LIST\n");
//	fflush(stdout);
    vtkXMLDataElement* reps =
        elem->FindNestedElementWithName(REPLICATOR_LIST_ELEMENT_NAME);
    if (reps == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    if (xmlToReplicatorList(proj, reps, objectIds) == XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }
    vtkXMLDataElement* springs =
        elem->FindNestedElementWithName(CONNECTOR_LIST_ELEMENT_NAME);
    if (springs == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    if (xmlToSpringList(proj, springs, objectIds) == XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }
    vtkXMLDataElement* transformOps =
        elem->FindNestedElementWithName(TRANSFORM_OP_LIST_ELEMENT_NAME);
    if (transformOps == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    if (xmlToTransformOpList(proj, transformOps, objectIds) ==
        XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }

	double minLum, maxLum;
	if (elem->GetAttribute(MIN_LUMINANCE_ATTRIBUTE_NAME)) {
	    elem->GetScalarAttribute(MIN_LUMINANCE_ATTRIBUTE_NAME, minLum);
		elem->GetScalarAttribute(MAX_LUMINANCE_ATTRIBUTE_NAME, maxLum);
		proj->getWorldManager().setMinLuminance(minLum);
		proj->getWorldManager().setMaxLuminance(maxLum);
	}
  } else {
    return XML_TO_DATA_FAILURE;
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::objectFromClipboardXML(
    SketchBio::Project* proj, vtkXMLDataElement* elem, double* newPos)
{
  if (QString(elem->GetName()) == QString(ROOT_ELEMENT_NAME)) {
    if (convertToCurrent(elem) == XML_TO_DATA_FAILURE) {
      // if failed to convert file to readable format version, exit we cannot do
      // any more
      return XML_TO_DATA_FAILURE;
    }
    vtkXMLDataElement* models =
        elem->FindNestedElementWithName(MODEL_MANAGER_ELEMENT_NAME);
    if (models == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    QHash< QPair<QString, int>, QPair<SketchModel*,int> > modelIds;
    if (xmlToModelManager(proj, models, modelIds) == XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    }
    QHash< QString, SketchObject* > objectIds;
    vtkXMLDataElement* objs =
        elem->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
    if (objs == NULL) {
      return XML_TO_DATA_FAILURE;
    }
    QList< SketchObject* > objList;
    if (readObjectList(objList, objs, modelIds, objectIds) ==
        XML_TO_DATA_FAILURE) {
      return XML_TO_DATA_FAILURE;
    } else {
      for (int i = 0; i < objList.size(); i++) {
        double bb[6];
        q_vec_type center, pos, dir;
        ColorMapType::Type cmap = objList[i]->getColorMapType();
        proj->getWorldManager().addObject(objList[i]);
        objList[i]->getBoundingBox(bb);
        center[0] = (bb[1] + bb[0]) * 0.5;
        center[1] = (bb[3] + bb[2]) * 0.5;
        center[2] = (bb[5] + bb[4]) * 0.5;
        if (objList[i]->numInstances() == 1) {
          objList[i]->setColorMapType(cmap);
          objList[i]->getModelSpacePointInWorldCoordinates(center, center);
        }
        objList[i]->getPosition(pos);
        q_vec_subtract(dir, center, pos);
        q_vec_subtract(pos, newPos, dir);
        objList[i]->setPosition(pos);
      }
    }
	if (readKeyframesForObjectList(objs, objectIds) ==
	  XML_TO_DATA_FAILURE) {
	return XML_TO_DATA_FAILURE;
    }
  } else {
    return XML_TO_DATA_FAILURE;
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::modelNamesFromClipboardXML(
    QList< const char* > &list, SketchBio::Project *proj,
	vtkXMLDataElement *elem)
{
	vtkXMLDataElement* models = 
		elem->FindNestedElementWithName(MODEL_MANAGER_ELEMENT_NAME);
	if (models == NULL) {
		return XML_TO_DATA_FAILURE;
	}
	for (int i = 0; i < models->GetNumberOfNestedElements(); i++) {
	  vtkXMLDataElement* model = models->GetNestedElement(i);
	  if (model == NULL) {
		  return XML_TO_DATA_FAILURE;
	  }
	  int numConformations;
	  model->GetScalarAttribute(MODEL_NUM_CONFORMATIONS_ATTR_NAME,
                                numConformations);
	  for (int i = 0; i < numConformations; i++) {
		vtkXMLDataElement* conf = model->FindNestedElementWithNameAndAttribute(
			MODEL_CONFORMATION_ELEMENT_NAME, MODEL_CONFORMATION_NUMBER_ATTR_NAME,
			QString::number(i).toStdString().c_str());
		if (conf == NULL) {
			return XML_TO_DATA_FAILURE;
		}
		for (int i = 0; i < conf->GetNumberOfNestedElements(); i++) {
		  vtkXMLDataElement* child = conf->GetNestedElement(i);
		  if (child == NULL) {
			  return XML_TO_DATA_FAILURE;
		  }
		  if (QString(child->GetName()) == 
				QString(MODEL_RESOLUTION_ELEMENT_NAME)) {
			const char* filename = 
			  child->GetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME);
			if (filename == 0) {
				return XML_TO_DATA_FAILURE;
			}
			list.append(filename);
		  }
		}
	  }
	}
	if(list.isEmpty()) {
		std::cout << "Unable to find VTK file names in XML" << std::endl;
	}
	return XML_TO_DATA_SUCCESS;
}

void ProjectToXML::saveObjectFromClipboardXML(vtkXMLDataElement *elem, SketchBio::Project *proj,
												QString dirPath, QString name) {
	QDir dir(dirPath);
	dir.mkdir(name);
	QDir save_dir(dir.absoluteFilePath(name));
	QStringList allFiles;
	// save XML from clipboard into the directory
    QString file = save_dir.absoluteFilePath(STRUCTURE_XML_FILENAME);
	QList< const char* > modelFileNames;
	if (elem) {
		vtkIndent indent(0);
		vtkXMLUtilities::WriteElementToFile(elem,file.toStdString().c_str(),&indent);
		if (modelNamesFromClipboardXML(modelFileNames,proj,elem) 
			== XML_TO_DATA_FAILURE) {
			std::cout << "Failed to read model XML." << std::endl;		
		}
		allFiles.append(file);
	}
	else {
		std::cout << "Failed to read object." << std::endl;
	}
	
	// save VTK files for models
	if (!modelFileNames.isEmpty()) {
		for (QListIterator< const char* > it(modelFileNames); it.hasNext();) {
			const char* srcfilename = it.next();
			const char* basename = strrchr(srcfilename, '/') + 1;
			QString destfilename = save_dir.absoluteFilePath(basename);
			QFile srcfile(srcfilename);
			if (srcfile.exists()) 
			{
			   if(!srcfile.copy(destfilename)) {
				  std::cout << "Failed to copy VTK file (may already exist)." << std::endl;
			   }
			   else {
				  allFiles.append(destfilename);
			   }
			}
			else {
				std::cout << "Could not find the needed VTK file." << std::endl;
			}
		}
	}
	else {
		std::cout << "Read no VTK filenames from modelFileName list" << std::endl;
	}

	// zip up the XML and VTK files together
	QString zipname = name.append(".zip");
	if(!JlCompress::compressDir(dir.absoluteFilePath(zipname),
									save_dir.absolutePath(),true)) {
		std::cout << "Failed to zip XML and VTK files" << std::endl;
	}
	for (QStringListIterator it(allFiles); it.hasNext();) {
		QFile f(it.next());
		if(!f.remove()) {
			std::cout << "Failed to remove file" << std::endl;
		}
	}
	if(!dir.rmdir(save_dir.absolutePath())) {
		std::cout << "Failed to remove copied structure directory" << std::endl;
	}
}

void ProjectToXML::loadObjectFromSavedXML(SketchBio::Project*proj, QString zipPath,
                                          q_vec_type pos) {
	QDir dir(proj->getProjectDir());
	QStringList allFiles = JlCompress::extractDir(zipPath, proj->getProjectDir());
	
	QString xml_file = dir.absoluteFilePath(STRUCTURE_XML_FILENAME);
    QFile xml_f(xml_file);
    if (xml_f.exists())
    {
        vtkSmartPointer< vtkXMLDataElement > elem =
                vtkSmartPointer< vtkXMLDataElement >::Take(
                    vtkXMLUtilities::ReadElementFromFile(xml_file.toStdString().c_str())
                    );
		if(!xml_f.remove()) {
			std::cout << "Failed to remove XML file after loading" << std::endl;
		}
		if (objectFromClipboardXML(proj,elem,pos)
			 == XML_TO_DATA_FAILURE)
        {
            std::cout << "Read xml correctly, but reading object failed."
                        << std::endl;
		}
    }
	else {
		std::cout << "Could not find structure XML file in this directory" << std::endl;
	}
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToModelManager(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QPair<QString, int>, QPair<SketchModel*,int> >& modelIds)
{
  if (QString(elem->GetName()) != QString(MODEL_MANAGER_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    vtkXMLDataElement* child = elem->GetNestedElement(i);
    if (QString(child->GetName()) == QString(MODEL_ELEMENT_NAME)) {
      if (xmlToModel(proj, child, modelIds) != XML_TO_DATA_SUCCESS) {
        return XML_TO_DATA_FAILURE;
      }
    }
  }
//  printf("\nMADE IT PAST MODELS\n");
fflush(stdout);
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToModel(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QPair<QString, int>, QPair<SketchModel*,int> >& modelIds)
{
  if (QString(elem->GetName()) != QString(MODEL_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  // need addmodel method on project...
  QString id;
  int numConformations;
  double invMass, invMoment;
  id = elem->GetAttribute(ID_ATTRIBUTE_NAME);
  if (id == NULL) return XML_TO_DATA_FAILURE;
  if (!elem->GetScalarAttribute(MODEL_NUM_CONFORMATIONS_ATTR_NAME,
                                numConformations)) {
    return XML_TO_DATA_FAILURE;
  }

  vtkXMLDataElement* props =
      elem->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
  if (props == NULL) return XML_TO_DATA_FAILURE;
  if (!props->GetScalarAttribute(MODEL_IMASS_ATTRIBUTE_NAME, invMass)) {
    return XML_TO_DATA_FAILURE;
  }
  if (!props->GetScalarAttribute(MODEL_IMOMENT_ATTRIBUTE_NAME, invMoment)) {
    return XML_TO_DATA_FAILURE;
  }

  bool foundModel = false;
  QScopedPointer< SketchModel > model(new SketchModel(invMass, invMoment));

 
  for (int i = 0; i < numConformations; i++) {
     vtkXMLDataElement* conf = elem->FindNestedElementWithNameAndAttribute(
        MODEL_CONFORMATION_ELEMENT_NAME, MODEL_CONFORMATION_NUMBER_ATTR_NAME,
        QString::number(i).toStdString().c_str());
    if (conf == NULL) return XML_TO_DATA_FAILURE;
    vtkXMLDataElement* src =
        conf->FindNestedElementWithName(MODEL_SOURCE_ELEMENT_NAME);
    if (src == NULL) return XML_TO_DATA_FAILURE;
    const char* srcString = src->GetCharacterData();
    if (srcString == NULL) return XML_TO_DATA_FAILURE;
    if(proj->getModelManager().hasModel(srcString)) {
        model.reset(proj->getModelManager().getModel(srcString));
		foundModel = true;
		break;
	}
  }

  int conformations = 0;
  for (int i = 0; i < numConformations; i++) {
    // the relationship between conformation numbers and conformations needs
    // to be kept since only the conformation number is saved with each object
    vtkXMLDataElement* conf = elem->FindNestedElementWithNameAndAttribute(
        MODEL_CONFORMATION_ELEMENT_NAME, MODEL_CONFORMATION_NUMBER_ATTR_NAME,
        QString::number(i).toStdString().c_str());
    if (conf == NULL) return XML_TO_DATA_FAILURE;
    conformations++;
    vtkXMLDataElement* src =
        conf->FindNestedElementWithName(MODEL_SOURCE_ELEMENT_NAME);
    if (src == NULL) return XML_TO_DATA_FAILURE;
    const char* srcString = src->GetCharacterData();
    if (srcString == NULL) return XML_TO_DATA_FAILURE;

    vtkXMLDataElement* fullRes = conf->FindNestedElementWithNameAndAttribute(
        MODEL_RESOLUTION_ELEMENT_NAME, ID_ATTRIBUTE_NAME,
        getResolutionString(ModelResolution::FULL_RESOLUTION));
    if (fullRes == NULL) return XML_TO_DATA_FAILURE;
    const char* fileString =
        fullRes->GetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME);
    if (fileString == NULL) return XML_TO_DATA_FAILURE;

    if(!foundModel || proj->getModelManager().getModel(srcString) != model.data()) {
	  QString filename;
	  proj->getFileInProjDir(fileString, filename);
	  QString source = srcString;
	  if (source == CAMERA_MODEL_KEY) {
		model.reset(proj->getCameraModel());
		QPair<QString, int>idPair("#" + id,i);
		  QPair<SketchModel*, int>modelPair(model.data(),
					model->getNumberOfConformations() - 1);
		  modelIds.insert(idPair,modelPair);
		break;
	  } else {
		model->addConformation(source, filename);
	  }    
	  for (int j = 0; j < conf->GetNumberOfNestedElements(); j++) {
		vtkXMLDataElement* elt = conf->GetNestedElement(j);
		if (elt != src && elt != fullRes &&
			elt->GetName() == QString(MODEL_RESOLUTION_ELEMENT_NAME)) {
		const char* f = elt->GetAttribute(MODEL_FILENAME_ATTRIBUTE_NAME);
		if (f == NULL) return XML_TO_DATA_FAILURE;
		const char* r = elt->GetAttribute(ID_ATTRIBUTE_NAME);
		if (r == NULL) return XML_TO_DATA_FAILURE;
		ModelResolution::ResolutionType res = getResolutionTypeFromId(r);
		QString fName;
		proj->getFileInProjDir(f,fName);
		model->addSurfaceFileForResolution(
			model->getNumberOfConformations() - 1, res, fName);
		}
	  }
	  QPair<QString, int>idPair("#" + id,i);
	  QPair<SketchModel*, int>modelPair(model.data(),
				model->getNumberOfConformations() - 1);
	  modelIds.insert(idPair,modelPair);
	}
	else {
		QPair<QString, int>idPair("#" + id,i);
	  QPair<SketchModel*, int>modelPair(model.data(),
				model->getConformationNumber(srcString));
	  modelIds.insert(idPair,modelPair);
	}
  }
  if (model.data() != NULL && conformations == numConformations) {
    if (model.data() != proj->getCameraModel() && !foundModel) {
      SketchModel* myModel = proj->getModelManager().addModel(model.data());

      if (myModel != model.data()) {
        model.reset(myModel);
      }
    }
    model.take();
    return XML_TO_DATA_SUCCESS;
  } else {
    return XML_TO_DATA_FAILURE;
  }
}

inline void matrixFromDoubleArray(vtkMatrix4x4* mat, double* data)
{
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat->SetElement(i, j, data[i * 4 + j]);
    }
  }
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToTransforms(
    SketchBio::Project* proj, vtkXMLDataElement* elem)
{
  if (QString(elem->GetName()) != TRANSFORM_MANAGER_ELEMENT_NAME) {
    return XML_TO_DATA_FAILURE;
  }
  double roomWorld[16];
  double roomEye[16];
  vtkXMLDataElement* rtW =
      elem->FindNestedElementWithName(TRANSFORM_WORLD_TO_ROOM_ELEMENT_NAME);
  if (rtW == NULL) {
    return XML_TO_DATA_FAILURE;
  }
  int err =
      rtW->GetVectorAttribute(TRANSFORM_MATRIX_ATTRIBUTE_NAME, 16, roomWorld);
  if (err != 16) {
    return XML_TO_DATA_FAILURE;
  }
  vtkXMLDataElement* rtE =
      elem->FindNestedElementWithName(TRANSFORM_ROOM_TO_EYE_ELEMENT_NAME);
  if (rtE == NULL) {
    return XML_TO_DATA_FAILURE;
  }
  err = rtE->GetVectorAttribute(TRANSFORM_MATRIX_ATTRIBUTE_NAME, 16, roomEye);
  if (err != 16) {
    return XML_TO_DATA_FAILURE;
  }
  vtkSmartPointer< vtkMatrix4x4 > rwMat =
      vtkSmartPointer< vtkMatrix4x4 >::New();
  matrixFromDoubleArray(rwMat, roomWorld);
  vtkSmartPointer< vtkMatrix4x4 > reMat =
      vtkSmartPointer< vtkMatrix4x4 >::New();
  matrixFromDoubleArray(reMat, roomEye);
  proj->getTransformManager().setWorldToRoomMatrix(rwMat);
  proj->getTransformManager().setRoomToEyeMatrix(reMat);
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::readObjectList(
    QList< SketchObject* >& list, vtkXMLDataElement* elem,
    QHash< QPair<QString, int>, QPair<SketchModel*,int> >& modelIds,
    QHash< QString, SketchObject* >& objectIds)
{
  if (QString(elem->GetName()) != QString(OBJECTLIST_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  if (!list.empty()) {  // if the list is already populated, give up.
    return XML_TO_DATA_FAILURE;
  }
  // this function processes whole list not single object
  // need function that gets object and returns it for each object in list
  // this function needs to be changed to return list of objects for caller to
  // do something with
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    vtkXMLDataElement* child = elem->GetNestedElement(i);
    SketchObject* object = readObject(child, modelIds, objectIds);
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

ProjectToXML::XML_Read_Status ProjectToXML::readKeyframesForObjectList(
     vtkXMLDataElement* elem, QHash< QString, SketchObject* >& objectIds)
{
  if (QString(elem->GetName()) != QString(OBJECTLIST_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    vtkXMLDataElement* child = elem->GetNestedElement(i);
    QString oId(child->GetAttribute(ID_ATTRIBUTE_NAME));
	SketchObject* object = objectIds.value(oId);
	vtkXMLDataElement* keyframes =
        child->FindNestedElementWithName(OBJECT_KEYFRAME_LIST_ELEMENT_NAME);
    // keyframes list will only exist on objects that have keyframes so this
    // being NULL simply
    // means that the object has no keyframes
    if (keyframes != NULL) {  // if we have keyframes... read them in
      for (int i = 0; i < keyframes->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement* frame = keyframes->GetNestedElement(i);
        if (QString(OBJECT_KEYFRAME_ELEMENT_NAME) ==
            QString(frame->GetName())) {
          if (readKeyframe(object, objectIds, frame) == XML_TO_DATA_FAILURE) {
            return XML_TO_DATA_FAILURE;
          }
        }  // in xml: ignore extra stuff
      }
    } 
	vtkXMLDataElement* subObjects = 
		child->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
	if (subObjects != NULL) {
		if (readKeyframesForObjectList(subObjects, objectIds) == 
			XML_TO_DATA_FAILURE) {
			return XML_TO_DATA_FAILURE;
		}
	}
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::readKeyframe(
	SketchObject* object, QHash<QString,SketchObject *> &objectIds,
	vtkXMLDataElement* frame)
{
	if (QString(OBJECT_KEYFRAME_ELEMENT_NAME) != QString(frame->GetName())) {
		return XML_TO_DATA_FAILURE;
	}
	// make sure not to change the color map data, save the old data
	ColorMapType::Type colorMap;
	QString array;
	q_vec_type pos, absPos;
	q_type orient, absOrient;
	bool visA, active;
	int level;
	double time;
	int numRead = 0;
	numRead =
		frame->GetScalarAttribute(OBJECT_KEYFRAME_TIME_ATTRIBUTE_NAME, time);
	if (numRead != 1)
		return XML_TO_DATA_FAILURE;  // if frame has no time, then don't know what
	// to do with it
	vtkXMLDataElement* kTrans =
		frame->FindNestedElementWithName(TRANSFORM_ELEMENT_NAME);
	if (kTrans == NULL) return XML_TO_DATA_FAILURE;  // if no transform, fail
	numRead = kTrans->GetVectorAttribute(POSITION_ATTRIBUTE_NAME, 3, pos);
	if (numRead != 3) return XML_TO_DATA_FAILURE;  // position wrong length.. fail
	numRead = kTrans->GetVectorAttribute(ROTATION_ATTRIBUTE_NAME, 4, orient);
	if (numRead != 4)
		return XML_TO_DATA_FAILURE;  // orientation wrong length... fail
	vtkXMLDataElement* absTrans = 
		frame->FindNestedElementWithName(OBJECT_KEYFRAME_ABSTRFM_ELEMENT_NAME);
	if (absTrans == NULL) { //may be old format with no absolute position saved
		q_vec_copy(absPos, pos);
		q_copy(absOrient, orient);
	}
	else {
		numRead = absTrans->GetVectorAttribute(POSITION_ATTRIBUTE_NAME, 3, absPos);
		if (numRead != 3) 
			return XML_TO_DATA_FAILURE;  // position wrong length.. fail
		numRead = absTrans->GetVectorAttribute(ROTATION_ATTRIBUTE_NAME, 4, absOrient);
		if (numRead != 4)
			return XML_TO_DATA_FAILURE;  // orientation wrong length... fail
	}
	vtkXMLDataElement* properties =
		frame->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
	if (properties == NULL)
		return XML_TO_DATA_FAILURE;  // if no visiblitly status.. fail
	const char* c = properties->GetAttribute(OBJECT_KEYFRAME_VIS_AF_ATTR_NAME);
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
	if (object->numInstances() == 1) {
		c = frame->GetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME);
		if (c != NULL) {
			colorMap = ColorMapType::colorMapFromString(c);
			c = frame->GetAttribute(OBJECT_ARRAY_TO_COLOR_BY_ATTR_NAME);
			if (c != NULL) {
				array = QString(c);
			} else {
				return XML_TO_DATA_FAILURE;  // can have neither array or color, but
				// if you have one, you should have both
			}
		} else {
			// if not defined default to state of object
			colorMap = object->getColorMapType();
			array = object->getArrayToColorBy();
		}
	} else {
		// if not defined default to state of object
		colorMap = object->getColorMapType();
		array = object->getArrayToColorBy();
	}
	SketchObject* parent = NULL;
	const char* id = frame->GetAttribute(OBJECT_KEYFRAME_PARENT_ID_ATTRIBUTE_NAME);
	if (id == 0) { // if loading old version, use the object's parent (won't change at keyframes)
		parent = object->getParent();
	}
	else {
		QString parentId(&id[1]);
		if (strcmp(&id[1], "NULL")) {
			if (objectIds.contains(parentId)) {
				parent = objectIds.value(parentId);
			}
			else {
				return XML_TO_DATA_FAILURE;
			}
		}
	}
	numRead = frame->GetScalarAttribute(OBJECT_KEYFRAME_LEVEL_ATTRIBUTE_NAME, level);
	if (numRead != 1) { // if loading old version, find out the grouping level for keyframes now
		level = object->getGroupingLevel();
	}
	Keyframe f(pos,absPos,orient,absOrient,colorMap,array,level,parent,visA,active);
	object->insertKeyframe(time,f);
	return XML_TO_DATA_SUCCESS;
}

SketchObject* ProjectToXML::readObject(
    vtkXMLDataElement* elem, QHash< QPair<QString, int>, QPair<SketchModel*,int> >& modelIds,
    QHash< QString, SketchObject* >& objectIds)
{
  if (QString(elem->GetName()) == QString(OBJECT_ELEMENT_NAME)) {
    QString mId;
    int confNum = -1;
    q_vec_type pos;
    q_type orient;
    bool visibility, active;
	double luminance;
    QString oId(elem->GetAttribute(ID_ATTRIBUTE_NAME));
    int numInstances;
    if (!elem->GetScalarAttribute(OBJECT_NUM_INSTANCES_ATTRIBUTE_NAME,
                                  numInstances)) {
      return NULL;
    }
    vtkXMLDataElement* props =
        elem->FindNestedElementWithName(PROPERTIES_ELEMENT_NAME);
    if (props == NULL) {
      return NULL;
    }
    if (numInstances == 1) {
      const char* c = props->GetAttribute(OBJECT_MODELID_ATTRIBUTE_NAME);
      if (c == NULL) {
        return NULL;
      }
      mId = c;
      if (props->GetScalarAttribute(OBJECT_MODEL_CONF_NUM_ATTR_NAME, confNum) ==
          0)
        return NULL;
    } else {
      mId = "";
    }
    const char* c = props->GetAttribute(OBJECT_VISIBILITY_ATTRIBUTE_NAME);
    if (!c) {
      visibility = true;  // default to visible
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
    vtkXMLDataElement* trans =
        elem->FindNestedElementWithName(TRANSFORM_ELEMENT_NAME);
    if (trans == NULL) {
      return NULL;
    }
    int err = trans->GetVectorAttribute(POSITION_ATTRIBUTE_NAME, 3, pos);
    err = err + trans->GetVectorAttribute(ROTATION_ATTRIBUTE_NAME, 4, orient);
    if (err != 7) {
      return NULL;
    }
    q_normalize(orient, orient);
    QScopedPointer< SketchObject > object(NULL);
    if (numInstances == 1) {
      // modelInstace
	  QPair<QString,int>idPair(mId,confNum);
	  SketchModel* model = modelIds.value(idPair).first;
	  int modelConfNum = modelIds.value(idPair).second;
      object.reset(new ModelInstance(model, modelConfNum));
      object->setPosAndOrient(pos, orient);
      if (props->GetAttribute(OBJECT_ARRAY_TO_COLOR_BY_ATTR_NAME)) {
        QString array(props->GetAttribute(OBJECT_ARRAY_TO_COLOR_BY_ATTR_NAME));
        object->setArrayToColorBy(array);
      }
      if (props->GetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME)) {
        const char* ch = props->GetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME);
        ColorMapType::Type cmap;
        cmap = ColorMapType::colorMapFromString(ch);
        object->setColorMapType(cmap);
      }
	  if (props->GetAttribute(OBJECT_LUMINANCE_ATTRIBUTE_NAME)) {
	    props->GetScalarAttribute(OBJECT_LUMINANCE_ATTRIBUTE_NAME, luminance);
		object->setLuminance(luminance);
	  }
    } else {
      // group
      QScopedPointer< ObjectGroup > group(new ObjectGroup());
      group->setPosAndOrient(pos, orient);
      vtkXMLDataElement* childList =
          elem->FindNestedElementWithName(OBJECTLIST_ELEMENT_NAME);
      if (childList == NULL) {
        return NULL;
      }
      QList< SketchObject* > childObjects;
      if (readObjectList(childObjects, childList, modelIds, objectIds) ==
          XML_TO_DATA_FAILURE) {
        return NULL;
      }
      for (int i = 0; i < childObjects.size(); i++) {
        group->addObject(childObjects[i]);
      }
      object.reset(group.take());
      // since each object will be saved in its actual position and not its
      // group relative position,
      // we can simply let addObject's averaging take care of the group
      // position/orientation
    }
    objectIds.insert(oId, object.data());
    object->setIsVisible(visibility);  // set visibility after keyframes so
                                       // frames can store visibility state
    object->setActive(active);         // similar to reason above
    return object.take();
  }
  return NULL;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToObjectList(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QPair<QString, int>, QPair<SketchModel*,int> >& modelIds,
    QHash< QString, SketchObject* >& objectIds)
{
  QList< SketchObject* > objects;
  if (readObjectList(objects, elem, modelIds, objectIds) ==
      XML_TO_DATA_FAILURE) {
    return XML_TO_DATA_FAILURE;
  }
  if (readKeyframesForObjectList(elem, objectIds) ==
	  XML_TO_DATA_FAILURE) {
	return XML_TO_DATA_FAILURE;
  }
  for (int i = 0; i < objects.size(); i++) {
    ColorMapType::Type cmap = objects[i]->getColorMapType();
    proj->getWorldManager().addObject(objects[i]);
    if (objects[i]->numInstances() == 1) {
      objects[i]->setColorMapType(cmap);
    }
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToReplicatorList(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QString, SketchObject* >& objectIds)
{
  if (QString(elem->GetName()) != QString(REPLICATOR_LIST_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    vtkXMLDataElement* child = elem->GetNestedElement(i);
    if (QString(child->GetName()) == QString(REPLICATOR_ELEMENT_NAME)) {
      QString obj1Id, obj2Id, grpId;
      int numReplicas;
      const char* c = child->GetAttribute(REPLICATOR_OBJECT1_ATTRIBUTE_NAME);
      if (c == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      obj1Id = &c[1];
      c = child->GetAttribute(REPLICATOR_OBJECT2_ATTRIBUTE_NAME);
      if (c == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      obj2Id = &c[1];
      c = child->GetAttribute(REPLICAS_GROUP_ATTRIBUTE_NAME);
      if (c == NULL) {
        return XML_TO_DATA_FAILURE;
      }
	  grpId = &c[1];
      int err = child->GetScalarAttribute(REPLICATOR_NUM_SHOWN_ATTRIBUTE_NAME,
                                          numReplicas);
      if (!err) {
        return XML_TO_DATA_FAILURE;
      }
      SketchObject* obj = objectIds.value(grpId, NULL),
                    *first = objectIds.value(obj1Id, NULL),
                    *second = objectIds.value(obj2Id, NULL);
      QList< SketchObject* > repList;
      for (int i = 0; i < child->GetNumberOfNestedElements(); i++) {
        vtkXMLDataElement* replica = child->GetNestedElement(i);
        if (strcmp(replica->GetName(), REPLICA_ID_ELEMENT_NAME) == 0) {
          c = replica->GetAttribute(REPLICA_OBJECT_ID_ATTRIBUTE_NAME);
          if (c == NULL) {
            return XML_TO_DATA_FAILURE;
          }
          QString objID(&c[1]);
          repList.append(objectIds.value(objID, NULL));
        }
      }
      ObjectGroup* grp = dynamic_cast< ObjectGroup* >(obj);
      if (grp == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      if (first == NULL || second == NULL) {
        return XML_TO_DATA_FAILURE;
      }
      if (repList.contains(NULL)) {
        return XML_TO_DATA_FAILURE;
      }
      StructureReplicator* rep = new StructureReplicator(
          first, second, &proj->getWorldManager(), grp, repList);
      proj->addReplication(rep);
    }
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToSpringList(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QString, SketchObject* >& objectIds)
{
  if (QString(elem->GetName()) != QString(CONNECTOR_LIST_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    vtkXMLDataElement* child = elem->GetNestedElement(i);
    if (QString(child->GetName()) == QString(CONNECTOR_ELEMENT_NAME)) {
      if (xmlToSpring(proj, child, objectIds) != XML_TO_DATA_SUCCESS) {
        return XML_TO_DATA_FAILURE;
      }
    }
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToSpring(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QString, SketchObject* >& objectIds)
{
  if (QString(elem->GetName()) != QString(CONNECTOR_ELEMENT_NAME)) {
    return XML_TO_DATA_FAILURE;
  }
  const char* c;
  QString obj1Id, obj2Id;
  int objCount = 0, pointCount = 0;
  bool seenProperties = false;
  bool hasK = false, isMeasuringTape = false;
  double k, minRLen, maxRLen, alpha, radius;
  q_vec_type o1Pos, o2Pos, pt1, pt2;
  ColorMapType::Type cmap = ColorMapType::SOLID_COLOR_GRAY;
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    vtkXMLDataElement* child = elem->GetNestedElement(i);
    QString name = child->GetName();
    if (name == QString(CONNECTOR_OBJECT_END_ELEMENT_NAME)) {
      if (objCount == 0) {
        c = child->GetAttribute(CONNECTOR_OBJECT_ID_ATTRIBUTE_NAME);
		obj1Id = &c[1];
        int err = child->GetVectorAttribute(
            CONNECTOR_CONNECTION_POINT_ATTR_NAME, 3, o1Pos);
        if (err != 3) {
          return XML_TO_DATA_FAILURE;
        }
        if (obj1Id.length() > 0) {
          objCount++;
        }
      } else if (objCount == 1) {
        c = child->GetAttribute(CONNECTOR_OBJECT_ID_ATTRIBUTE_NAME);
		obj2Id = &c[1];
        int err = child->GetVectorAttribute(
            CONNECTOR_CONNECTION_POINT_ATTR_NAME, 3, o2Pos);
        if (err != 3) {
          return XML_TO_DATA_FAILURE;
        }
        if (obj2Id.length() > 0) {
          objCount++;
        }
      }
    } else if (name == QString(PROPERTIES_ELEMENT_NAME)) {
      seenProperties = true;
      if (child->GetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME)) {
        const char* ch = child->GetAttribute(OBJECT_COLOR_MAP_ATTRIBUTE_NAME);
        cmap = ColorMapType::colorMapFromString(ch);
      }
      hasK = (child->GetAttribute(SPRING_STIFFNESS_ATTRIBUTE_NAME) != NULL);
	  isMeasuringTape = 
		  (child->GetAttribute(MEASURING_TAPE_ATTRIBUTE_NAME) != NULL);
      int err = 0;
      err += child->GetScalarAttribute(CONNECTOR_ALPHA_ATTRIBUTE_NAME, alpha);
      err += child->GetScalarAttribute(CONNECTOR_RADIUS_ATTRIBUTE_NAME, radius);
      if (err != 2) {
        return XML_TO_DATA_FAILURE;
      }
      if (hasK) {
        err = child->GetScalarAttribute(SPRING_STIFFNESS_ATTRIBUTE_NAME, k);
        err +=
            child->GetScalarAttribute(SPRING_MIN_REST_ATTRIBUTE_NAME, minRLen);
        err +=
            child->GetScalarAttribute(SPRING_MAX_REST_ATTRIBUTE_NAME, maxRLen);
        if (err != 3) {
          return XML_TO_DATA_FAILURE;
        }
      }
    } else if (name == QString(CONNECTOR_POINT_END_ELEMENT_NAME)) {
      int err = child->GetVectorAttribute(CONNECTOR_CONNECTION_POINT_ATTR_NAME,
                                          3, (pointCount == 0) ? pt1 : pt2);
      if (err != 3) {
        return XML_TO_DATA_FAILURE;
      }
      pointCount++;
    }
  }
  Connector* conn = NULL;
  if (!seenProperties) {
    // if we didn't read a properties element
    return XML_TO_DATA_FAILURE;
  }
  if (pointCount + objCount != 2) {
    // we have too few or too many endpoints
    return XML_TO_DATA_FAILURE;
  } else if (pointCount == 1) {
    if (hasK) {
      conn = new SpringConnection(objectIds.value(obj1Id), NULL, minRLen,
                                  maxRLen, k, o1Pos, pt1, true);
    } else if (isMeasuringTape) {
	  conn = new MeasuringTape(objectIds.value(obj1Id), NULL, o1Pos, pt1);
    } else {
      conn = new Connector(objectIds.value(obj1Id), NULL, o1Pos, pt1, alpha,
                           radius);
    }
    conn->setColorMapType(cmap);
    proj->getWorldManager().addConnector(conn);
  } else if (pointCount == 2) {
    if (hasK) {
      conn = new SpringConnection(NULL, NULL, minRLen,
                                  maxRLen, k, pt1, pt2, true);
    } else if (isMeasuringTape) {
	  conn = new MeasuringTape(NULL, NULL, pt1, pt2);
    } else {
      conn = new Connector(NULL, NULL, pt1, pt2, alpha,
                           radius);
    }
    conn->setColorMapType(cmap);
    proj->getWorldManager().addConnector(conn);
  } else if (objCount == 2) {
    if (hasK) {
      conn = SpringConnection::makeSpring(objectIds.value(obj1Id),
                                          objectIds.value(obj2Id), o1Pos, o2Pos,
                                          false, k, minRLen, maxRLen, true);
    } else if (isMeasuringTape) {
	  conn = new MeasuringTape(objectIds.value(obj1Id), objectIds.value(obj2Id),
							   o1Pos, o2Pos);
	} else { conn = new Connector(objectIds.value(obj1Id), objectIds.value(obj2Id),
                           o1Pos, o2Pos, alpha, radius);
    }
    conn->setColorMapType(cmap);
    proj->getWorldManager().addConnector(conn);
  } else {
    return XML_TO_DATA_FAILURE;
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToTransformOpList(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QString, SketchObject* >& objectIds)
{
  for (int i = 0; i < elem->GetNumberOfNestedElements(); i++) {
    if (QString(elem->GetNestedElement(i)->GetName()) ==
        QString(TRANSFORM_OP_ELEMENT_NAME)) {
      if (xmlToTransformOp(proj, elem->GetNestedElement(i), objectIds) ==
          XML_TO_DATA_FAILURE) {
        return XML_TO_DATA_FAILURE;
      }
    }
  }
  return XML_TO_DATA_SUCCESS;
}

ProjectToXML::XML_Read_Status ProjectToXML::xmlToTransformOp(
    SketchBio::Project* proj, vtkXMLDataElement* elem,
    QHash< QString, SketchObject* >& objectIds)
{
  if (elem->GetNumberOfNestedElements() == 0) {
    return XML_TO_DATA_FAILURE;
  }
  vtkXMLDataElement* pair = elem->GetNestedElement(0);
  if (QString(pair->GetName()) != QString(TRANSFORM_OP_PAIR_ELEMENT_NAME) ||
      pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME) == NULL ||
      pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME) == NULL) {
    return XML_TO_DATA_FAILURE;
  }
  const char* c = pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME);
  QString o1 = &c[1];
  c = pair->GetAttribute(TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME);
  QString o2 = &c[1];
  QWeakPointer< TransformEquals > eq =
      proj->addTransformEquals(objectIds.value(o1), objectIds.value(o2));
  QSharedPointer< TransformEquals > eqS(eq);
  if (eqS) {
    for (int i = 1; i < elem->GetNumberOfNestedElements(); i++) {
      pair = elem->GetNestedElement(i);
      if (QString(pair->GetName()) == QString(TRANSFORM_OP_PAIR_ELEMENT_NAME)) {
        c = pair->GetAttribute(TRANSFORM_OP_PAIR_FIRST_ATTRIBUTE_NAME);
		QString obj1 = &c[1];
		c = pair->GetAttribute(TRANSFORM_OP_PAIR_SECOND_ATTRIBUTE_NAME);
        QString obj2 = &c[1];
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
