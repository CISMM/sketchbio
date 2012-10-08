#include "ui_SimpleViewUI.h"
#include "SimpleViewUI.h"
 
#include <vtkPolyDataMapper.h>
#include <vtkOBJReader.h>
#include <vtkSphereSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkTransform.h>
 
#include "vtkSmartPointer.h"

#define SCALE_DOWN_FACTOR (.03125/4)
#define HYDRA_SCALE_FACTOR 4.0f
#define NUM_EXTRA_FIBERS 5

// Constructor
SimpleView::SimpleView() :
    tracker("Tracker0@localhost"),
    timer(new QTimer())
{
  this->ui = new Ui_SimpleView;
  this->ui->setupUi(this);

    tracker.register_change_handler((void *) this, handle_tracker_pos_quat);
 
  // model
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(.5);
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
  camera->SetPosition(0, 0, 20);
  camera->SetFocalPoint(0, 0, 0);


  vtkSmartPointer<vtkTransform> transform1a =
    vtkSmartPointer<vtkTransform>::New();
  transform1a->PostMultiply();
  transform1a->Scale(SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR);
// fiber1Actor->SetUserTransform(transform1a);

 vtkSmartPointer<vtkTransform> transform1b =
   vtkSmartPointer<vtkTransform>::New();
 transform1b->PostMultiply();
 transform1b->Scale(SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR);
 transform1b->Translate(0,0,5);
// fiber2Actor->SetUserTransform(transform1b);

 left = vtkSmartPointer<vtkTransform>::New();
 left->PostMultiply();
 left->Translate(-1,2,0);
 right = vtkSmartPointer<vtkTransform>::New();
 right->PostMultiply();
 right->Translate(1,2,0);
// leftHand->SetUserTransform(left);
// rightHand->SetUserTransform(right);
 fiber1Actor->SetUserTransform(left);
 fiber2Actor->SetUserTransform(right);
 
  // VTK Renderer
  vtkSmartPointer<vtkRenderer> renderer = 
      vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(fiber1Actor);
  renderer->AddActor(fiber2Actor);
//  renderer->AddActor(leftHand);
//  renderer->AddActor(rightHand);
  renderer->SetActiveCamera(camera);

  vtkSmartPointer<vtkTransform> prev = right.GetPointer();
  lInv = vtkSmartPointer<vtkTransform>::New();
  lInv->Identity();
  master = vtkSmartPointer<vtkTransform>::New();
  master->Identity();
  master->Concatenate(lInv);
  master->Concatenate(right);
  for (int i = 0; i < NUM_EXTRA_FIBERS; i++) {
      vtkSmartPointer<vtkTransform> next = vtkSmartPointer<vtkTransform>::New();
      next->Identity();
      next->Concatenate(prev);
      next->Concatenate(master);

      vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
      actor->SetUserTransform(next);
      actor->SetMapper(objMapper);
      renderer->AddActor(actor);

      prev = next.GetPointer();
  }
 
  // VTK/Qt wedded
  this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);
 
  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

  connect(timer, SIGNAL(timeout()), this, SLOT(slot_vrpnLoop()));
  timer->start(16);

}

SimpleView::~SimpleView() {
    delete timer;
}

void SimpleView::slotExit() 
{
  qApp->exit();
}

void SimpleView::slot_vrpnLoop() {
    tracker.mainloop();
    this->ui->qvtkWidget->GetRenderWindow()->Render();
}

void SimpleView::setLeftPos(q_xyz_quat_type *xyzQuat) {
    left->Identity();
    left->PostMultiply();
    left->Scale(SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR);
    left->RotateWXYZ(xyzQuat->quat[Q_W]*180/Q_PI,&(xyzQuat->quat[Q_X]));
    left->Translate(xyzQuat->xyz);
    lInv->Identity();
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    left->GetInverse(mat);
    lInv->Concatenate(mat);
}

void SimpleView::setRightPos(q_xyz_quat_type *xyzQuat) {
    right->Identity();
    right->PostMultiply();
    right->Scale(SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR,SCALE_DOWN_FACTOR);
    right->RotateWXYZ(xyzQuat->quat[Q_W]*180/Q_PI,&(xyzQuat->quat[Q_X]));
    right->Translate(xyzQuat->xyz);
}

//####################################################################################
// VRPN callback functions
//####################################################################################


void VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
    SimpleView *view = (SimpleView *) userdata;
    // changes coordinates to OpenGL coords, switching y & z and negating y
    q_xyz_quat_type type;
    q_copy(type.quat,t.quat);
    q_vec_scale(type.xyz,HYDRA_SCALE_FACTOR,t.pos);
    if (t.sensor == 0) {
        view->setLeftPos(&type);
    } else {
        view->setRightPos(&type);
    }
}
