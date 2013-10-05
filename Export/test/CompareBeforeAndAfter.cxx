#include "CompareBeforeAndAfter.h"

#include <iostream>
using std::cout;
using std::endl;
#include <cmath>

#include <sketchtests.h>
#include <sketchmodel.h>
#include <modelmanager.h>
#include <keyframe.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <springconnection.h>
#include <worldmanager.h>
#include <structurereplicator.h>
#include <transformequals.h>
#include <sketchproject.h>

// This should print the error message along with the file and line number
// this should make finding which test failed a lot easier
#define PRINT_ERROR(msg) cout << __FILE__ << ":" << __LINE__ << " " << msg << endl

namespace CompareBeforeAndAfter
{

void compareNumbers(SketchProject* proj1, SketchProject* proj2, int& retVal)
{
    // make sure both have camera models created (easier than making sure neither has it)
    proj1->getCameraModel();
    proj2->getCameraModel();
    if (proj2->getModelManager()->getNumberOfModels()
            != proj1->getModelManager()->getNumberOfModels())
    {
        retVal++;
        PRINT_ERROR("Number of models different");
    }
    if (proj2->getWorldManager()->getNumberOfObjects()
            != proj1->getWorldManager()->getNumberOfObjects())
    {
        retVal++;
        PRINT_ERROR("Number of objects is different");
    }
    if (proj1->getWorldManager()->getNumberOfConnectors()
            != proj2->getWorldManager()->getNumberOfConnectors())
    {
        retVal++;
        PRINT_ERROR("Number of springs is different");
    }
    if (proj1->getNumberOfReplications()
            != proj2->getNumberOfReplications())
    {
        retVal++;
        PRINT_ERROR("Number of replications is different");
    }
    if (proj1->getTransformOps()->size()
            != proj2->getTransformOps()->size())
    {
        retVal++;
        PRINT_ERROR("Number of transform ops is different");
    }
    if (proj1->getCameras()->size()
            != proj2->getCameras()->size() )
    {
        retVal++;
        PRINT_ERROR("Number of cameras is different");
    }
}

void compareModels(const SketchModel* m1, const SketchModel* m2, int& numDifferences,
                   bool printDiffs)
{
    if (m1->getNumberOfConformations() != m2->getNumberOfConformations())
    {
        numDifferences++;
        if (printDiffs) PRINT_ERROR("Different numbers of conformations.");
        return;
    }
    for (int i = 0; i < m1->getNumberOfConformations(); i++)
    {
        if (m1->getSource(i) != m2->getSource(i))
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Conformation " << i << " sources are different.");
        }
        if (m1->getFileNameFor(i,ModelResolution::FULL_RESOLUTION)
                != m2->getFileNameFor(i,ModelResolution::FULL_RESOLUTION))
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Conformation " << i << " filenames are different.");
        }
        if (m1->getFileNameFor(i,ModelResolution::SIMPLIFIED_FULL_RESOLUTION)
                != m2->getFileNameFor(i,ModelResolution::SIMPLIFIED_FULL_RESOLUTION))
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Conformation " << i << " filenames are different.");
        }
        if (m1->getFileNameFor(i,ModelResolution::SIMPLIFIED_5000)
                != m2->getFileNameFor(i,ModelResolution::SIMPLIFIED_5000))
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Conformation " << i << " filenames are different.");
        }
        if (m1->getFileNameFor(i,ModelResolution::SIMPLIFIED_2000)
                != m2->getFileNameFor(i,ModelResolution::SIMPLIFIED_2000))
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Conformation " << i << " filenames are different.");
        }
        if (m1->getFileNameFor(i,ModelResolution::SIMPLIFIED_1000)
                != m2->getFileNameFor(i,ModelResolution::SIMPLIFIED_1000))
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Conformation " << i << " filenames are different.");
        }
    }
    if (Q_ABS(m1->getInverseMass() - m2->getInverseMass()) > Q_EPSILON)
    {
        numDifferences++;
        if (printDiffs) PRINT_ERROR("Masses are different.");
    }
    if (Q_ABS(m1->getInverseMomentOfInertia() - m2->getInverseMomentOfInertia()) > Q_EPSILON)
    {
        numDifferences++;
        if (printDiffs) PRINT_ERROR("Moments of inertia are different.");
    }
}

