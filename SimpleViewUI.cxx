#include "ui_SimpleViewUI.h"
#include "SimpleViewUI.h"
#include <QDebug>

#include <vtkPolyDataMapper.h>
#include <vtkOBJReader.h>
#include <vtkSphereSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkTransform.h>

#include "vtkSmartPointer.h"

#define SCALE_BUTTON 5
#define ROTATE_BUTTON 13
#define HYDRA_SCALE_FACTOR 4.0f
#define HYDRA_LEFT_TRIGGER 2
#define HYDRA_RIGHT_TRIGGER 5
#define VRPN_ON true
#define SCALE_DOWN_FACTOR (.03125)
#define NUM_EXTRA_FIBERS 5

// Constructor
SimpleView::SimpleView() :
    tracker("Tracker0@localhost"),
    buttons("Tracker0@localhost"),
    analogRemote("Tracker0@localhost"),
    timer(new QTimer()),
    transforms()
{
    this->ui = new Ui_SimpleView;
    this->ui->setupUi(this);
    currentNumActors = 0;
    transforms.scaleWorldRelativeToRoom(SCALE_DOWN_FACTOR);
    if (!VRPN_ON) {
        q_xyz_quat_type pos;
        q_type ident = Q_ID_QUAT;
        q_vec_set(pos.xyz,.5,0,.5);
        q_vec_scale(pos.xyz,.5,pos.xyz);
        q_copy(pos.quat,ident);
        transforms.setLeftHandTransform(&pos);
        q_vec_set(pos.xyz,0,0,1);
        q_vec_scale(pos.xyz,.5,pos.xyz);
        transforms.setRightHandTransform(&pos);
    }

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
    buttons.register_change_handler((void *) this, handle_button);
    analogRemote.register_change_handler((void *) this, handle_analogs);

    for (int i = 0; i < NUM_HYDRA_BUTTONS; i++) {
        buttonDown[i] = false;
    }
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analog[i] = 0.0;
    }

    // model
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(.25/8);
    sphereSource->Update();
    vtkSmartPointer<vtkOBJReader> objReader =
            vtkSmartPointer<vtkOBJReader>::New();
    objReader->SetFileName("/Users/shawn/Documents/QtProjects/RenderWindowUI/models/1m1j.obj");
    objReader->Update();
    vtkSmartPointer<vtkPolyDataMapper> objMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    objMapper->SetInputConnection(objReader->GetOutputPort());

    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(sphereSource->GetOutputPort());


    vtkSmartPointer<vtkActor> fiber1Actor = vtkSmartPointer<vtkActor>::New();
    fiber1Actor->SetMapper(objMapper);
    double bb[6];
    fiber1Actor->GetBounds(bb);
    qDebug() << bb[0] << bb[1] << bb[2] << bb[3] << bb[4] << bb[5];
    vtkSmartPointer<vtkActor> fiber2Actor = vtkSmartPointer<vtkActor>::New();
    fiber2Actor->SetMapper(objMapper);
    vtkSmartPointer<vtkActor> leftHand = vtkSmartPointer<vtkActor>::New();
    leftHand->SetMapper(sphereMapper);
    vtkSmartPointer<vtkActor> rightHand = vtkSmartPointer<vtkActor>::New();
    rightHand->SetMapper(sphereMapper);
    vtkSmartPointer<vtkCamera> camera =
            vtkSmartPointer<vtkCamera>::New();
    camera->SetPosition(0, 0, 50);
    camera->SetFocalPoint(0, 0, 30);

    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    addActor(fiber1Actor,pos,orient);
    q_vec_set(pos,0,2,0);
    q_from_axis_angle(orient,0,1,0,Q_PI/22);
    addActor(fiber2Actor,pos,orient);

    left = vtkSmartPointer<vtkTransform>::New();
    left->PostMultiply();
    left->Translate(-1,2,0);
    right = vtkSmartPointer<vtkTransform>::New();
    right->PostMultiply();
    right->Translate(1,2,0);
    leftHand->SetUserTransform(left);
    rightHand->SetUserTransform(right);


    // VTK Renderer
    vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(fiber1Actor);
    renderer->AddActor(fiber2Actor);
    renderer->AddActor(leftHand);
    renderer->AddActor(rightHand);
    renderer->SetActiveCamera(camera);

    copies = new StructureReplicator(fiber1Actor, fiber2Actor,renderer,objMapper, &transforms);
    copies->setNumShown(NUM_EXTRA_FIBERS);

    // VTK/Qt wedded
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

    // Set up action signals and slots
    connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

    connect(timer, SIGNAL(timeout()), this, SLOT(slot_frameLoop()));
    timer->start(16);
    if (VRPN_ON) {
        tracker.mainloop();
        buttons.mainloop();
        analogRemote.mainloop();
    }

}

