#ifndef CONTROLFUNCTIONS_H
#define CONTROLFUNCTIONS_H

class SketchProject;

namespace ControlFunctions
{
	// ANIMATION functions:
	void keyframeAll(SketchProject*, int, bool);
	void toggleKeyframeObject(SketchProject*, int, bool);
	void addCamera(SketchProject*, int, bool);
	void showAnimationPreview(SketchProject*, int, bool);

	// GROUP EDITING functions:
	void toggleGroupMembership(SketchProject*, int, bool); // !! INCOMPLETE !!
	void copyObject(SketchProject*, int, bool);
	void pasteObject(SketchProject*, int, bool);

	// COLOR EDITING functions:
	void changeObjectColor(SketchProject*, int, bool);
	void changeObjectColorVariable(SketchProject*, int, bool);
	void toggleObjectVisibility(SketchProject*, int, bool);
	void toggleShowInvisibleObjects(SketchProject*, int, bool);

	// SPRING EDITING functions:
	void grabSpringOrWorld(SketchProject*, int, bool);
	void deleteSpring(SketchProject*, int, bool);
	void snapSpringToTerminus(SketchProject*, int, bool); // !! INCOMPLETE !!
	void createSpring(SketchProject*, int, bool); // !! INCOMPLETE !!
	void createTransparentConnector(SketchProject*, int, bool);

	// UTILITY functions:
	void resetViewPoint(SketchProject*, int, bool);
    void grabObjectOrWorld(SketchProject*, int, bool);
}

#endif // CONTROLFUNCTIONS_H