#ifndef COMPAREBEFOREANDAFTER_H
#define COMPAREBEFOREANDAFTER_H

#include <QList>
#include <QSharedPointer>

class SketchModel;
class SketchObject;
class SketchProject;
class TransformEquals;
class StructureReplicator;
class SpringConnection;

namespace CompareBeforeAndAfter
{

void compareNumbers(SketchProject *proj1, SketchProject *proj2, int &retVal);

void compareModels(const SketchModel *m1, const SketchModel *m2, int &numDifferences,
                   bool printDiffs);

void compareObjects(const SketchObject *o1, const SketchObject *o2,
                    int &numDifferences, bool printDiffs);

void compareObjectLists(const QList< SketchObject * > &list1,
                        const QList< SketchObject * > &list2,
                        int &retVal, bool printDiffs);

void compareCameras(SketchProject *proj1, SketchProject *proj2, int &retVal);

void compareTransformOps(QSharedPointer< TransformEquals > t1,
                         QSharedPointer< TransformEquals > t2,
                         int &retVal, bool printDiffs);

void compareTransformOpLists(const QVector< QSharedPointer< TransformEquals > > &list1,
                             const QVector< QSharedPointer< TransformEquals > > &list2,
                             int &retVal, bool printDiffs);

void compareWorldObjects(SketchProject *proj1, SketchProject *proj2, int &retVal);

void compareReplications(const StructureReplicator *rep1,
                         const StructureReplicator *rep2,
                         int &diffs, bool printDiffs = false);

void compareReplicationLists(const QList< StructureReplicator * > &list1,
                             const QList< StructureReplicator * > &list2,
                             int retVal, bool printDiffs);

void compareSprings(const SpringConnection *sp1, const SpringConnection *sp2,
                    int &diffs, bool printDiffs = false);

void compareSpringLists(const QList< SpringConnection * > &list1,
                        const QList< SpringConnection * > &list2,
                        int retVal, bool printDiffs);

void compareProjects(SketchProject *proj1, SketchProject *proj2,
                     int &differences);
}

#endif
