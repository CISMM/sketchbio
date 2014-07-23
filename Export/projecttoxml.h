#ifndef PROJECTTOXML_H
#define PROJECTTOXML_H

class vtkXMLDataElement;

class QString;
#include <QVector>
#include <QList>
#include <QHash>
#include <QSharedPointer>

class TransformManager;
class SketchModel;
class ModelManager;
class SketchObject;
class Connector;
class WorldManager;
class StructureReplicator;
class TransformEquals;
//<<<<<<< HEAD
namespace SketchBio
{
class Project;
}

class ProjectToXML
{
 public:
  enum XML_Read_Status { XML_TO_DATA_FAILURE = 0, XML_TO_DATA_SUCCESS = 1 };

  static vtkXMLDataElement *projectToXML(const SketchBio::Project *project);

  // creates a simplified version of the save to xml state that only
  // encapsulates
  // the information needed to recreate the given object
  static vtkXMLDataElement *objectToClipboardXML(const SketchObject *object);

  // assumes proj is a NEW SketchBio::Project with nothing in it
  // if there is an insufficient data error at any point in the tree,
  // the methods return immediately and whatever halfway state they were in is
  // the final state of the project.  So the final state of the project is
  // undefined if
  // there is bad xml
  static XML_Read_Status xmlToProject(SketchBio::Project *proj,
                                      vtkXMLDataElement *elem);

  // performs the inverse operation ot objectToClipboardXML and reads the given
  // xml into an existing project (this assumes the xml contains only the
  // information
  // written in objectToClipboardXML)
  static XML_Read_Status objectFromClipboardXML(SketchBio::Project *proj,
                                                vtkXMLDataElement *elem,
                                                double *newPos);
  // retrieves names of VTK files for models needed to save structures
  static XML_Read_Status modelNamesFromClipboardXML(QList< const char* > &list, SketchBio::Project *proj,
                                                    vtkXMLDataElement *elem);

  static void saveObjectFromClipboardXML(vtkXMLDataElement *elem, SketchBio::Project *proj,
                                          QString dirPath, QString name);

  static void loadObjectFromSavedXML(SketchBio::Project*proj, QString zipPath,
                                     double *newPos);

 private:  // no other code should call these (this is the reason for making
           // this a class)
  static vtkXMLDataElement *modelManagerToXML(
      const ModelManager &models, const QString &dir,
      QHash< const SketchModel *, QString > &modelIds);
  static vtkXMLDataElement *modelsToXML(
      const SketchObject *object,
      QHash< const SketchModel *, QString > &modelIds);
  static vtkXMLDataElement *modelToXML(const SketchModel *model,
                                       const QString &dir, const QString &id);

  static vtkXMLDataElement *transformManagerToXML(
      const TransformManager &transforms);

  static vtkXMLDataElement *objectListToXML(
      const WorldManager &world,
      const QHash< const SketchModel *, QString > &modelIds,
      QHash< const SketchObject *, QString > &objectIds);

  static vtkXMLDataElement *objectListToXML(
      const QList< SketchObject * > *objectList,
      const QHash< const SketchModel *, QString > &modelIds,
      QHash< const SketchObject *, QString > &objectIds,
	  bool saveKeyframes = true);

  static vtkXMLDataElement *objectToXML(
      const SketchObject *object,
      const QHash< const SketchModel *, QString > &modelIds,
      QHash< const SketchObject *, QString > &objectIds,
	  bool saveKeyframes = true);

  static vtkXMLDataElement *replicatorListToXML(
      const QList< StructureReplicator * > &replicaList,
      QHash< const SketchObject *, QString > &objectIds);

  static vtkXMLDataElement *springListToXML(
      const WorldManager &world,
      const QHash< const SketchObject *, QString > &objectIds);

  static vtkXMLDataElement *springToXML(
      const Connector *spring,
      const QHash< const SketchObject *, QString > &objectIds);

  static vtkXMLDataElement *transformOpListToXML(
      const QVector< QSharedPointer< TransformEquals > > &ops,
      const QHash< const SketchObject *, QString > &objectIds);

