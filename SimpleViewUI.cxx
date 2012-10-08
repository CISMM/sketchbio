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
#define VRPN_ON false

// Constructor
SimpleView::SimpleView() :
    tracker("Tracker0@localhost"),
    buttons("Tracker0@localhost"),
    transforms(),
    timer(new QTimer())
{
    this->ui = new Ui_SimpleView;
    this->ui->setupUi(this);
    currentNumActors = 0;
    //    transforms.scaleWorldRelativeToRoom(0.125);
    if (!VRPN_ON) {
        q_xyz_quat_type pos;
        q_type ident = Q_ID_QUAT;
        q_vec_set(pos.xyz,1,0,1);
        q_vec_scale(pos.xyz,.5,pos.xyz);
        q_copy(pos.quat,ident);
        transforms.setLeftHandTransform(&pos);
        q_vec_set(pos.xyz,1,1,1);
        q_vec_scale(pos.xyz,.5,pos.xyz);
        transforms.setRightHandTransform(&pos);
    }

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
    buttons.register_change_handler((void *) this, handle_button);

    // model
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(.25/8);
    sphereSource->Update();
    vtkSmartPointer<vtkOBJReader> objReader =
            vtkSmartPointer<vtkOBJReader>::New();
    objReader->SetFileName("/Users/shawn/Documents/panda/biodoodle/models/1m1j.obj");
    objReader->Update();
    vtkSmartPointer<vtkPolyDataMapper> objMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    objMapper->SetInputConnection(objReader->GetOutputPort());
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
    vtkSmartPointer<vtkActor> fiber1Actor = vtkSmartPointer<vtkActor>::New();
    fiber1Actor->SetMapper(objMapper);
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


    //  vtkSmartPointer<vtkTransform> transform1a =
    //    vtkSmartPointer<vtkTransform>::New();
    //  transform1a->PostMultiply();
    //  transform1a->Scale(SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR);
    // fiber1Actor->SetUserTransform(transform1a);

    // vtkSmartPointer<vtkTransform> transform1b =
    //   vtkSmartPointer<vtkTransform>::New();
    // transform1b->PostMultiply();
    // transform1b->Scale(SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR);
    // transform1b->Translate(0,0,5);
    // fiber2Actor->SetUserTransform(transform1b);
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    addActor(fiber1Actor,pos,orient);
    q_vec_set(pos,0,1,0);
    q_from_axis_angle(orient,1,0,0,1);
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

    // VTK/Qt wedded
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

    // Set up action signals and slots
    connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

    connect(timer, SIGNAL(timeout()), this, SLOT(slot_frameLoop()));
    timer->start(16);

    if (VRPN_ON) {
        tracker.mainloop();
        buttons.mainloop();
    }

}

SimpleView::~SimpleView() {
    delete timer;
}

void SimpleView::slotExit() 
{
    qApp->exit();
}

void SimpleView::slot_frameLoop() {
    q_vec_type beforeVect, afterVect;
    double dist = transforms.getWorldDistanceBetweenHands();
    transforms.getLeftToRightHandWorldVector(beforeVect);
    if (VRPN_ON) {
        tracker.mainloop();
        buttons.mainloop();
    } else {
        q_xyz_quat_type pos;
        q_type q;
        q_from_axis_angle(q,1,0,0,.01);
        transforms.getRightHand(&pos);
        q_xform(pos.xyz,q,pos.xyz);
//        pos.xyz[1] += 0.01;
        transforms.setRightHandTransform(&pos);
    }
    double delta = transforms.getWorldDistanceBetweenHands() / dist;
    transforms.getLeftToRightHandWorldVector(afterVect);
    // possibly scale the world
    if (buttonDown[SCALE_BUTTON]) {
        transforms.scaleWorldRelativeToRoom(delta);
    }
    // possibly rotate the world
    if (buttonDown[ROTATE_BUTTON]||1) {
        q_type rotation;
        q_normalize(afterVect,afterVect);
        q_normalize(beforeVect,beforeVect);
        q_from_two_vecs(rotation,afterVect,beforeVect);
//        q_vec_type vec;
//        double d;
//        q_to_axis_angle(&vec[0],&vec[1],&vec[2],&d,rotation);
//        qDebug() << "Angle: " << d << endl;
        //        q_vec_scale(vec,.03125,vec);
//        q_from_euler(rotation,vec[0],vec[1],vec[2]);
        //        q_print(rotation);
        transforms.rotateWorldRelativeToRoom(rotation);

    }

    if (buttonDown[0]) {
        //        transforms.translateWorld(0,0,-.0005);
        q_type q;
        q_from_axis_angle(q,0,0,1,.01);
                transforms.translateWorldRelativeToRoom(0,0,-0.5);
        //        transforms.rotateWorldRelativeToRoom(q);
        //        transforms.translateWorld(0,-10,0);
    } else if (buttonDown[8]) {
        //        transforms.translateWorld(0,0,.0005);
        q_type q;
        q_from_axis_angle(q,0,0,1,-.01);
//                transforms.translateWorldRelativeToRoom(0,.05,0);
        //        transforms.rotateWorldRelativeToRoom(q);
        //        transforms.translateWorld(0,-10,0);
    }

    // set tracker locations
    transforms.getLeftTrackerTransformInEyeCoords(left);
    transforms.getRightTrackerTransformInEyeCoords(right);

    // reset transformation matrices
    vtkSmartPointer<vtkTransform> worldEye = vtkSmartPointer<vtkTransform>::New();
    transforms.getWorldToEyeTransform(worldEye);
    for (int i = 0; i < currentNumActors; i++) {
        qogl_matrix_type objTransform;
        q_xyz_quat_to_ogl_matrix(objTransform,&positions[i]);
        vtkSmartPointer<vtkTransform> trans = (vtkTransform *) actors[i]->GetUserTransform();
        trans->PostMultiply();
        trans->SetMatrix(objTransform);
        trans->Concatenate(worldEye);
    }
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
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->PostMultiply();
    actors[currentNumActors]->SetUserTransform(transform);
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
    data.xyz[0] = -t.pos[0];
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