void compareObjects(const SketchObject* o1, const SketchObject* o2,
                    int& numDifferences, bool printDiffs)
{
    if (o1 == NULL || o2 == NULL)
    {
        if (o1 == o2)
        {
            return;
        }
        else
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("One object is NULL.");
            return;
        }
    }
    if (o1->numInstances() != o2->numInstances())
    {
        numDifferences++;
        if (printDiffs) PRINT_ERROR("Different numbers of objects contained in groups!");
        return;
    }
    double epsilon_modifier = 1;
    const SketchObject* p = o1->getParent();
    // if this is the case, it is probably a replica, so give a higher tolerance
    if (o1->isLocalTransformPrecomputed() && p != NULL)
    {
        double d = (4.0 * p->getSubObjects()->size());
        epsilon_modifier *= d;
    }
    while (p != NULL)
    {
        epsilon_modifier *= 8;
        p = p->getParent();
    }
    q_vec_type pos1, pos2;
    q_type orient1, orient2;
    o1->getPosition(pos1);
    // we have to take the maximum dimension into account for the epsilon value too...
    double vector_epsilon = max(max(Q_ABS(pos1[0]*Q_EPSILON),
                   Q_ABS(pos1[1]*Q_EPSILON)),
            Q_ABS(pos1[2]*Q_EPSILON));
    o2->getOrientation(orient1);
    o2->getPosition(pos2);
    o2->getOrientation(orient2);
    if (!q_vec_equals(pos1,pos2,epsilon_modifier *vector_epsilon))
    {
        numDifferences++;
        if (printDiffs)
        {
            PRINT_ERROR("positions wrong");
            q_vec_type diff;
            q_vec_subtract(diff,pos1,pos2);
            std::cout << "P1: ";
            q_vec_print(pos1);
            std::cout << "P2: ";
            q_vec_print(pos2);
            std::cout << "DIFF: ";
            printf("(%g, %g, %g), epsilon_modifier = %g\n",diff[0], diff[1], diff[2],epsilon_modifier);
        }
        return;
    }
    if (o1->isVisible() != o2->isVisible())
    {
        numDifferences++;
        if (printDiffs) PRINT_ERROR("Visibility state of objects is different.");
        return;
    }
    if (o1->isActive() != o2->isActive())
    {
        numDifferences++;
        if (printDiffs) PRINT_ERROR("Active state of objects is different.");
        return;
    }
    if (o1->numInstances() == 1)
    { // test single instance things... orientation of a group is tested by
        // position of group memebers in recursion
        if (!q_equals(orient1,orient2,epsilon_modifier * Q_EPSILON))
        {
            numDifferences++;
            if (printDiffs)
            {
                PRINT_ERROR("orientations wrong");
                std::cout << "O1: ";
                q_print(orient1);
                std::cout << "O2: ";
                q_print(orient2);
            }
            return;
        }
        if (o2->getModelConformation() != o1->getModelConformation())
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Model conformation changed");
            return;
        }
        else
        {
            compareModels(o1->getModel(), o2->getModel(), numDifferences, printDiffs);
        }
        if (o1->getColorMapType() != o2->getColorMapType())
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Color map changed");
            return;
        }
        if (o1->getArrayToColorBy() != o2->getArrayToColorBy())
        {
            numDifferences++;
            if (printDiffs) PRINT_ERROR("Array to color by changed");
            return;
        }
    }
    else
    { // test group specific stuff
        compareObjectLists(*o1->getSubObjects(),*o2->getSubObjects(),numDifferences,printDiffs);
    }
    if (o1->getNumKeyframes() != o2->getNumKeyframes())
    {
        numDifferences++;
        if (printDiffs)
        {
            PRINT_ERROR("Different numbers of keyframes.");
            PRINT_ERROR("Object 1 has: " << o1->getNumKeyframes());
            PRINT_ERROR("Object 2 has: " << o2->getNumKeyframes());
        }
        return;
    }
    else if (o1->getNumKeyframes() > 0)
    {
        QMapIterator< double,Keyframe > it1(*o1->getKeyframes());
        QMapIterator< double,Keyframe > it2(*o2->getKeyframes());
        while (it1.hasNext())
        {
            double time1 = it1.peekNext().key();
            double time2 = it2.peekNext().key();
            if (Q_ABS(time1-time2) > Q_EPSILON)
            {
                numDifferences++;
                if (printDiffs) PRINT_ERROR("Keyframe for time " << time1
                                     << " missing in object 2. It has time "
                                     << time2 << " instead.");
                return;
            }
            else
            {
                const Keyframe& frame1 = it1.next().value();
                const Keyframe& frame2 = it2.next().value();
                q_vec_type p1, p2;
                q_type o1, o2;
                frame1.getPosition(p1);
                frame2.getPosition(p2);
                frame1.getOrientation(o1);
                frame2.getOrientation(o2);
                double vector_epsilon = max(max(Q_ABS(p1[0]*Q_EPSILON),
                                            Q_ABS(p1[1]*Q_EPSILON)),
                        Q_ABS(p1[2]*Q_EPSILON));
                if (!q_vec_equals(p1,p2,epsilon_modifier * vector_epsilon))
                {
                    numDifferences++;
                    if (printDiffs) PRINT_ERROR("Keyframe positions are different at time "
                                         << time1);
                    return;
                }
                if (!q_equals(o1,o2,epsilon_modifier * Q_EPSILON))
                {
                    numDifferences++;
                    if (printDiffs) PRINT_ERROR("Keyframe orientations are different at time "
                                         << time1);
                    return;
                }
                if (frame1.isVisibleAfter() != frame2.isVisibleAfter())
                {
                    numDifferences++;
                    if (printDiffs) PRINT_ERROR("Keyframe visibility is different at time "
                                         << time1);
                    return;
                }
                if (frame1.isActive() != frame2.isActive())
                {
                    numDifferences++;
                    if (printDiffs) PRINT_ERROR("Keyframe active state is different at time "
                                         << time1);
                    return;
                }
                if (frame1.getColorMapType() != frame2.getColorMapType())
                {
                    numDifferences++;
                    if (printDiffs) PRINT_ERROR("Keyframe color changed at time "
                                         << time1);
                }
                if (frame1.getArrayToColorBy() != frame2.getArrayToColorBy())
                {
                    numDifferences++;
                    if (printDiffs) PRINT_ERROR("Keyframe array to color by is different at "
                                            "time " << time1);
                }
            }
        }
    }
}

