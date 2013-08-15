#ifndef MAKETESTPROJECT_H
#define MAKETESTPROJECT_H

class SketchObject;
class StructureReplicator;
class SketchProject;

namespace MakeTestProject
{
// makes sure the project has the model names that we are assuming are there...
void setUpProject(SketchProject *proj);

// adds an object to the project (position/orientation based on current # of
// objects).
SketchObject *addObjectToProject(SketchProject *proj, int conformation = 0);

// adds a camera to the project (position/orientation based on current # of
// objects).
void addCameraToProject(SketchProject *proj);

// adds a group to the project with the given number of items...
SketchObject *addGroupToProject(SketchProject *proj, int numItemsInGroup);

// adds a replication to the project with the given number of replicas
// Note: this creates two new objects to base the replication on
StructureReplicator *addReplicationToProject(SketchProject *proj, int numReplicas);

// adds a spring to the project between the given two objects
void addSpringToProject(SketchProject *proj, SketchObject *o1, SketchObject *o2);

// adds a transform equals with the given number of pairs to the project.
void addTransformEqualsToProject(SketchProject *proj, int numPairs);

// adds some keyframes to the object
void addKeyframesToObject(SketchObject *obj, int numKeyframes);

// sets color map and array to color by info for the project
void setColorMapForObject(SketchObject *obj);
}

#endif // MAKETESTPROJECT_H
