#include "ui_SimpleView.h"
#include "SimpleView.h"
#include <QDebug>

#include <vtkPolyDataMapper.h>
#include <vtkOBJReader.h>
#include <vtkSphereSource.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkProperty.h>

#define SCALE_BUTTON 5
#define ROTATE_BUTTON 13
#define HYDRA_SCALE_FACTOR 8.0f
#define HYDRA_LEFT_TRIGGER 2
#define HYDRA_RIGHT_TRIGGER 5
#define VRPN_ON true
#define SCALE_DOWN_FACTOR (.03125)
#define NUM_EXTRA_FIBERS 5
#define COLOR1 1,0,0
#define COLOR2 0,1,1

// Constructor
SimpleView::SimpleView() :
    tracker("Tracker0@localhost"),
    buttons("Tracker0@localhost"),
    analogRemote("Tracker0@localhost"),
    timer(new QTimer()),
    renderer(vtkSmartPointer<vtkRenderer>::New()),
    models(ModelManager()),
    transforms(),
    world(&models,renderer.GetPointer(),transforms.getWorldToEyeTransform())
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

    // models
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(4);
    sphereSource->Update();
    vtkSmartPointer<vtkOBJReader> objReader =
            vtkSmartPointer<vtkOBJReader>::New();
    objReader->SetFileName("/Users/shawn/Documents/QtProjects/RenderWindowUI/models/1m1j.obj");
    objReader->Update();
    int fiberSourceType = models.addObjectSource(objReader.GetPointer());
    int sphereSourceType = models.addObjectSource(sphereSource.GetPointer());


    int fiberModelType = models.addObjectType(fiberSourceType,1);
    int sphereModelType = models.addObjectType(sphereSourceType,TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*SCALE_DOWN_FACTOR);

    // creating objects
    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    ObjectId object1Id = world.addObject(fiberModelType,pos,orient);
    (*object1Id)->getActor()->GetProperty()->SetColor(COLOR1);
    objects.push_back(object1Id);

    q_vec_set(pos,0,2/SCALE_DOWN_FACTOR,0);
    q_from_axis_angle(orient,0,1,0,Q_PI/22);
    ObjectId object2Id = world.addObject(fiberModelType,pos,orient);
    (*object2Id)->getActor()->GetProperty()->SetColor(COLOR2);
    objects.push_back((object2Id));

    // creating springs
    q_vec_type p1 = {200,30,0}, p2 = {0,-30,0};
    SpringConnection *spring = new SpringConnection((*object1Id),(*object2Id),20,1,p1,p2);
    SpringId springId = world.addSpring(spring);
    q_vec_set(p1,0,30,0);
    q_vec_set(p2,-200,-30,0);
    spring = new SpringConnection((*object1Id),(*object2Id),20,.5,p1,p2);
    springId = world.addSpring(spring);

    // copying objects
    copies = new StructureReplicator(object1Id,object2Id,&world);
//    copies->setNumShown(5);

    // add objects for trackers
    q_vec_set(pos,0,0,0);
    q_from_axis_angle(orient,1,0,0,0);

    leftHand = world.addObject(sphereModelType,pos,orient);
    (*leftHand)->setDoPhysics(false);
//    (*leftHand)->allowLocalTransformUpdates(false);
    rightHand = world.addObject(sphereModelType,pos,orient);
    (*rightHand)->setDoPhysics(false);
//    (*rightHand)->allowLocalTransformUpdates(false);

    // camera setup
    vtkSmartPointer<vtkCamera> camera =
            vtkSmartPointer<vtkCamera>::New();
    camera->SetPosition(0, 0, 50);
    camera->SetFocalPoint(0, 0, 30);


    // set up tracker objects
    handleInput();


    // VTK Renderer
    renderer->SetActiveCamera(camera);

    // VTK/Qt wedded
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

    // Set up action signals and slots
    connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

    // start timer for frame update
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_frameLoop()));
    timer->start(16);
}

SimpleView::~SimpleView() {
    delete timer;
    delete copies;
}

void SimpleView::slotExit() 
{
    qApp->exit();
}

void SimpleView::handleInput() {
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
    updateTrackerObjectConnections();

    // set tracker locations
    updateTrackerPositions();
}

