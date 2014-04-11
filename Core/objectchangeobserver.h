#ifndef OBJECTCHANGEOBSERVER_H
#define OBJECTCHANGEOBSERVER_H

class SketchObject;

/*
 * This class is an observer that needs to be notified when the object's state
 * changes...
 * such as a force/torque applied, a keyframe added, or the object's
 * position/orientation changing
 */
class ObjectChangeObserver
{
   public:
    virtual ~ObjectChangeObserver() {}
    virtual void objectPushed(SketchObject *obj) {}
    virtual void objectKeyframed(SketchObject *obj, double time) {}
    virtual void objectMoved(SketchObject *obj) {}
    virtual void objectVisibilityChanged(SketchObject *obj) {}
    virtual void subobjectAdded(SketchObject *parent, SketchObject *child) {}
    virtual void subobjectRemoved(SketchObject *parent, SketchObject *child) {}
};

#endif
