#include "keyframe.h"

Keyframe::Keyframe() :
  colorMap(ColorMapType::SOLID_COLOR_RED,"modelNum"),
  visibleAfter(true),
  active(false)
{
    q_vec_set(position,0,0,0);
    q_from_axis_angle(orientation,1,0,0,0);
}

Keyframe::Keyframe(const q_vec_type pos, const q_vec_type absPos, const q_type orient, 
					ColorMapType::Type cMap, const QString& arr, int group_level, 
					SketchObject *parent, bool visibleA, bool isActive) :
  colorMap(cMap,arr),
  level(group_level),
  group(parent),
  visibleAfter(visibleA),
  active(isActive)
{
    q_vec_copy(position,pos);
	q_vec_copy(absolutePosition,absPos);
    q_copy(orientation,orient);
}

int Keyframe::getLevel() {
	return level;
}

SketchObject *Keyframe::getGroup() {
	return group;
}
