#ifndef PROJECTTOXML_H
#define PROJECTTOXML_H

#include <vtkXMLDataElement.h>
#include "sketchproject.h"

namespace ProjectToXML {
enum XML_Read_Status {
    XML_TO_DATA_FAILURE=0,
    XML_TO_DATA_SUCCESS=1
};

vtkXMLDataElement *projectToXML(const SketchProject *project);

vtkXMLDataElement *modelManagerToXML(const ModelManager *models, const QString &dir,
                                     QHash<const SketchModel *, QString> &modelIds);

vtkXMLDataElement *modelToXML(const SketchModel *model, const QString &dir, const QString &id);

vtkXMLDataElement *transformManagerToXML(const TransformManager *transforms);

vtkXMLDataElement *objectListToXML(const WorldManager *world, const QHash<const SketchModel *, QString> &modelIds,
                              QHash<const SketchObject *, QString> &objectIds);

vtkXMLDataElement *objectToXML(const SketchObject *object, const QHash<const SketchModel *, QString> &modelIds,
                               const QString &id);

vtkXMLDataElement *replicatorListToXML(const QList<StructureReplicator *> *replicaList,
                                       const QHash<const SketchModel *, QString> &modelIds,
                                       QHash<const SketchObject *, QString> &objectIds);

vtkXMLDataElement *springListToXML(const WorldManager *world,
                                   const QHash<const SketchObject *, QString> &objectIds);

vtkXMLDataElement *springToXML(const SpringConnection *spring,
                               const QHash<const SketchObject *, QString> &objectIds);

// these all return an error code, XML_TO_DATA_SUCCESS for succeeded
// and XML_TO_DATA_FAILURE for failed


// assumes proj is a NEW SketchProject with nothing in it
// if there is an insufficient data error at any point in the tree,
// the methods return immediately and whatever halfway state they were in is
// the final state of the project.  So the final state of the project is undefined if
// there is bad xml
XML_Read_Status xmlToProject(SketchProject *proj, vtkXMLDataElement *elem);

XML_Read_Status xmlToModelManager(SketchProject *proj, vtkXMLDataElement *elem,
                       QHash<QString, SketchModel *> &modelIds);

XML_Read_Status xmlToModel(SketchProject *proj, vtkXMLDataElement *elem,
                QHash<QString, SketchModel *> &modelIds);

XML_Read_Status xmlToTransforms(SketchProject *proj, vtkXMLDataElement *elem);

XML_Read_Status xmlToObjectList(SketchProject *proj, vtkXMLDataElement *elem, QHash<QString,
                     SketchModel *> &modelIds, QHash<QString, SketchObject *> &objectIds);

XML_Read_Status xmlToReplicatorList(SketchProject *proj, vtkXMLDataElement *elem,
                         QHash<QString, SketchObject *> &objectIds);

XML_Read_Status xmlToSpringList(SketchProject *proj, vtkXMLDataElement *elem,
                     QHash<QString, SketchObject *> &objectIds);

XML_Read_Status xmlToSpring(SketchProject *proj, vtkXMLDataElement *elem,
                 QHash<QString, SketchObject *> &objectIds);
}

#endif // PROJECTTOXML_H
