#include "ui_SimpleView.h"
#include "SimpleView.h"
#include <QDebug>
#include <QInputDialog>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QThread>

#include <stdexcept>

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
#define BOND_SPRING_CONSTANT .5
#define TIMESTEP (16/1000.0)

#define NUM_COLORS (6)
static double COLORS[][3] =
  {  { 1.0, 0.7, 0.7 },
     { 0.7, 1.0, 0.8 },
     { 0.7, 0.7, 1.0 },
     { 1.0, 1.0, 0.7 },
     { 1.0, 0.7, 1.0 },
     { 0.7, 1.0, 1.0 } };

// Constructor
SimpleView::SimpleView(bool load_fibrin, bool fibrin_springs, bool do_replicate) :
    tracker("Tracker0@localhost"),
    buttons("Tracker0@localhost"),
    analogRemote("Tracker0@localhost"),
    timer(new QTimer()),
    renderer(vtkSmartPointer<vtkRenderer>::New()),
    models(ModelManager()),
    transforms(),
    world(&models,renderer.GetPointer(),transforms.getWorldToEyeTransform()),
    copies(NULL)
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
    int sphereSourceType = models.addObjectSource(sphereSource.GetPointer());
    int sphereModelType = models.addObjectType(sphereSourceType,
                                               TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE*SCALE_DOWN_FACTOR);

    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
  if (load_fibrin) {
    ObjectId object1Id = addObject("./models/1m1j.obj");
    ObjectId object2Id = addObject("./models/1m1j.obj");

   if (fibrin_springs) {
    // creating springs
    q_vec_type p1 = {200,-30,0}, p2 = {0,-30,0};
    SpringConnection *spring = new SpringConnection((*object1Id),(*object2Id),0,BOND_SPRING_CONSTANT,p1,p2);
    SpringId springId = world.addSpring(spring);

    q_vec_set(p1,0,-30,0);
    q_vec_set(p2,200,-30,0);
    spring = new SpringConnection((*object1Id),(*object2Id),0,BOND_SPRING_CONSTANT,p1,p2);
    springId = world.addSpring(spring);
   }

    // Replicate objects
   if (do_replicate) {
    copies = new StructureReplicator(object1Id,object2Id,&world);
    copies->setNumShown(5);
   }
  }

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
        transforms.scaleWithLeftTrackerFixed(delta);
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

/*
 * This function takes a q_vec_type and returns the index of
 * the component with the minimum magnitude.
 */
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

/*
 * This method updates the springs connecting the trackers and the objects in the world...
 * we may need to apply a torque on the objects at each timestep to match the change in tracker
 * orientation... torques from the springs are VERY unintiuative
 */
