#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <quat.h>

/*
 * This class holds the state information for a keyframe, the positions of each object that
 * have been explicitly set for that frame and the camera position for that frame if that has
 * been set.
 */
class Keyframe
{
public:
    Keyframe(); // creates a keyframe with all default values
    Keyframe(const q_vec_type pos, const q_type orient, bool visibleA);
    void getPosition(q_vec_type pos) const;
    void getOrientation(q_type orient) const;
    bool isVisibleAfter() const;
private:
    q_vec_type position;
    q_type orientation;
    bool visibleAfter;
};

inline void Keyframe::getPosition(q_vec_type pos) const {
    q_vec_copy(pos,position);
}
inline void Keyframe::getOrientation(q_type orient) const {
    q_copy(orient,orientation);
}
inline bool Keyframe::isVisibleAfter() const {
    return visibleAfter;
}

#endif // KEYFRAME_H
