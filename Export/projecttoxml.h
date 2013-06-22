#ifndef PROJECTTOXML_H
#define PROJECTTOXML_H

#include <vtkXMLDataElement.h>
#include "sketchproject.h"

class ProjectToXML {
public:
    enum XML_Read_Status {
        XML_TO_DATA_FAILURE=0,
        XML_TO_DATA_SUCCESS=1
    };

    static vtkXMLDataElement *projectToXML(const SketchProject *project);

    // assumes proj is a NEW SketchProject with nothing in it
    // if there is an insufficient data error at any point in the tree,
    // the methods return immediately and whatever halfway state they were in is
    // the final state of the project.  So the final state of the project is undefined if
    // there is bad xml
    static XML_Read_Status xmlToProject(SketchProject *proj, vtkXMLDataElement *elem);

private: // no other code should call these (this is the reason for making this a class
    static vtkXMLDataElement *modelManagerToXML(const ModelManager *models, const QString &dir,
                                                QHash<const SketchModel *, QString> &modelIds);
    static vtkXMLDataElement *modelToXML(const SketchModel *model, const QString &dir,
                                         const QString &id);

    static vtkXMLDataElement *transformManagerToXML(const TransformManager *transforms);

    static vtkXMLDataElement *objectListToXML(const WorldManager *world, const QHash<const SketchModel *, QString> &modelIds,
                                              QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *objectListToXML(const QList<SketchObject *> *objectList,
                                              const QHash<const SketchModel *, QString> &modelIds,
                                              QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *objectToXML(const SketchObject *object, const QHash<const SketchModel *, QString> &modelIds,
                                          QHash<const SketchObject *, QString> &objectIds, const QString &id);

    static vtkXMLDataElement *replicatorListToXML(const QList<StructureReplicator *> *replicaList,
                                                  const QHash<const SketchModel *, QString> &modelIds,
                                                  QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *springListToXML(const WorldManager *world,
                                              const QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *springToXML(const SpringConnection *spring,
                                          const QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *transformOpListToXML(const QVector<QSharedPointer<TransformEquals> > *ops,
                                                   const QHash<const SketchObject *, QString> &objectIds);

    // converts the older file to the current xml project format
    // returns success unless something goes wrong in conversion
    static XML_Read_Status convertToCurrent(vtkXMLDataElement *root);
    static XML_Read_Status convertToCurrentVersion(vtkXMLDataElement *root,int minorVersion);

    // this is used by readObject to read in each keyframe to the object.  Note that any state that
    // is set in the object to create the keyframe is also restored by the end of the function
    static XML_Read_Status readKeyframe(SketchObject *object, vtkXMLDataElement *frame);
    // these are for reading objects from the xml... need recursively defined functions for groups
    // if there is an error, they will clean up any created objects as they fail
    // this one returns the object or NULL on an error
    static SketchObject *readObject(vtkXMLDataElement *elem,
                                    QHash<QString, SketchModel *> &modelIds,
                                    QHash<QString, SketchObject *> &objectIds);
    // this one populates the passed in list with the objects it reads and returns an error code
    // it assumes an empty list is passed in... and will fail without doing anything if this is not
    // the case
    static XML_Read_Status readObjectList(QList<SketchObject *> &list, vtkXMLDataElement *elem,
                                           QHash<QString,SketchModel *> &modelIds,
                                           QHash<QString,SketchObject *> &objectIds);
    // these all return an error code, XML_TO_DATA_SUCCESS for succeeded
    // and XML_TO_DATA_FAILURE for failed

    static XML_Read_Status xmlToModelManager(SketchProject *proj, vtkXMLDataElement *elem,
                                             QHash<QString, SketchModel *> &modelIds);

    static XML_Read_Status xmlToModel(SketchProject *proj, vtkXMLDataElement *elem,
                                      QHash<QString, SketchModel *> &modelIds);

    static XML_Read_Status xmlToTransforms(SketchProject *proj, vtkXMLDataElement *elem);

    static XML_Read_Status xmlToObjectList(SketchProject *proj, vtkXMLDataElement *elem,
                                           QHash<QString,SketchModel *> &modelIds,
                                           QHash<QString,SketchObject *> &objectIds);

    static XML_Read_Status xmlToReplicatorList(SketchProject *proj, vtkXMLDataElement *elem,
                                               QHash<QString, SketchObject *> &objectIds);

    static XML_Read_Status xmlToSpringList(SketchProject *proj, vtkXMLDataElement *elem,
                                           QHash<QString, SketchObject *> &objectIds);

    static XML_Read_Status xmlToSpring(SketchProject *proj, vtkXMLDataElement *elem,
                                       QHash<QString, SketchObject *> &objectIds);

    static XML_Read_Status xmlToTransformOpList(SketchProject *proj, vtkXMLDataElement *elem,
                                            QHash<QString, SketchObject *> &objectIds);

    static XML_Read_Status xmlToTransformOp(SketchProject *proj, vtkXMLDataElement *elem,
                                            QHash<QString, SketchObject *> &objectIds);
};

#endif // PROJECTTOXML_H
