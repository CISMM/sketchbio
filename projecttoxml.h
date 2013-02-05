#ifndef PROJECTTOXML_H
#define PROJECTTOXML_H

#include <vtkXMLDataElement.h>
#include "sketchproject.h"

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

// assumes proj is a NEW SketchProject with nothing in it
void xmlToProject(SketchProject *proj, vtkXMLDataElement *elem);

void xmlToModelManager(SketchProject *proj, vtkXMLDataElement *elem,
                       QHash<QString, SketchModel *> &modelIds);

void xmlToModel(SketchProject *proj, vtkXMLDataElement *elem,
                QHash<QString, SketchModel *> &modelIds);

void xmlToTransforms(SketchProject *proj, vtkXMLDataElement *elem);

void xmlToObjectList(SketchProject *proj, vtkXMLDataElement *elem, QHash<QString,
                     SketchModel *> &modelIds, QHash<QString, SketchObject *> &objectIds);

void xmlToReplicatorList(SketchProject *proj, vtkXMLDataElement *elem,
                         QHash<QString, SketchObject *> &objectIds);

#endif // PROJECTTOXML_H
