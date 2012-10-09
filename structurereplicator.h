#ifndef STRUCTUREREPLICATOR_H
#define STRUCTUREREPLICATOR_H

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkMapper.h>

#include "transformmanager.h"

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
    StructureReplicator(vtkActor *ref1, vtkActor *ref2, vtkRenderer *rend, vtkMapper *map, TransformManager *transforms);

    /*
     * Changes the number of copies shown to the given amount
     *
     * num must be in the interval [0, STRUCTURE_REPLICATOR_MAX_COPIES]
     */
    void setNumShown(int num);

    /*
     * Updates the transformation between copies, should be called after changing the positions of
     * the original actors
     */
    void updateBaseline();

private:
    int numShown;
    vtkSmartPointer<vtkTransform> master;
    vtkSmartPointer<vtkActor> actor1;
    vtkSmartPointer<vtkActor> actor2;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkMapper> mapper;
    vtkSmartPointer<vtkActor> copies[STRUCTURE_REPLICATOR_MAX_COPIES];
    TransformManager *transformMgr;
};

#endif // STRUCTUREREPLICATOR_H
