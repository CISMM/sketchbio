// This file is a test for the Hand class added to SketchBio as
// part of the generalized input system.

// Author: Shawn Waldon

#include <iostream>
#include <cassert>

#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <modelmanager.h>
#include <sketchobject.h>
#include <modelinstance.h>
#include <objectgroup.h>
#include <connector.h>
#include <worldmanager.h>
#include <sketchproject.h>
#include <hand.h>

#include <test/TestCoreHelpers.h>

#define PRINT_ERROR(msg) \
    std::cout << __FILE__ << ":" << __LINE__ << msg << std::endl

int testComputeNearest();
int testSelectSubObject();

int main(int argc, char *argv[])
{
    return testComputeNearest() + testSelectSubObject();
}

int testComputeNearest() {
    vtkSmartPointer< vtkRenderer > renderer =
        vtkSmartPointer< vtkRenderer >::New();
    SketchBio::Project proj(renderer, ".");
    SketchModel *model = TestCoreHelpers::getCubeModel();
    proj.getModelManager().addModel(model);
    SketchBio::Hand &hand = proj.getHand(SketchBioHandId::LEFT);
    if (hand.getNearestObject() != NULL) {
        PRINT_ERROR("Incorrect nearest object");
        return 1;
    }
    if (hand.getNearestConnector(NULL) != NULL) {
        PRINT_ERROR("Incorrect nearest connector");
        return 1;
    }
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    SketchObject *obj = proj.getWorldManager().addObject(model, pos, orient);
    hand.computeNearestObjectAndConnector();
    if (hand.getNearestObject() != obj) {
        PRINT_ERROR("Incorrect object selected");
        return 1;
    }
    if (hand.getNearestConnector(NULL) != NULL) {
        PRINT_ERROR("Nearest connector wrong");
        return 1;
    }
    q_vec_type pos1 = {0,0,0};
    q_vec_type pos2 = {3,0,0};
    Connector *c = new Connector(NULL,NULL,pos1,pos2);
    proj.getWorldManager().addConnector(c);
    hand.computeNearestObjectAndConnector();
    if (hand.getNearestObject() != obj) {
        PRINT_ERROR("Incorrect object selected");
    }
    bool atEnd1;
    if (hand.getNearestConnector(&atEnd1) != c) {
        PRINT_ERROR("Nearest connector wrong");
        return 1;
    }
    if (!atEnd1) {
        PRINT_ERROR("Nearest connector end wrong");
        return 1;
    }
    return 0;
}

int testSelectSubObject() {
    vtkSmartPointer< vtkRenderer > renderer =
        vtkSmartPointer< vtkRenderer >::New();
    SketchBio::Project proj(renderer, ".");
    SketchModel *model = TestCoreHelpers::getCubeModel();
    proj.getModelManager().addModel(model);
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    ObjectGroup *grp = new ObjectGroup();
    SketchObject *objs[4];
    for (int i = 0; i < 4; ++i) {
        ModelInstance *inst = new ModelInstance(model);
        objs[i] = inst;
        inst->setPosAndOrient(pos,orient);
        pos[0] += 10;
        grp->addObject(inst);
    }
    proj.getWorldManager().addObject(grp);
    SketchBio::Hand &hand = proj.getHand(SketchBioHandId::LEFT);
    hand.computeNearestObjectAndConnector();
    if (hand.getNearestObject() != grp) {
        PRINT_ERROR("Nearest object incorrect");
        return 1;
    }
    hand.selectSubObjectOfCurrent();
    if (hand.getNearestObject() != objs[0]) {
        PRINT_ERROR("Nearest object incorrect");
        return 1;
    }
    hand.computeNearestObjectAndConnector();
    if (hand.getNearestObject() != objs[0]) {
        PRINT_ERROR("Nearest object incorrect");
        return 1;
    }
    hand.selectParentObjectOfCurrent();
    if (hand.getNearestObject() != grp) {
        PRINT_ERROR("Nearest object incorrect");
        return 1;
    }
    hand.computeNearestObjectAndConnector();
    if (hand.getNearestObject() != grp) {
        PRINT_ERROR("Nearest object incorrect");
        return 1;
    }
    return 0;
}