void compareObjectLists(const QList< SketchObject* >& list1,
                        const QList< SketchObject* >& list2,
                        int& retVal, bool printDiffs)
{
    if (list1.size() != list2.size())
    {
        retVal++;
        if (printDiffs) PRINT_ERROR("Different numbers of objects in lists.");
    }
    QScopedPointer< bool, QScopedPointerArrayDeleter< bool > >
            used(new bool[list2.size()]);
    for (int i = 0; i < list2.size(); i++)
    {
        used.data()[i] = false;
    }
    for (QListIterator< SketchObject* > it1(list1); it1.hasNext();)
    {
        const SketchObject* n1 = it1.next();
        int i = 0;
        bool failed = true;
        for (QListIterator< SketchObject* > it2(list2); it2.hasNext();i++)
        {
            const SketchObject* n2 = it2.next();
            if (!used.data()[i])
            {
                int diffs = 0;
                compareObjects(n1,n2,diffs,true);
                if (diffs == 0)
                {
                    used.data()[i] = true;
                    failed = false;
                    break;
                }
            }
        }
        if (failed)
        {
            retVal++;
            if (printDiffs) PRINT_ERROR("Could not match object.");
        }
    }

}

void compareCameras(SketchProject* proj1, SketchProject* proj2, int& retVal)
{
    const QHash< SketchObject*, vtkSmartPointer< vtkCamera > >* cameras1 =
            proj1->getCameras();
    const QHash< SketchObject*, vtkSmartPointer< vtkCamera > >* cameras2 =
            proj2->getCameras();
    for (QHashIterator< SketchObject*, vtkSmartPointer< vtkCamera > > it (*cameras1);
         it.hasNext(); )
    {
        SketchObject* obj1 = it.next().key();
        bool match = false;
        for (QHashIterator< SketchObject*, vtkSmartPointer< vtkCamera > >
             it2(*cameras2); it2.hasNext(); )
        {
            SketchObject* obj2 = it2.next().key();
            int numDiffs = 0;
            compareObjects(obj1,obj2,numDiffs,true);
            if (numDiffs == 0)
            {
                match = true;
            }
        }
        if (match == false)
        {
            retVal++;
            PRINT_ERROR("Could not match camera.");
        }
    }
}