void SimpleView::updateTrackerObjectConnections() {
    for (int i = 0; i < 2; i++) {
        int analogIdx, objIdx;
        ObjectId tracker;
        std::vector<SpringId> *springs;
        // select left or right
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
        // if they are gripping the trigger
        if (analog[analogIdx] > .1) {
            // if we do not have springs yet add them
            if (springs->size() == 0) { // add springs
                SketchObject *obj = (*objects[objIdx]);
                SketchObject *trackerObj = *tracker;
                q_vec_type oPos, tPos, vec;
                obj->getPosition(oPos);
                trackerObj->getPosition(tPos);
                q_vec_subtract(vec,tPos,oPos);
                q_vec_normalize(vec,vec);
                q_vec_type axis = Q_NULL_VECTOR;
                axis[getMinIdx(vec)] = 1; // this gives an axis that is guaranteed not to be
                                    // parallel to vec
                q_vec_type per1, per2; // create two perpendicular unit vectors
                q_vec_cross_product(per1,axis,vec);
                q_vec_normalize(per1,per1);
                q_vec_cross_product(per2,per1,vec); // should already be length 1
#define OBJECT_SIDE_LEN 200
#define TRACKER_SIDE_LEN 200
                // create scaled perpendicular vectors
                q_vec_type oPer1, oPer2, tPer1, tPer2;
                q_vec_scale(oPer1,OBJECT_SIDE_LEN,per1);
                q_vec_scale(oPer2,OBJECT_SIDE_LEN,per2);
                q_vec_scale(tPer1,TRACKER_SIDE_LEN,per1);
                q_vec_scale(tPer2,TRACKER_SIDE_LEN,per2);
                q_vec_type wPos1, wPos2;
                // create springs and add them
                // first spring --defined along the "x" axis (per1)
                SpringId id;
                q_vec_add(wPos2,tPos,tPer1);
                q_vec_add(wPos1,oPos,oPer1);
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,
                                     analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                springs->push_back(id);
                // second spring --defined as rotated 120 degrees about "z" axis.
                // coordinates in terms of x and y: (-1/2x, sqrt(3)/2y)
                q_vec_scale(oPer1,-.5,oPer1);
                q_vec_scale(tPer1,-.5,tPer1);
                q_vec_scale(oPer2,sqrt(3.0)/2,oPer2);
                q_vec_scale(tPer2,sqrt(3.0)/2,tPer2);
                q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                q_vec_add(wPos2,wPos2,tPer2); // + sqrt(3)/2 y
                q_vec_add(wPos1,oPos,oPer1); // origin - 1/2 x
                q_vec_add(wPos1,wPos1,oPer2); // + sqrt(3)/2 y
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,
                                     analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                springs->push_back(id);
                // third spring --defined as rotated 240 degrees about "z" axis.
                // coordinates in terms of x and y: (-1/2x, -sqrt(3)/2y)
                q_vec_invert(oPer2,oPer2);
                q_vec_invert(tPer2,tPer2);
                q_vec_add(wPos2,tPos,tPer1); // origin - 1/2 x
                q_vec_add(wPos2,wPos2,tPer2); // - sqrt(3)/2 y
                q_vec_add(wPos1,oPos,oPer1); // origin - 1/2 x
                q_vec_add(wPos1,wPos1,oPer2); // - sqrt(3)/2 y
                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,
                                     analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
                springs->push_back(id);
//                q_vec_invert(oPer2,oPer2);
//                q_vec_invert(tPer2,tPer2);
//                q_vec_add(wPos2,tPos,tPer2);
//                q_vec_add(wPos1,oPos,oPer2);
//                id = world.addSpring(objects[objIdx],tracker,wPos1,wPos2,
//                                     analog[analogIdx],abs(OBJECT_SIDE_LEN-TRACKER_SIDE_LEN));
//                springs->push_back(id);
#undef OBJECT_SIDE_LEN
#undef TRACKER_SIDE_LEN
            } else { // update springs stiffness if they are already there
                // may need to update endpoints too... not sure
                for (std::vector<SpringId>::iterator it = springs->begin(); it != springs->end(); it++) {
                    (*(*it))->setStiffness(analog[analogIdx]);
                }
            }
        } else {
            if (!springs->empty()) { // if we have springs and they are no longer gripping the trigger
                // remove springs between model & tracker
                for (std::vector<SpringId>::iterator it = springs->begin(); it != springs->end(); it++) {
                    world.removeSpring(*it);
                }
                springs->clear();
            }
        }
    }
}

/*
 * Updates the positions and transformations of the tracker objects.
 */
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

/*
 * The method called once per frame to update things and redraw
 */
