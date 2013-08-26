#ifndef STRUCTUREREPLICATOR_H
#define STRUCTUREREPLICATOR_H

#include <QList>

#include <vtkSmartPointer.h>
class vtkTransform;

class SketchObject;
class ObjectGroup;
class WorldManager;
#include "objectchangeobserver.h"

#define STRUCTURE_REPLICATOR_MAX_COPIES 100

/*
 * This class replicates the transformation between the given two objects to a number of copies.
 * The number of copies can be changed dynamically.  Each copy will be of class ReplicatedObject.
 */
class StructureReplicator : public ObjectChangeObserver
{
public:
    /*
     * Creates a new StructureRenderer with the given two actors as a baseline, adding
     * the copies the the world manager passed to the constructor
     */
    StructureReplicator(SketchObject *object1, SketchObject *object2, WorldManager *w);

    /*
     * Creates a new StructureReplicator with the given base objects, adding the copies
     * to the given group and initializing the list of copies with the given list,
     * which is assumed to be in order starting closest to the base object.
     *
     * This constructor will set the localTransforms of the objects in the list
     * to be based on the first two objects, and add them to the given group if
     * they are not already among its members.
     *
     * Note that unlike the other constructor, this one does not add the group to
     * the world manager.
     */
    StructureReplicator(SketchObject *object1, SketchObject *object2,
                        WorldManager *w, ObjectGroup *grp,
                        QList< SketchObject * > &replicas);

    virtual ~StructureReplicator();

    /*
     * Changes the number of copies shown to the given amount
     *
     * num must be in the interval [0, STRUCTURE_REPLICATOR_MAX_COPIES]
     */
    void setNumShown(int num);

    /*
     * Gets the number of copies that are being shown
     */
    int getNumShown() const;

    /*
     * Gets an iterator over the copies that have been generated
     */
    QListIterator<SketchObject *> getReplicaIterator() const;

    /*
     * Gets the first original object
     */
    inline const SketchObject *getFirstObject() const { return obj1; }
    inline SketchObject *getFirstObject() { return obj1; }
    /*
     * Gets the second original object
     */
    inline const SketchObject *getSecondObject() const { return obj2; }
    inline SketchObject *getSecondObject() { return obj2; }

    /*
     * Updates the transform used to generate all the models
     */
    void updateTransform();

    /*
     * Makes sure the objects that define position are keyframed correctly when
     * the replication chain is keyframed
     */
    virtual void objectKeyframed(SketchObject *obj, double time);
    /*
     * Makes sure that only replicas are in the replicas group
     */
    virtual void subobjectAdded(SketchObject *parent, SketchObject *child);
    /*
     * If an object is deleted out of the replicas group, delete everything
     * that depends on it
     */
    virtual void subobjectRemoved(SketchObject *parent, SketchObject *child);

    /*
     * Gets the group that contains all the replicas
     */
    ObjectGroup *getReplicaGroup();

private:
    int numShown;
    SketchObject *obj1, *obj2;
    ObjectGroup *replicas;
    QList< SketchObject * > replicaList;
    WorldManager *world;
    vtkSmartPointer<vtkTransform> transform;
};


#endif // STRUCTUREREPLICATOR_H