inline int getMinIdx(const q_vec_type vec) {
    if (Q_ABS(vec[Q_X]) < Q_ABS(vec[Q_Y])) {
        if (Q_ABS(vec[Q_X]) < Q_ABS(vec[Q_Z])) {
            return Q_X;
        } else {
            return Q_Z;
        }
    } else {
        if (Q_ABS(vec[Q_Y]) < Q_ABS(vec[Q_Z])) {
            return Q_Y;
        } else {
            return Q_Z;
        }
    }
}

void SimpleView::updateTrackerObjectConnections() {
    for (int i = 0; i < 2; i++) {
        int analogIdx, objIdx;
        ObjectId tracker;
        std::vector<SpringId> *springs;
        if (i == 0) {
            analogIdx = HYDRA_LEFT_TRIGGER;
            objIdx = 0;
            tracker = leftHand;
            springs = &lhand;
        } else if (i == 1) {
            analogIdx = HYDRA_RIGHT_TRIGGER;
            objIdx = 1;
            tracker = rightHand;
            springs = &rhand;
        }
        if (analog[analogIdx] > .1) {
            // either add springs or update their endpoints
            if (springs->size() == 0) { // add springs
                SketchObject *obj = (*objects[objIdx]);
                SketchObject *trackerObj = *tracker;
                q_vec_type oPos, tPos, vec;
                obj->getPosition(oPos);
                trackerObj->getPosition(tPos);
                q_vec_subtract(vec,tPos,oPos);
                q_vec_normalize(vec,vec);
                q_vec_type axis = Q_NULL_VECTOR;
                axis[getMinIdx(vec)] = 1;
                q_vec_type per1, per2;
                q_vec_cross_product(per1,axis,vec);
                q_vec_normalize(per1,per1);
                q_vec_cross_product(per2,per1,vec); // should already be length 1
#define VEC_LEN 5
                q_vec_scale(per1,VEC_LEN,per1);
                q_vec_scale(per2,VEC_LEN,per2);
                q_vec_type wPos1, wPos2;
                SpringId id;
                q_vec_add(wPos2,tPos,per1);
                q_vec_add(wPos1,oPos,per1);
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,analog[analogIdx],VEC_LEN);
                springs->push_back(id);
                q_vec_invert(per1,per1);
                q_vec_add(wPos2,tPos,per1);
                q_vec_add(wPos1,oPos,per1);
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,analog[analogIdx],VEC_LEN);
                springs->push_back(id);
                q_vec_add(wPos2,tPos,per2);
                q_vec_add(wPos1,oPos,per2);
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,analog[analogIdx],VEC_LEN);
                springs->push_back(id);
                q_vec_invert(per2,per2);
                q_vec_add(wPos2,tPos,per2);
                q_vec_add(wPos1,oPos,per2);
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,analog[analogIdx],VEC_LEN);
                springs->push_back(id);
#undef VEC_LEN
            } else { // update springs
                for (std::vector<SpringId>::iterator it = springs->begin(); it != springs->end(); it++) {
                    (*(*it))->setStiffness(analog[analogIdx]);
                }
            }
        } else {
            if (!springs->empty()) {
                // remove springs between model & tracker
                for (std::vector<SpringId>::iterator it = springs->begin(); it != springs->end(); it++) {
                    world.removeSpring(*it);
                }
                springs->clear();
            }
        }
    }
}

void SimpleView::updateTrackerPositions() {
    q_vec_type pos;
    q_type orient;
    transforms.getLeftTrackerPosInWorldCoords(pos);
    transforms.getLeftTrackerOrientInWorldCoords(orient);
    (*leftHand)->setPosition(pos);
    (*leftHand)->setOrientation(orient);
    transforms.getLeftTrackerTransformInEyeCoords((vtkTransform*)(*leftHand)->getActor()->GetUserTransform());
    transforms.getRightTrackerPosInWorldCoords(pos);
    transforms.getRightTrackerOrientInWorldCoords(orient);
    (*rightHand)->setPosition(pos);
    (*rightHand)->setOrientation(orient);
    transforms.getRightTrackerTransformInEyeCoords((vtkTransform*)(*rightHand)->getActor()->GetUserTransform());
}

void SimpleView::slot_frameLoop() {
    // input
    handleInput();

    // physics
    world.stepPhysics(16/1000.0);
    copies->updateTransform();

    // render
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
