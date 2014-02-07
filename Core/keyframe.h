#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <quat.h>

#include <QString>

#include "colormaptype.h"
#include "sketchobject.h"

/*
 * This class holds the state information for a keyframe, the positions of each object that
 * have been explicitly set for that frame and the camera position for that frame if that has
 * been set.
 */
class Keyframe
{
public:
    Keyframe(); // creates a keyframe with all default values
    Keyframe(const q_vec_type pos, const q_vec_type absPos, 
				const q_type orient, const q_type absOr,
				ColorMapType::Type cMap, const QString& arr, int group_level, 
				SketchObject *parent, bool visibleA, bool isActive);
    void getPosition(q_vec_type pos) const;
	void getAbsolutePosition(q_vec_type pos) const;
    void getOrientation(q_type orient) const;
	void getAbsoluteOrientation(q_type orient) const;
    ColorMapType::Type getColorMapType() const;
    const QString& getArrayToColorBy() const;
    const ColorMapType::ColorMap& getColorMap() const;
    bool isVisibleAfter() const;
    bool isActive() const;
	int getLevel();
	SketchObject *getGroup();
private:
    q_vec_type position;
	q_vec_type absolutePosition;
    q_type orientation;
	q_type absoluteOrientation;
    ColorMapType::ColorMap colorMap;
    bool visibleAfter;
    bool active;
	SketchObject *group;
	int level;
};

inline void Keyframe::getPosition(q_vec_type pos) const
{
    q_vec_copy(pos,position);
}
inline void Keyframe::getAbsolutePosition(q_vec_type pos) const
{
    q_vec_copy(pos,absolutePosition);
}
inline void Keyframe::getOrientation(q_type orient) const
{
    q_copy(orient,orientation);
}
inline void Keyframe::getAbsoluteOrientation(q_type orient) const
{
    q_copy(orient,absoluteOrientation);
}
inline ColorMapType::Type Keyframe::getColorMapType() const
{
    return colorMap.first;
}
inline const QString& Keyframe::getArrayToColorBy() const
{
    return colorMap.second;
}
inline const ColorMapType::ColorMap& Keyframe::getColorMap() const
{
    return colorMap;
}
inline bool Keyframe::isVisibleAfter() const
{
    return visibleAfter;
}
inline bool Keyframe::isActive() const
{
    return active;
}

#endif // KEYFRAME_H