  // converts the older file to the current xml project format
  // returns success unless something goes wrong in conversion
  static XML_Read_Status convertToCurrent(vtkXMLDataElement *root);
  static XML_Read_Status convertToCurrentVersion(vtkXMLDataElement *root,
                                                 int minorVersion);

  // this is used by readObject to read in each keyframe to the object.  Note
  // that any state that
  // is set in the object to create the keyframe is also restored by the end of
  // the function
  static XML_Read_Status readKeyframe(
      SketchObject *object, QHash< QString, SketchObject * > &objectIds,
      vtkXMLDataElement *frame);
  // these are for reading objects from the xml... need recursively defined
  // functions for groups
  // if there is an error, they will clean up any created objects as they fail
  // this one returns the object or NULL on an error
  static SketchObject *readObject(vtkXMLDataElement *elem,
                                  QHash< QPair<QString,int>, QPair<SketchModel*,int> > &modelIds,
                                  QHash< QString, SketchObject * > &objectIds);
  // this one populates the passed in list with the objects it reads and returns
  // an error code
  // it assumes an empty list is passed in... and will fail without doing
  // anything if this is not
  // the case
  static XML_Read_Status readObjectList(
      QList< SketchObject * > &list, vtkXMLDataElement *elem,
      QHash< QPair<QString,int>, QPair<SketchModel*,int> > &modelIds,
      QHash< QString, SketchObject * > &objectIds);
  // this reads in the keyframes for each object in a list, and is used after
  // calling readObjectList()
  // so that all potential keyframe parents have already been loaded
  static XML_Read_Status readKeyframesForObjectList(
      vtkXMLDataElement *elem, QHash< QString, SketchObject * > &objectIds);
  // these all return an error code, XML_TO_DATA_SUCCESS for succeeded
  // and XML_TO_DATA_FAILURE for failed

  static XML_Read_Status xmlToModelManager(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
          QHash< QPair<QString, int>, QPair<SketchModel*,int> > &modelIds);

  static XML_Read_Status xmlToModel(SketchBio::Project *proj,
                                    vtkXMLDataElement *elem,
                                    QHash< QPair<QString,int>, QPair<SketchModel*,int> > &modelIds);

  static XML_Read_Status xmlToTransforms(SketchBio::Project *proj,
                                         vtkXMLDataElement *elem);

  static XML_Read_Status xmlToObjectList(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
      QHash< QPair<QString,int>, QPair<SketchModel*,int> > &modelIds,
      QHash< QString, SketchObject * > &objectIds);

  static XML_Read_Status xmlToReplicatorList(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
      QHash< QString, SketchObject * > &objectIds);

  static XML_Read_Status xmlToSpringList(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
      QHash< QString, SketchObject * > &objectIds);

  static XML_Read_Status xmlToSpring(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
      QHash< QString, SketchObject * > &objectIds);

  static XML_Read_Status xmlToTransformOpList(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
      QHash< QString, SketchObject * > &objectIds);

