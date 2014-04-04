#ifndef MAKETESTPROJECT_H
#define MAKETESTPROJECT_H

class SketchObject;
class StructureReplicator;
namespace SketchBio {
class Project;
}

namespace MakeTestProject
{
// makes sure the project has the model names that we are assuming are there...
void setUpProject(SketchBio::Project *proj);

// adds an object to the project (position/orientation based on current # of
// objects).
SketchObject *addObjectToProject(SketchBio::Project *proj, int conformation = 0);

// adds a camera to the project (position/orientation based on current # of
// objects).
SketchObject *addCameraToProject(SketchBio::Project *proj);

// adds a group to the project with the given number of items...
SketchObject *addGroupToProject(SketchBio::Project *proj, int numItemsInGroup);

// adds a replication to the project with the given number of replicas
// Note: this creates two new objects to base the replication on
StructureReplicator *addReplicationToProject(SketchBio::Project *proj, int numReplicas);

// adds a spring to the project between the given two objects
void addSpringToProject(SketchBio::Project *proj, SketchObject *o1, SketchObject *o2);

// adds a connector to the project between the given two objects
void addConnectorToProject(SketchBio::Project *proj, SketchObject *o1, SketchObject *o2);

// adds a transform equals with the given number of pairs to the project.
void addTransformEqualsToProject(SketchBio::Project *proj, int numPairs);

// adds some keyframes to the object
void addKeyframesToObject(SketchObject *obj, int numKeyframes);

// sets color map and array to color by info for the project
void setColorMapForObject(SketchObject *obj);
}

#endif // MAKETESTPROJECT_H
