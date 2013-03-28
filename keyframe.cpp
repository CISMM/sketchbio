#include "keyframe.h"

Keyframe::Keyframe(const q_vec_type pos, const q_type orient, bool visible)
{
    q_vec_copy(position,pos);
    q_copy(orientation,orient);
    this->visible = visible;
}
