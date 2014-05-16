#ifndef CONTROLFUNCTIONS_H
#define CONTROLFUNCTIONS_H

class vtkTransform;

namespace SketchBio {
class Project;
}
class SketchObject;

namespace ControlFunctions
{
typedef void (*ButtonControlFunctionPtr)(SketchBio::Project*,int,bool);
typedef void (*AnalogControlFunctionPtr)(SketchBio::Project*,int,double);

	// ANIMATION functions:
    //adds keyframes to all objects in world
    void keyframeAll(SketchBio::Project*, int, bool);
    //toggles keyframe for the nearest object to the hand
    void toggleKeyframeObject(SketchBio::Project*, int, bool);
    //adds a camera to the position of the hand
    void addCamera(SketchBio::Project*, int, bool);
    //toggles showing animation
    void showAnimationPreview(SketchBio::Project*, int, bool);

	// GROUP EDITING functions:
    //adds or removes the nearest object from a group
    void toggleGroupMembership(SketchBio::Project*, int, bool);

	// COLOR EDITING functions:
    //changes object color (loops through 7 options)
    void changeObjectColor(SketchBio::Project*, int, bool);
    //loops through 3 object color variables on release
    void changeObjectColorVariable(SketchBio::Project*, int, bool);
    //toggles whether or not an object is set to be invisible
    void toggleObjectVisibility(SketchBio::Project*, int, bool);
    //toggles hide/show objects set to invisible
    void toggleShowInvisibleObjects(SketchBio::Project*, int, bool);

	// SPRING EDITING functions:
    //deletes nearest spring
    void deleteSpring(SketchBio::Project*, int, bool);
    //snaps the nearest spring to the C or N terminus
    void snapSpringToTerminus(SketchBio::Project*, int, bool);
    //press and hold for N terminus, release for C. used with snapSpringToTerminus
    void setTerminusToSnapSpring(SketchBio::Project*, int, bool);
    //creates a spring
    void createSpring(SketchBio::Project*, int, bool);
    //creates a transparent connector
    void createTransparentConnector(SketchBio::Project*, int, bool);
	//creates a measuring tape
	void createMeasuringTape(SketchBio::Project*, int, bool);

  // GRAB functions:
    //grabs nearest object, or if object too far, grabs world
    void grabObjectOrWorld(SketchBio::Project*, int, bool);
    //same as above but for spring
    void grabSpringOrWorld(SketchBio::Project*, int, bool);
    //selects the parent of nearest
    void selectParentOfCurrent(SketchBio::Project*, int, bool);
    //selects child of nearest 
    void selectChildOfCurrent(SketchBio::Project*, int, bool);
  
  // TRANSFORM functions:
    //removes object form world
    void deleteObject(SketchBio::Project*, int, bool);
    //adds a replicator with a copy of the nearest object
    void replicateObject(SketchBio::Project*, int, bool);
    //lets user set transform between two objects
    void setTransforms(SketchBio::Project*, int, bool);
    //lock a transform between two objects
    void lockTransforms(SketchBio::Project*, int, bool);
  

	// UTILITY functions:
    //resets viewpoint to 0,0
    void resetViewPoint(SketchBio::Project*, int, bool);
    //copies the selected object to clipboard
    void copyObject(SketchBio::Project*, int, bool);
    //pastes the object copied to clipboard
    void pasteObject(SketchBio::Project*, int, bool);
    //goes to the last undone state
    void redo(SketchBio::Project*, int, bool);
    //go backwards one operation, adds to redo stack
    void undo(SketchBio::Project*, int, bool);
    //turns collision checking on or off
    void toggleCollisionChecks(SketchBio::Project*, int, bool);
    //sets the spring physics on or off in the world
    void toggleSpringsEnabled(SketchBio::Project*, int, bool);
    //zooms on press
    void zoom(SketchBio::Project*, int, bool);
  
  // ANALOG functions:
    //rotates camera in y direction
    void rotateCameraYaw(SketchBio::Project*, int, double);
    //rotates camera in z direction
    void rotateCameraPitch(SketchBio::Project *, int, double);
    //changes the number of copies in structure replicator
    void setCrystalByExampleCopies(SketchBio::Project *, int, double);
    //move forward or backwards on animation timeline
    void moveAlongTimeline(SketchBio::Project *, int, double);
    // adjusts the rest length of the spring grabbed
    void adjustSpringRestLength(SketchBio::Project *, int, double);

  // NON CONTROL FUNCTIONS (but still useful)
  // Adds a save-to-xml undo state to the project for its current state
    void addUndoState(SketchBio::Project *project);
}

#endif // CONTROLFUNCTIONS_H