void SimpleView::slot_frameLoop() {
    // input
    handleInput();

    // physics
    world.stepPhysics(TIMESTEP);
    if (copies != NULL) {
	copies->updateTransform();
    }

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

ObjectId SimpleView::addObject(QString name)
{
    vtkSmartPointer<vtkOBJReader> objReader =
            vtkSmartPointer<vtkOBJReader>::New();
    objReader->SetFileName(name.toStdString().c_str());
    objReader->Update();
    int fiberSourceType = models.addObjectSource(objReader.GetPointer());
    int fiberModelType = models.addObjectType(fiberSourceType,1);

    q_vec_type pos = Q_NULL_VECTOR;
    q_type orient = Q_ID_QUAT;
    int myIdx = objects.size();
    q_vec_set(pos,0,2*myIdx/SCALE_DOWN_FACTOR,0);
    ObjectId objectId = world.addObject(fiberModelType,pos,orient);
    (*objectId)->getActor()->GetProperty()->SetColor(COLORS[myIdx%NUM_COLORS]);
    objects.push_back(objectId);

    return objectId;
}

bool SimpleView::addObjects(QVector<QString> names)
{
  int i;
  for (i = 0; i < names.size(); i++) {
    // XXX Check for error here and return false if one found.
      // notes: no good cross-platform way to check if file exists
      // VTK prints out errors but does not throw anything, so we must
      // do error checking before calling this
      addObject(names[i]);
  }

  return true;
}

void SimpleView::openOBJFile()
{
    // Ask the user for the name of the file to open.
    QString fn = QFileDialog::getOpenFileName(this,
                                              tr("Open OBJ file"),
                                              "./",
                                              tr("OBJ Files (*.obj)"));

    // Open the file for reading.
    if (fn.length() > 0) {
	printf("Loading %s\n", fn.toStdString().c_str());
	addObject(fn);
    }
}

void SimpleView::importPDBId()
{
    // Ask the user for the ID of the PDB file to open.
    bool ok;
    QString text = QInputDialog::getText(this, tr("Specify molecule"),
                                         tr("PDB ID:"), QLineEdit::Normal,
                                         "1M1J", &ok);
    if (ok && !text.isEmpty()) {
	// Ask the user where to put the output file.
	QString fn = QFileDialog::getExistingDirectory ( this,
					"Choose a directory to save to");

	if (fn.length() > 0) {
	  printf("Importing %s from PDB\n", text.toStdString().c_str());

	  // Start the pymol process and then write the commands to it
	  // that will cause it to load the PDB file, generate a surface
	  // for it, and then save the file.
	  QProcess pymol;
	  // Combine stderr with stdout and send back back together.
	  pymol.setProcessChannelMode(QProcess::MergedChannels);
	  // -c: Don't display graphics; -p: Read commands from stdin
	  pymol.start("pymol", QStringList() << "-c" << "-p");
	  if (!pymol.waitForStarted()) {
		QMessageBox::warning(NULL, "Could not run pymol to import molecule ", text);;
	  } else {
	    // Send the commands to Pymol to make it do what we want.
	    QString cmd;
	    cmd = "load http://www.pdb.org/pdb/download/downloadFile.do?fileFormat=pdb&compression=NO&structureId=" + text + "\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());
	    cmd = "save " + fn + "/" + text + ".pdb\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());
	    cmd = "hide all\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());
	    cmd = "show surface\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());
	    cmd = "save " + fn + "/" + text + ".obj\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());

	    // If we don't print here and then wait, there is some sort of
	    // race condition with the file saving and it does not happen
	    // because we quit too soon.
	    // This can hang forever, need a timeout in case things lock
	    // up.
	    cmd = "print \"readytoquit\"\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());

	    // If we don't wait for written, the reads hang forever.
	    pymol.waitForBytesWritten(-1);
	    qint64 linelength = 0;
	    QByteArray full;
	    do {
		// If we don't wait for Ready, we get no bytes
		if (pymol.waitForReadyRead(50)) {
		  QByteArray next = pymol.readAll();
		  full.append(next);
		}
	    } while ( (linelength >= 0) && !full.contains("readytoquit"));

	    // Waiting for the print to appear doesn't always fix the
	    // problem.  We now wait some amount longer to let the write
	    // hopefully finish.  Not sure how long to wait here.  It is
	    // a race condition.
	    // XXX Wait here until the file shows up, rather than doing
	    // a print and then waiting until the print shows up and then
	    // some random time longer.
	    // XXX I submitted a bug report to the Pymol user list to try
	    // and get this race condition fixed.  Remove all of the hanky-
	    // panky about reading and printing and waiting when it is
	    // fixed.
	    vrpn_SleepMsecs(5000);

	    cmd = "quit\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());

	    // Time out after 2 minutes if the process does not exit
	    // on its own by then.
	    if (!pymol.waitForFinished(120000)) {
		QMessageBox::warning(NULL, "Could not complete pymol import", text);
	    }

	    // Read all of the output from the program and then
	    // see if we got what we expected.  We look for reports
	    // that it saved the file and that it exited normally.
	    // XXX This does not guarantee that it saved the file.
	    // can we check return codes in pymol and print an error?
	    QByteArray result = pymol.readAll();
	    if ( !result.contains("normal program termination.") ) {
		QMessageBox::warning(NULL, "Error while importing", text);
		printf("Python problem:\n%s\n", result.data());
	    }
	  }

	}
    }
}

//####################################################################################
// VRPN callback functions
//####################################################################################


void VRPN_CALLBACK SimpleView::handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
    SimpleView *view = (SimpleView *) userdata;
    q_xyz_quat_type data;
    // changes coordinates to OpenGL coords, switching y & z and negating y
    data.xyz[Q_X] = t.pos[Q_X];
    data.xyz[Q_Y] = -t.pos[Q_Z];
    data.xyz[Q_Z] = t.pos[Q_Y];
    data.quat[Q_X] = t.quat[Q_X];
    data.quat[Q_Y] = -t.quat[Q_Z];
    data.quat[Q_Z] = t.quat[Q_Y];
    data.quat[Q_W] = t.quat[Q_W];
    // set the correct hand's position
    if (t.sensor == 0) {
        view->setLeftPos(&data);
    } else {
        view->setRightPos(&data);
    }
}

void VRPN_CALLBACK SimpleView::handle_button(void *userdata, const vrpn_BUTTONCB b) {
    SimpleView *view = (SimpleView *) userdata;
    view->setButtonState(b.button,b.state);
}

void VRPN_CALLBACK SimpleView::handle_analogs(void *userdata, const vrpn_ANALOGCB a) {
    SimpleView *view = (SimpleView *) userdata;
    if (a.num_channel != NUM_HYDRA_ANALOGS) {
        qDebug() << "We have problems!";
    }
    view->setAnalogStates(a.channel);
}