void compareTransformOps(QSharedPointer< TransformEquals > t1,
                         QSharedPointer< TransformEquals > t2,
                         int& retVal, bool printDiffs)
{
    const QVector< ObjectPair >* l1 = t1->getPairsList(), * l2 = t2->getPairsList();
    if (l1->size() != l2->size())
    {
        retVal++;
        if (printDiffs)
        {
            PRINT_ERROR("Different numbers of object pairs in transform equals.");
        }
        return;
    }
    QScopedPointer< bool, QScopedPointerArrayDeleter< bool > >
            used(new bool[l2->size()]);
    for (int i = 0; i < l2->size(); i++)
    {
        used.data()[i] = false;
    }
    for (int i = 0; i < l1->size(); i++)
    {
        bool failed = true;
        for (int j = 0; j < l2->size(); j++)
        {
            if (!used.data()[j])
            {
                int diffs = 0;
                compareObjects(l1->at(i).o1,l2->at(j).o1,diffs,false);
                compareObjects(l1->at(i).o2,l2->at(j).o2,diffs,false);
                if (diffs == 0)
                {
                    failed = false;
                    used.data()[i] = true;
                }
            }
        }
        if (failed)
        {
            retVal++;
            if (printDiffs) PRINT_ERROR("Could not match object pair");
        }
    }
}

void compareTransformOpLists(const QVector<QSharedPointer<TransformEquals> >& list1,
                             const QVector<QSharedPointer<TransformEquals> >& list2,
                             int& retVal, bool printDiffs)
{
    if (list1.size() != list2.size())
    {
        retVal++;
        if (printDiffs) PRINT_ERROR("Different numbers of transform ops");
    }
    QScopedPointer< bool, QScopedPointerArrayDeleter< bool > >
            used(new bool[list2.size()]);
    for (int i = 0; i < list2.size(); i++)
    {
        used.data()[i] = false;
    }
    for (QVectorIterator< QSharedPointer< TransformEquals > > it1(list1);
         it1.hasNext();)
    {
        QSharedPointer< TransformEquals > n1 = it1.next();
        int i = 0;
        bool failed = true;
        for (QVectorIterator< QSharedPointer< TransformEquals > > it2(list2);
             it2.hasNext();i++)
        {
            QSharedPointer< TransformEquals > n2 = it2.next();
            if (!used.data()[i])
            {
                int diffs = 0;
                compareTransformOps(n1,n2,diffs,false);
                if (diffs == 0)
                {
                    used.data()[i] = true;
                    failed = false;
                    break;
                }
            }
        }
        if (failed)
        {
            retVal++;
            if (printDiffs) PRINT_ERROR("Could not match transform op.");
        }
    }
}

void compareWorldObjects(SketchProject* proj1, SketchProject* proj2, int& retVal)
{
    compareObjectLists(*proj1->getWorldManager()->getObjects(),
                       *proj2->getWorldManager()->getObjects(),
                       retVal,true);
}

