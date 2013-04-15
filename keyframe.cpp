#include "keyframe.h"

Keyframe::Keyframe() {
    q_vec_set(position,0,0,0);
    q_from_axis_angle(orientation,1,0,0,0);
    this->visibleAfter = true;
}

Keyframe::Keyframe(const q_vec_type pos, const q_type orient, bool visibleA)
{
    q_vec_copy(position,pos);
    q_copy(orientation,orient);
    this->visibleAfter = visibleA;
}
