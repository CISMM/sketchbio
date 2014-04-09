#ifndef CONTROLFUNCTIONS_H
#define CONTROLFUNCTIONS_H

namespace SketchBio {
class Project;
}

namespace ControlFunctions
{
	// ANIMATION functions:
    void keyframeAll(SketchBio::Project*, int, bool);
    void toggleKeyframeObject(SketchBio::Project*, int, bool);
    void addCamera(SketchBio::Project*, int, bool);
    void showAnimationPreview(SketchBio::Project*, int, bool);

	// GROUP EDITING functions:
    void toggleGroupMembership(SketchBio::Project*, int, bool); // !! INCOMPLETE !!

	// COLOR EDITING functions:
    void changeObjectColor(SketchBio::Project*, int, bool);
    void changeObjectColorVariable(SketchBio::Project*, int, bool);
    void toggleObjectVisibility(SketchBio::Project*, int, bool);
    void toggleShowInvisibleObjects(SketchBio::Project*, int, bool);

	// SPRING EDITING functions:
    void deleteSpring(SketchBio::Project*, int, bool);
    void snapSpringToTerminus(SketchBio::Project*, int, bool); // !! INCOMPLETE !!
    void createSpring(SketchBio::Project*, int, bool); // !! INCOMPLETE !!
    void createTransparentConnector(SketchBio::Project*, int, bool);

  // GRAB functions:
  void grabObjectOrWorld(SketchBio::Project*, int, bool);
  void grabSpringOrWorld(SketchBio::Project*, int, bool);
  void selectParentOfCurrent(SketchBio::Project*, int, bool);
  void selectChildOfCurrent(SketchBio::Project*, int, bool);
  
  // TRANSFORM functions:
  void deleteObject(SketchBio::Project*, int, bool);
  void replicateObject(SketchBio::Project*, int, bool);
  void setTransforms(SketchBio::Project*, int, bool);
  

	// UTILITY functions:
  void resetViewPoint(SketchBio::Project*, int, bool);
  void copyObject(SketchBio::Project*, int, bool);
  void pasteObject(SketchBio::Project*, int, bool);
}

#endif // CONTROLFUNCTIONS_H
