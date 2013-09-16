#include "keyframe.h"

Keyframe::Keyframe() :
  colorMap(ColorMapType::SOLID_COLOR_RED),
  arrayToColorBy(),
  visibleAfter(true),
  active(false)
{
    q_vec_set(position,0,0,0);
    q_from_axis_angle(orientation,1,0,0,0);
}

Keyframe::Keyframe(const q_vec_type pos, const q_type orient, ColorMapType::Type cMap,
                   const QString& array, bool visibleA, bool isActive) :
  colorMap(cMap),
  arrayToColorBy(array),
  visibleAfter(visibleA),
  active(isActive)
{
    q_vec_copy(position,pos);
    q_copy(orientation,orient);
}