void compareReplications(const StructureReplicator* rep1, const StructureReplicator* rep2,
                        int& diffs, bool printDiffs)
{
    int v = 0;
    compareObjects(rep1->getFirstObject(),rep2->getFirstObject(),v,printDiffs);
    if ( v != 0)
    {
        diffs++;
        if (printDiffs) PRINT_ERROR("First objects didn't match");
    }
    v = 0;
    compareObjects(rep1->getSecondObject(),rep2->getSecondObject(),v,printDiffs);
    if ( v != 0)
    {
        diffs++;
        if (printDiffs) PRINT_ERROR("Second objects didn't match");
    }
    if (rep1->getNumShown() != rep2->getNumShown())
    {
        diffs++;
        if (printDiffs) PRINT_ERROR("Number of replicas different");
    }
}

void compareReplicationLists(const QList< StructureReplicator* >& list1,
                             const QList< StructureReplicator* >& list2,
                             int& retVal, bool printDiffs)
{
    if (list1.size() != list2.size())
    {
        retVal++;
        if (printDiffs) PRINT_ERROR("Different numbers of replications");
    }
    QScopedPointer< bool, QScopedPointerArrayDeleter< bool > >
            used(new bool[list2.size()]);
    for (int i = 0; i < list2.size(); i++)
    {
        used.data()[i] = false;
    }
    for (QListIterator< StructureReplicator* > it1(list1);
         it1.hasNext();)
    {
        StructureReplicator* n1 = it1.next();
        int i = 0;
        bool failed = true;
        for (QListIterator< StructureReplicator* > it2(list2);
             it2.hasNext();i++)
        {
            StructureReplicator* n2 = it2.next();
            if (!used.data()[i])
            {
                int diffs = 0;
                compareReplications(n1,n2,diffs,printDiffs);
                if (diffs == 0)
                {
                    used.data()[i] = true;
                    failed = false;
                    break;
                }
            }
        }
        if (failed)
        {
            retVal++;
            if (printDiffs) PRINT_ERROR("Could not match replication.");
        }
    }
}

