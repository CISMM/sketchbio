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

#endif // PROJECTTOXML_H
