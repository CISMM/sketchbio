#ifndef COMPAREBEFOREANDAFTER_H
#define COMPAREBEFOREANDAFTER_H

#include <QList>
#include <QSharedPointer>

class SketchModel;
class SketchObject;
namespace SketchBio {
class Project;
}
class TransformEquals;
class StructureReplicator;
class Connector;

namespace CompareBeforeAndAfter
{

void compareNumbers(SketchBio::Project* proj1, SketchBio::Project* proj2, int& retVal);

void compareModels(const SketchModel* m1, const SketchModel* m2, int& numDifferences,
                   bool printDiffs);

void compareObjects(const SketchObject* o1, const SketchObject* o2,
                    int& numDifferences, bool printDiffs, bool compareKeyframes);

void compareObjectLists(const QList< SketchObject* >& list1,
                        const QList< SketchObject* >& list2,
                        int& retVal, bool printDiffs, bool compareKeyframes);

void compareCameras(SketchBio::Project* proj1, SketchBio::Project* proj2, int& retVal);

void compareTransformOps(QSharedPointer< TransformEquals > t1,
                         QSharedPointer< TransformEquals > t2,
                         int& retVal, bool printDiffs);

void compareTransformOpLists(const QVector< QSharedPointer< TransformEquals > >& list1,
                             const QVector< QSharedPointer< TransformEquals > >& list2,
                             int& retVal, bool printDiffs);

void compareWorldObjects(SketchBio::Project* proj1, SketchBio::Project* proj2, int& retVal);

void compareReplications(const StructureReplicator* rep1,
                         const StructureReplicator* rep2,
                         int& diffs, bool printDiffs = false);

void compareReplicationLists(const QList< StructureReplicator* >& list1,
                             const QList< StructureReplicator* >& list2,
                             int& retVal, bool printDiffs);

void compareConnectors(const Connector* sp1, const Connector* sp2,
                    int& diffs, bool printDiffs = false);

void compareConnectorLists(const QList< Connector* >& list1,
                        const QList< Connector* >& list2,
                        int& retVal, bool printDiffs);

void compareProjects(SketchBio::Project* proj1, SketchBio::Project* proj2,
                     int& differences);
}

#endif
