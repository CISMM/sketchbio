#ifndef STRUCTUREREPLICATOR_H
#define STRUCTUREREPLICATOR_H

#include "worldmanager.h"
#include "transformmanager.h"
#include <list>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>


#define STRUCTURE_REPLICATOR_MAX_COPIES 100

/*
 * This class replicates the transformation between the given two actors to a number of copies.
 * The number of copies can be changed dynamically.
 */
class StructureReplicator
{
public:
    /*
     * Creates a new StructureRenderer with the given two actors as a baseline, the given
     * mapper containing the model to use on the newly created actors, and the given renderer
     * as the renderer to register the copies with.
     */
    StructureReplicator(ObjectId object1Id, ObjectId object2Id, WorldManager *w, TransformManager *trans);

    /*
     * Changes the number of copies shown to the given amount
     *
     * num must be in the interval [0, STRUCTURE_REPLICATOR_MAX_COPIES]
     */
    void setNumShown(int num);

    /*
     * Gets the number of copies that are being shown
     */
    int getNumShown();

    /*
     * Updates the transform used to generate all the models
     */
    void updateTransform();

private:
    int numShown;
    ObjectId id1, id2;
    WorldManager *world;
    std::list<ObjectId> newIds;
    vtkSmartPointer<vtkTransform> transform;
    TransformManager *transforms;
};

#endif // STRUCTUREREPLICATOR_H