SimpleView::~SimpleView() {
    delete timer;
    delete copies;
}

void SimpleView::slotExit() 
{
    qApp->exit();
}

void SimpleView::slot_frameLoop() {
    q_vec_type beforeDVect, afterDVect, beforeLPos, beforeRPos, afterLPos, afterRPos;
    q_type beforeLOr, afterLOr, beforeROr, afterROr;
    double dist = transforms.getWorldDistanceBetweenHands();
    transforms.getLeftTrackerPosInWorldCoords(beforeLPos);
    transforms.getRightTrackerPosInWorldCoords(beforeRPos);
    transforms.getLeftTrackerOrientInWorldCoords(beforeLOr);
    transforms.getRightTrackerOrientInWorldCoords(beforeROr);
    transforms.getLeftToRightHandWorldVector(beforeDVect);
    if (VRPN_ON) {
        tracker.mainloop();
        buttons.mainloop();
        analogRemote.mainloop();
    } else {
        q_xyz_quat_type pos;
        q_type q;
        q_from_axis_angle(q,0,1,0,.01);
        transforms.getRightHand(&pos);
        q_xform(pos.xyz,q,pos.xyz);
        q_mult(pos.quat,q,pos.quat);
//        pos.xyz[1] += 0.01;
        transforms.setRightHandTransform(&pos);
    }
    double delta = transforms.getWorldDistanceBetweenHands() / dist;
    transforms.getLeftToRightHandWorldVector(afterDVect);
    transforms.getLeftTrackerPosInWorldCoords(afterLPos);
    transforms.getRightTrackerPosInWorldCoords(afterRPos);
    transforms.getLeftTrackerOrientInWorldCoords(afterLOr);
    transforms.getRightTrackerOrientInWorldCoords(afterROr);
    // possibly scale the world
    if (buttonDown[SCALE_BUTTON]) {
        transforms.scaleWorldRelativeToRoom(delta);
    }
    // possibly rotate the world
    if (buttonDown[ROTATE_BUTTON]) {
        q_type rotation;
        q_normalize(afterDVect,afterDVect);
        q_normalize(beforeDVect,beforeDVect);
        q_from_two_vecs(rotation,afterDVect,beforeDVect);
        transforms.rotateWorldRelativeToRoomAboutLeftTracker(rotation);

    }

    if (buttonDown[0]) {
        q_type q;
        q_from_axis_angle(q,0,0,1,.01);
        transforms.translateWorldRelativeToRoom(0,0,-0.5);
    } else if (buttonDown[8]) {
        q_type q;
        q_from_axis_angle(q,0,0,1,-.01);
        transforms.translateWorldRelativeToRoom(0,0,0.5);
    }

    // move fibers
    // HARD CODED----FIX AFTER DEMO
    if (analog[HYDRA_LEFT_TRIGGER] > .5) {
        q_vec_type diff;
        q_vec_subtract(diff,afterLPos, beforeLPos);
        q_vec_scale(diff,HYDRA_SCALE_FACTOR,diff);
        q_vec_add(positions[0].xyz,positions[0].xyz,diff);
        q_type changeInOrientation;
        q_invert(beforeLOr,beforeLOr);
        q_mult(changeInOrientation,beforeLOr,afterLOr);
        q_mult(positions[0].quat,changeInOrientation,positions[0].quat);
    }
    if (analog[HYDRA_RIGHT_TRIGGER] > .5) {
        q_vec_type diff;
        q_vec_subtract(diff,afterRPos, beforeRPos);
        q_vec_scale(diff,HYDRA_SCALE_FACTOR,diff);
        q_vec_add(positions[1].xyz,positions[1].xyz,diff);
        q_type changeInOrientation;
        q_invert(beforeROr,beforeROr);
        q_mult(changeInOrientation,beforeROr,afterROr);
        q_mult(positions[1].quat,changeInOrientation,positions[1].quat);
    }

    // set tracker locations
    transforms.getLeftTrackerTransformInEyeCoords(left);
    transforms.getRightTrackerTransformInEyeCoords(right);


    // reset transformation matrices
    vtkSmartPointer<vtkTransform> worldEye = vtkSmartPointer<vtkTransform>::New();
    transforms.getWorldToEyeTransform(worldEye);
    for (int i = 0; i < currentNumActors; i++) {
        vtkSmartPointer<vtkTransform> trans = (vtkTransform *) actors[i]->GetUserTransform();
        trans->Identity();
        trans->PostMultiply();
        trans->Translate(actorCenterOffset[i]);
        double xyz[3],angle;
        q_to_axis_angle(&xyz[0],&xyz[1],&xyz[2],&angle,positions[i].quat);
        trans->RotateWXYZ(angle*180/Q_PI,xyz);
        trans->Translate(positions[i].xyz[0]*1/SCALE_DOWN_FACTOR,
                         positions[i].xyz[1]*1/SCALE_DOWN_FACTOR,
                         positions[i].xyz[2]*1/SCALE_DOWN_FACTOR);
        trans->Concatenate(worldEye);
    }

    // update copies
    copies->updateBaseline();

    this->ui->qvtkWidget->GetRenderWindow()->Render();
}

