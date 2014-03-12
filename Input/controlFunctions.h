#ifndef CONTROLFUNCTIONS_H
#define CONTROLFUNCTIONS_H

class SketchProject;

namespace ControlFunctions
{
	// ANIMATION functions:
	void keyframeAll(SketchProject*, int, bool);
	void toggleKeyframeObject(SketchProject*, int, bool);
	void addCamera(SketchProject*, int, bool); // !!! getXXXTrackerPosInWorldCoords() & getXXXTrackerOrientInWorldCoords() are not generalized !!!
	void showAnimationPreview(SketchProject*, int, bool);

	// GROUP EDITING functions:
	void toggleGroupMembership(SketchProject*, int, bool);
	void copyObject(SketchProject*, int, bool);
	void pasteObject(SketchProject*, int, bool); // !!! getXXXTrackerPosInWorldCoords() is not generalized !!!

	// COLOR EDITING functions:
	void changeObjectColor(SketchProject*, int, bool);
	void changeObjectColorVariable(SketchProject*, int, bool);
	void toggleObjectVisibility(SketchProject*, int, bool);
	void toggleShowInvisibleObjects(SketchProject*, int, bool);

	// SPRING EDITING functions:
	void grabSpringOrWorld(SketchProject*, int, bool); // !! INCOMPLETE !! - needs a "hand-oriented" state mechanism
	void deleteSpring(SketchProject*, int, bool);
	void snapSpringToTerminus(SketchProject*, int, bool); // !! INCOMPLETE !!
	void createSpring(SketchProject*, int, bool); // !! INCOMPLETE !! & !!! getXXXTrackerPosInWorldCoords() is not generalized !!!
	void createTransparentConnector(SketchProject*, int, bool); // !!! getXXXTrackerPosInWorldCoords() is not generalized !!!

	// UTILITY functions:
	void resetViewPoint(SketchProject*, int, bool);
}

#endif // CONTROLFUNCTIONS_H