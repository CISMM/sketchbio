#ifndef STRUCTUREREPLICATOR_H
#define STRUCTUREREPLICATOR_H

#include "worldmanager.h"
#include "transformmanager.h"
#include <list>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>


#define STRUCTURE_REPLICATOR_MAX_COPIES 100

/*
 * This class is a bit of a cheat, it allows replicated objects to transfer any forces
 * applied to them back to the original objects so that it appears that the force was applied
 * to the replica.  The forces applied to the original objects are scaled based on how many
 * copies away the force is coming from.
 */
class ReplicatedObject : public ModelInstance {
public:
    ReplicatedObject(SketchModel *model,
                     SketchObject *original0, SketchObject *original1, int num);
    virtual void addForce(q_vec_type point, const q_vec_type force);
    virtual void getPosition(q_vec_type dest) const;
    virtual void getOrientation(q_type dest) const;
    virtual void setPrimaryCollisionGroupNum(int num);
    virtual int  getPrimaryCollisionGroupNum() const;
    virtual bool isInCollisionGroup(int num) const;
    inline  int  getReplicaNum() const { return replicaNum; }
private:
    // need original(s) here
    SketchObject *obj0; // first original
    SketchObject *obj1; // second original
    int replicaNum; // how far down the chain we are indexed as follows:
                    // -1 is first copy in negative direction
                    // 0 is first original
                    // 1 is second original
                    // 2 is first copy in positive direction
};

/*
 * This class replicates the transformation between the given two objects to a number of copies.
 * The number of copies can be changed dynamically.  Each copy will be of class ReplicatedObject.
 */
class StructureReplicator
{
public:
    /*
     * Creates a new StructureRenderer with the given two actors as a baseline, the given
     * mapper containing the model to use on the newly created actors, and the given renderer
     * as the renderer to register the copies with.
     */
    StructureReplicator(SketchObject *object1, SketchObject *object2, WorldManager *w, vtkTransform *worldEyeTransform);

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
    /*
     * Gets the second original object
     */
    inline const SketchObject *getSecondObject() const { return obj2; }

    /*
     * Updates the transform used to generate all the models
     */
    void updateTransform();

private:
    int numShown;
    SketchObject *obj1, *obj2;
    WorldManager *world;
    QList<SketchObject *> copies;
    vtkSmartPointer<vtkTransform> transform;
    vtkSmartPointer<vtkTransform> worldEyeTransform;
};

inline QListIterator<SketchObject *> StructureReplicator::getReplicaIterator() const {
    return QListIterator<SketchObject *>(copies);
}

#endif // STRUCTUREREPLICATOR_H
