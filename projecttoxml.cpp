#include "projecttoxml.h"

#define ID_ATTRIBUTE_NAME   "id"

#define ROOT_ELEMENT_NAME   "sketchbio"
#define VERSION_STRING      "version"
#define SAVE_VERSION_NUM    "1.0"

#define MODEL_MANAGER_ELEMENT_NAME  "models"

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

vtkXMLDataElement *projectToXML(const SketchProject *project) {
    vtkXMLDataElement *element = vtkXMLDataElement::New();
    element->SetName(ROOT_ELEMENT_NAME);
    element->SetAttribute(VERSION_STRING,SAVE_VERSION_NUM);

    QHash<const SketchModel *, QString> modelIds;

    element->AddNestedElement(modelManagerToXML(project->getModelManager(),project->getProjectDir(),modelIds));
    element->AddNestedElement(transformManagerToXML(project->getTransformManager()));

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
