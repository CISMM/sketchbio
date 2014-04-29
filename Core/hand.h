#ifndef HAND_H
#define HAND_H

#include <quat.h>

class vtkActor;
class vtkRenderer;

#include "sketchioconstants.h"
class TransformManager;
class SketchObject;
class Connector;
class WorldManager;

namespace SketchBio
{

class Hand
{
   public:
    Hand();
    Hand(TransformManager *t, WorldManager *w, SketchBioHandId::Type s, vtkRenderer *r);

    void init(TransformManager *t, WorldManager *w, SketchBioHandId::Type s, vtkRenderer *r);

    virtual ~Hand();

    void updatePositionAndOrientation();
    void updateScale();
    void getPosition(q_vec_type pos);
    void getOrientation(q_type orient);
    vtkActor *getHandActor();
    vtkActor *getShadowActor();

    // Functions for getting nearest object and distance to it
    SketchObject *getNearestObject();
    double getNearestObjectDistance();
    // Functions for getting nearest connector, which end of it is closer, and
    // the distance to it
    Connector *getNearestConnector(bool *atEnd1);
    double getNearestConnectorDistance();
    // Computes the nearest object and connector and the distances to them
    // Should be called every frame or at least before the getNearestX
    // functions are called
    void computeNearestObjectAndConnector();
    // Updates whatever is grabbed.  This function should be called
    // each frame
    void updateGrabbed();

    // The following functions grab and release the nearest object, nearest
    // connector or world
    void grabNearestObject();
    void grabNearestConnector();
    void grabWorld();
    void releaseGrabbed();

    // Increase and decrease the level within the group that is selected
    void selectSubObjectOfCurrent();
    void selectParentObjectOfCurrent();

    // Clears state for mode changes to avoid interference between modes'
    // operations
    void clearState();
    void clearNearestObject();
    void clearNearestConnector();

    // Operations for the outlines for the things selected by the hand
    bool isShowingSelectionOutline();
    void setShowingSelectionOutline(bool show);
    void setSelectionType(OutlineType::Type type);

   private:
    class HandImpl;
    HandImpl *impl;
};
};

#endif  // HAND_H