void compareConnectors(const Connector* c1, const Connector* c2,
                    int& diffs, bool printDiffs)
{
    int mydiffs = 0;
    int v = 0;
    q_vec_type pos1, pos2;
    compareObjects(c1->getObject1(),c2->getObject1(),v,printDiffs);
    if (v != 0)
    {
        v = 0;
        compareObjects(c1->getObject1(),c2->getObject2(),v,printDiffs);
        if (v != 0)
        {
            mydiffs++;
            if (printDiffs) PRINT_ERROR("Cannot match objects in 2-obj connector.");
        }
        else
        {
            c1->getObject1ConnectionPosition(pos1);
            c2->getObject2ConnectionPosition(pos2);
            if (!q_vec_equals(pos1,pos2))
            {
                mydiffs++;
                if (printDiffs) PRINT_ERROR("Object connection positions don't match");
            }
            v = 0;
            compareObjects(c1->getObject2(),c2->getObject1(),v,printDiffs);
            if (v != 0)
            {
                mydiffs++;
                if (printDiffs) PRINT_ERROR("Cannot match objects in 2-obj connector.");
            }
            else
            {
                c1->getObject2ConnectionPosition(pos1);
                c2->getObject1ConnectionPosition(pos2);
                if (!q_vec_equals(pos1,pos2))
                {
                    mydiffs++;
                    if (printDiffs) PRINT_ERROR("Object connection positions don't match");
                }
            }
        }
    }
    else
    { // the first objects match
        c1->getObject1ConnectionPosition(pos1);
        c2->getObject1ConnectionPosition(pos2);
        if (!q_vec_equals(pos1,pos2))
        {
            mydiffs++;
            if (printDiffs) PRINT_ERROR("Object connection positions don't match");
        }
        v = 0;
        compareObjects(c1->getObject2(),c2->getObject2(),v,printDiffs);
        if (v != 0)
        {
            mydiffs++;
            if (printDiffs) PRINT_ERROR("Unable to match objects in 2-obj connector");
        }
        else
        { // if the second objects match
            c1->getObject2ConnectionPosition(pos1);
            c2->getObject2ConnectionPosition(pos2);
            if (!q_vec_equals(pos1,pos2))
            {
                mydiffs++;
                if (printDiffs) PRINT_ERROR("Object connection positions don't match");
            }
            if (mydiffs && printDiffs)
            {
                q_vec_print(pos1);
                q_vec_print(pos2);
            }
        }
    }
    if (Q_ABS(c1->getAlpha() - c2->getAlpha()) > Q_EPSILON)
    {
        mydiffs++;
        if (printDiffs) PRINT_ERROR("Alpha of connector is different");
    }
    if (Q_ABS(c1->getRadius() - c2->getRadius()) > Q_EPSILON)
    {
        mydiffs++;
        if (printDiffs) PRINT_ERROR("Radius of connector is different");
    }
    const SpringConnection* sp1 = dynamic_cast< const SpringConnection* >(c1);
    const SpringConnection* sp2 = dynamic_cast< const SpringConnection* >(c2);
    if (sp1 != NULL && sp2 != NULL)
    {
        if (Q_ABS(sp1->getStiffness()-sp2->getStiffness()) > Q_EPSILON)
        {
            mydiffs++;
            if (printDiffs) PRINT_ERROR("Stiffness is different");
        }
        if (Q_ABS(sp1->getMinRestLength() - sp2->getMinRestLength()) > Q_EPSILON)
        {
            mydiffs++;
            if (printDiffs) PRINT_ERROR("Minimum rest length changed");
        }
        if (Q_ABS(sp1->getMaxRestLength() - sp2->getMaxRestLength()) > Q_EPSILON)
        {
            mydiffs++;
            if (printDiffs) PRINT_ERROR("Maximum rest length changed");
        }
    }
    else if ((sp1 != NULL) ^ (sp2 != NULL))
    {
        mydiffs++;
        if (printDiffs) PRINT_ERROR("One is a spring and the other is not");
    }
    diffs += mydiffs;
}

void compareConnectorLists(const QList< Connector* >& list1,
                        const QList< Connector* >& list2,
                        int& retVal, bool printDiffs)
{
    if (list1.size() != list2.size())
    {
        retVal++;
        if (printDiffs) PRINT_ERROR("Different numbers of springs");
    }
    QScopedPointer< bool, QScopedPointerArrayDeleter< bool > >
            used(new bool[list2.size()]);
    for (int i = 0; i < list2.size(); i++)
    {
        used.data()[i] = false;
    }
    for (QListIterator< Connector* > it1(list1);
         it1.hasNext();)
    {
        Connector* n1 = it1.next();
        int i = 0;
        bool failed = true;
        for (QListIterator< Connector* > it2(list2);
             it2.hasNext();i++)
        {
            Connector* n2 = it2.next();
            if (!used.data()[i])
            {
                int diffs = 0;
                compareConnectors(n1,n2,diffs,false);
                if (diffs == 0)
                {
                    used.data()[i] = true;
                    failed = false;
                    break;
                }
            }
        }
        if (failed)
        {
            retVal++;
            if (printDiffs) PRINT_ERROR("Could not match spring.");
        }
    }
}

void compareProjects(SketchProject* proj1, SketchProject* proj2, int& differences)
{
    compareNumbers(proj1,proj2,differences);
    compareWorldObjects(proj1,proj2,differences);
    compareConnectorLists(proj1->getWorldManager()->getSprings(),
                       proj2->getWorldManager()->getSprings(),differences,true);
    compareCameras(proj1,proj2,differences);
    compareReplicationLists(*proj1->getReplicas(),*proj2->getReplicas(),
                            differences,true);
    compareTransformOpLists(*proj1->getTransformOps(),*proj2->getTransformOps(),
                            differences,true);
}

}
