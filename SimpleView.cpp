#include "ui_SimpleView.h"
#include "SimpleView.h"
#include <QDebug>

#include <vtkPolyDataMapper.h>
#include <vtkOBJReader.h>
#include <vtkSphereSource.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>

#define SCALE_BUTTON 5
#define ROTATE_BUTTON 13
#define HYDRA_SCALE_FACTOR 8.0f
#define HYDRA_LEFT_TRIGGER 2
#define HYDRA_RIGHT_TRIGGER 5
#define VRPN_ON false
#define SCALE_DOWN_FACTOR (.03125)
#define NUM_EXTRA_FIBERS 5

// Constructor
SimpleView::SimpleView() :
    tracker("Tracker0@localhost"),
    buttons("Tracker0@localhost"),
    analogRemote("Tracker0@localhost"),
    renderer(vtkSmartPointer<vtkRenderer>::New()),
    models(ModelManager()),
    timer(new QTimer()),
    world(&models,renderer.GetPointer()),
    transforms()
{
    this->ui = new Ui_SimpleView;
    this->ui->setupUi(this);
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
    int fiberSourceType = models.addObjectSource(objReader.GetPointer());

    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(sphereSource->GetOutputPort());


    int fiberModelType = models.addObjectType(fiberSourceType,1);

    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    int object1Id = world.addObject(fiberModelType,pos,orient,transforms.getWorldToEyeTransform());

    q_vec_set(pos,0,2/SCALE_DOWN_FACTOR,0);
    q_from_axis_angle(orient,0,1,0,Q_PI/22);
    int object2Id = world.addObject(fiberModelType,pos,orient,transforms.getWorldToEyeTransform());

    copies = new StructureReplicator(object1Id,object2Id,&world,&transforms);
    copies->setNumShown(5);


    vtkSmartPointer<vtkActor> leftHand = vtkSmartPointer<vtkActor>::New();
    leftHand->SetMapper(sphereMapper);
    vtkSmartPointer<vtkActor> rightHand = vtkSmartPointer<vtkActor>::New();
    rightHand->SetMapper(sphereMapper);
    vtkSmartPointer<vtkCamera> camera =
            vtkSmartPointer<vtkCamera>::New();
    camera->SetPosition(0, 0, 50);
    camera->SetFocalPoint(0, 0, 30);


    left = vtkSmartPointer<vtkTransform>::New();
    left->PostMultiply();
    transforms.getLeftTrackerTransformInEyeCoords(left);
    right = vtkSmartPointer<vtkTransform>::New();
    right->PostMultiply();
    transforms.getRightTrackerTransformInEyeCoords(right);
    leftHand->SetUserTransform(left);
    rightHand->SetUserTransform(right);


    // VTK Renderer
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
        q_from_two_vecs(rotation,beforeDVect,afterDVect);
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
        q_xyz_quat_type position;
        SketchObject *obj = world.getObject(0);
        obj->getPosition(position.xyz);
        obj->getOrientation(position.quat);
        q_vec_subtract(diff,afterLPos, beforeLPos);
        q_vec_scale(diff,HYDRA_SCALE_FACTOR/SCALE_DOWN_FACTOR,diff);
        q_vec_add(position.xyz,position.xyz,diff);
        q_type changeInOrientation;
        q_invert(beforeLOr,beforeLOr);
        q_mult(changeInOrientation,beforeLOr,afterLOr);
        q_mult(position.quat,changeInOrientation,position.quat);
        obj->setPosition(position.xyz);
        obj->setOrientation(position.quat);
        obj->recalculateLocalTransform();
    }
    if (analog[HYDRA_RIGHT_TRIGGER] > .5) {
        q_vec_type diff;
        q_xyz_quat_type position;
        SketchObject *obj = world.getObject(1);
        obj->getPosition(position.xyz);
        obj->getOrientation(position.quat);
        q_vec_subtract(diff,afterRPos, beforeRPos);
        q_vec_scale(diff,HYDRA_SCALE_FACTOR/SCALE_DOWN_FACTOR,diff);
        q_vec_add(position.xyz,position.xyz,diff);
        q_type changeInOrientation;
        q_invert(beforeROr,beforeROr);
        q_mult(changeInOrientation,beforeROr,afterROr);
        q_mult(position.quat,changeInOrientation,position.quat);
        obj->setPosition(position.xyz);
        obj->setOrientation(position.quat);
        obj->recalculateLocalTransform();
    }

    // set tracker locations
    transforms.getLeftTrackerTransformInEyeCoords(left);
    transforms.getRightTrackerTransformInEyeCoords(right);

    // update copies
    world.stepPhysics(16/1000.0);
//    copies->updateBaseline();

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