  static XML_Read_Status xmlToTransformOp(
      SketchBio::Project *proj, vtkXMLDataElement *elem,
      QHash< QString, SketchObject * > &objectIds);
/*=======
class SketchProject;

class ProjectToXML {
public:
    enum XML_Read_Status {
        XML_TO_DATA_FAILURE=0,
        XML_TO_DATA_SUCCESS=1
    };

    static vtkXMLDataElement *projectToXML(const SketchProject *project);

    // creates a simplified version of the save to xml state that only encapsulates
    // the information needed to recreate the given object
    static vtkXMLDataElement *objectToClipboardXML(const SketchObject *object);

    // assumes proj is a NEW SketchProject with nothing in it
    // if there is an insufficient data error at any point in the tree,
    // the methods return immediately and whatever halfway state they were in is
    // the final state of the project.  So the final state of the project is undefined if
    // there is bad xml
    static XML_Read_Status xmlToProject(SketchProject *proj, vtkXMLDataElement *elem);

    // performs the inverse operation ot objectToClipboardXML and reads the given
    // xml into an existing project (this assumes the xml contains only the information
    // written in objectToClipboardXML)
    static XML_Read_Status objectFromClipboardXML(SketchProject *proj,
                                                  vtkXMLDataElement *elem,
                                                  double *newPos);
	// retrieves names of VTK files for models needed to save structures
	static XML_Read_Status modelNamesFromClipboardXML(QList< const char* > &list, SketchProject *proj,
													  vtkXMLDataElement *elem);

	static void saveObjectFromClipboardXML(vtkXMLDataElement *elem, SketchProject *proj, 
											QString dirPath);

	static void loadObjectFromSavedXML(SketchProject*proj, QString zipPath);

private: // no other code should call these (this is the reason for making this a class)
    static vtkXMLDataElement *modelManagerToXML(const ModelManager *models, const QString &dir,
                                                QHash<const SketchModel *, QString> &modelIds);
    static vtkXMLDataElement *modelsToXML(const SketchObject *object,
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
                                          QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *replicatorListToXML(
            const QList<StructureReplicator *> *replicaList,
            QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *springListToXML(const WorldManager *world,
                                              const QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *springToXML(const Connector *spring,
                                          const QHash<const SketchObject *, QString> &objectIds);

    static vtkXMLDataElement *transformOpListToXML(const QVector<QSharedPointer<TransformEquals> > *ops,
                                                   const QHash<const SketchObject *, QString> &objectIds);

    // converts the older file to the current xml project format
    // returns success unless something goes wrong in conversion
    static XML_Read_Status convertToCurrent(vtkXMLDataElement *root);
    static XML_Read_Status convertToCurrentVersion(vtkXMLDataElement *root,int minorVersion);

    // this is used by readObject to read in each keyframe to the object.  Note that any state that
    // is set in the object to create the keyframe is also restored by the end of the function
    static XML_Read_Status readKeyframe(SketchObject *object, QHash<QString,SketchObject *> &objectIds,
										vtkXMLDataElement *frame);
    // these are for reading objects from the xml... need recursively defined functions for groups
    // if there is an error, they will clean up any created objects as they fail
    // this one returns the object or NULL on an error
    static SketchObject *readObject(vtkXMLDataElement *elem,
                                    QHash< QPair<QString, int>, QPair<SketchModel*,int> > &modelIds,
                                    QHash<QString, SketchObject *> &objectIds);
    // this one populates the passed in list with the objects it reads and returns an error code
    // it assumes an empty list is passed in... and will fail without doing anything if this is not
    // the case
    static XML_Read_Status readObjectList(QList<SketchObject *> &list, vtkXMLDataElement *elem,
                                           QHash< QPair<QString, int>, QPair<SketchModel*,int> > &modelIds,
                                           QHash<QString,SketchObject *> &objectIds);
	// this reads in the keyframes for each object in a list, and is used after calling readObjectList()
	// so that all potential keyframe parents have already been loaded
	static XML_Read_Status readKeyframesForObjectList(vtkXMLDataElement *elem,
                                           QHash<QString,SketchObject *> &objectIds);
    // these all return an error code, XML_TO_DATA_SUCCESS for succeeded
    // and XML_TO_DATA_FAILURE for failed

    static XML_Read_Status xmlToModelManager(SketchProject *proj, vtkXMLDataElement *elem,
                                             QHash< QPair<QString, int>, QPair<SketchModel*,int> > &modelIds);

    static XML_Read_Status xmlToModel(SketchProject *proj, vtkXMLDataElement *elem,
                                      QHash< QPair<QString, int>, QPair<SketchModel*,int> > &modelIds);

    static XML_Read_Status xmlToTransforms(SketchProject *proj, vtkXMLDataElement *elem);

    static XML_Read_Status xmlToObjectList(SketchProject *proj, vtkXMLDataElement *elem,
                                           QHash< QPair<QString, int>, QPair<SketchModel*,int> > &modelIds,
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
>>>>>>> master*/
};

#endif  // PROJECTTOXML_H