void SimpleView::setLeftPos(q_xyz_quat_type *newPos) {
    transforms.setLeftHandTransform(newPos);
}

void SimpleView::setRightPos(q_xyz_quat_type *newPos) {
    transforms.setRightHandTransform(newPos);
}

void SimpleView::setButtonState(int buttonNum, bool buttonPressed) {
    buttonDown[buttonNum] = buttonPressed;
}

void SimpleView::setAnalogStates(const double state[]) {
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analog[i] = state[i];
    }
}

void SimpleView::addActor(vtkActor *actor, q_vec_type position, q_type orientation) {
    actors[currentNumActors] = actor;
    double bounds[6]; // bounding box
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->PostMultiply();
    actors[currentNumActors]->SetUserTransform(transform);
    actors[currentNumActors]->GetBounds(bounds);
    actorCenterOffset[currentNumActors][0] = -(bounds[0] + bounds[1])/2;
    actorCenterOffset[currentNumActors][1] = -(bounds[2] + bounds[3])/2;
    actorCenterOffset[currentNumActors][2] = -(bounds[4] + bounds[5])/2;
    q_vec_copy(positions[currentNumActors].xyz,position);
    q_copy(positions[currentNumActors].quat,orientation);
    currentNumActors++;
}

//####################################################################################
// VRPN callback functions
//####################################################################################


void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
    SimpleView *view = (SimpleView *) userdata;
    q_xyz_quat_type data;
    // changes coordinates to OpenGL coords, switching y & z and negating y
    data.xyz[0] = t.pos[0];
    data.xyz[1] = -t.pos[2];
    data.xyz[2] = t.pos[1];
    q_copy(data.quat,t.quat);
    // set the correct hand's position
    if (t.sensor == 0) {
        view->setLeftPos(&data);
    } else {
        view->setRightPos(&data);
    }
}

void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b) {
    SimpleView *view = (SimpleView *) userdata;
    view->setButtonState(b.button,b.state);
}

void VRPN_CALLBACK handle_analogs(void *userdata, const vrpn_ANALOGCB a) {
    SimpleView *view = (SimpleView *) userdata;
    if (a.num_channel != NUM_HYDRA_ANALOGS) {
        qDebug() << "We have problems!";
    }
    view->setAnalogStates(a.channel);
}
