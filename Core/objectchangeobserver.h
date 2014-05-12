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
    // Called in the object's destructor.  Use to get rid of pointers to obj
	virtual void objectDeleted(SketchObject *obj) {}
    // Called when force applied to the object
    virtual void objectPushed(SketchObject *obj) {}
    // Called when a keyframe is added to the object
    virtual void objectKeyframed(SketchObject *obj, double time) {}
    // Called when an object is moved
    virtual void objectMoved(SketchObject *obj) {}
    // Called when an object's visibility changes
    virtual void objectVisibilityChanged(SketchObject *obj) {}
    // Called when a subobject is added.  All levels up the tree are notified.
    // if child->getParent() != parent then parent the the ancestor whose observers
    // are getting called now
    virtual void subobjectAdded(SketchObject *parent, SketchObject *child) {}
    // Called when a subobject is removed.  All observers of direct ancestors are notified.
    // if child->getParent() != parent then parent is an ancestor (grandparent, etc).
    virtual void subobjectRemoved(SketchObject *parent, SketchObject *child) {}
    // Called when the parent of an object changes
    virtual void parentChanged(SketchObject *obj) {}
};

#endif
