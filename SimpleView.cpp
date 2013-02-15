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
#include <vtkVRMLExporter.h>
#include <limits>

#include <vtkXMLUtilities.h>
#include <projecttoxml.h>

// default number extra fibers
#define NUM_EXTRA_FIBERS 5
// fibrin default spring constant
#define BOND_SPRING_CONSTANT .5
// timestep
#define TIMESTEP (16/1000.0)

#define PROJECT_XML_FILENAME "project.xml"


// Constructor
SimpleView::SimpleView(QString projDir, bool load_fibrin, bool fibrin_springs, bool do_replicate) :
    tracker("Tracker0@localhost"),
    buttons("Tracker0@localhost"),
    analogRemote("Tracker0@localhost"),
    timer(new QTimer()),
    collisionModeGroup(new QActionGroup(this)),
    renderer(vtkSmartPointer<vtkRenderer>::New()),
    project(new SketchProject(renderer.GetPointer(),buttonDown,analog))
{
    this->ui = new Ui_SimpleView;
    this->ui->setupUi(this);
    collisionModeGroup->addAction(this->ui->actionPose_Mode_1);
    collisionModeGroup->addAction(this->ui->actionOld_Style);
    this->ui->actionPose_Mode_1->setChecked(true);
    if (!VRPN_ON) {
        q_xyz_quat_type pos;
        q_type ident = Q_ID_QUAT;
        q_vec_set(pos.xyz,.5,0,.5);
        q_vec_scale(pos.xyz,.5,pos.xyz);
        q_copy(pos.quat,ident);
//        transforms.setLeftHandTransform(&pos);
        q_vec_set(pos.xyz,0,0,1);
        q_vec_scale(pos.xyz,.5,pos.xyz);
//        transforms.setRightHandTransform(&pos);
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
    project->setProjectDir(projDir);
    QDir pd(project->getProjectDir());
    QString file = pd.absoluteFilePath(PROJECT_XML_FILENAME);
    QFile f(file);
    if (f.exists()) {
        vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());
        xmlToProject(project,root);
        root->Delete();
    } else if (load_fibrin) {
        // eventually we will just load the example...

        // ???
        SketchObject *object1 = project->addObject("./models/1m1j.obj");
        SketchObject *object2 = project->addObject("./models/1m1j.obj");

        if (fibrin_springs) {
            // creating springs
            q_vec_type p1 = {200,-30,0}, p2 = {0,-30,0};
            project->addSpring(object1,object2,0,0,BOND_SPRING_CONSTANT,p1,p2);

            q_vec_set(p1,0,-30,0);
            q_vec_set(p2,200,-30,0);
            project->addSpring(object1,object2,0,0,BOND_SPRING_CONSTANT,p1,p2);
        }

        // Replicate objects
        if (do_replicate) {
            project->addReplication(object1,object2,NUM_EXTRA_FIBERS);
        }

    }

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
    delete project;
    delete collisionModeGroup;
    delete timer;
}

void SimpleView::slotExit() 
{
    qApp->exit();
}

void SimpleView::handleInput() {
    tracker.mainloop();
    buttons.mainloop();
    analogRemote.mainloop();
}

/*
 * The method called once per frame to update things and redraw
 */
void SimpleView::slot_frameLoop() {
    // input
    handleInput();

    project->timestep(TIMESTEP);

    // render
    this->ui->qvtkWidget->GetRenderWindow()->Render();
}

void SimpleView::oldCollisionMode() {
    project->setCollisionMode(CollisionMode::ORIGINAL_COLLISION_RESPONSE);
}

void SimpleView::poseModeTry1() {
    project->setCollisionMode(CollisionMode::POSE_MODE_TRY_ONE);
}

void SimpleView::binaryCollisionSearch() {
    project->setCollisionMode(CollisionMode::BINARY_COLLISION_SEARCH);
}

void SimpleView::poseModePCA() {
    project->setCollisionMode(CollisionMode::POSE_WITH_PCA_COLLISION_RESPONSE);
}

void SimpleView::setLeftPos(q_xyz_quat_type *newPos) {
    project->setLeftHandPos(newPos);
}

void SimpleView::setRightPos(q_xyz_quat_type *newPos) {
    project->setRightHandPos(newPos);
}

void SimpleView::setButtonState(int buttonNum, bool buttonPressed) {
    buttonDown[buttonNum] = buttonPressed;
}

void SimpleView::setAnalogStates(const double state[]) {
    for (int i = 0; i < NUM_HYDRA_ANALOGS; i++) {
        analog[i] = state[i];
    }
}

SketchObject *SimpleView::addObject(QString name)
{
    return project->addObject(name);
}

bool SimpleView::addObjects(QVector<QString> names)
{
    return project->addObjects(names);
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

void SimpleView::saveProjectAs() {
    QString path = QFileDialog::getExistingDirectory(this,tr("Save Project As..."),"./");
    if (path.length() == 0) {
        return;
    } else {
        project->setProjectDir(path);
        saveProject();
    }
}

void SimpleView::saveProject() {
    QString path = project->getProjectDir();
    if (path.length() == 0) {
        // maybe eventually allow them to select if nothing exists... but for now,
        // enforce dir as part of project.
//        saveProjectAs();
        return;
    }
    QDir dir(path);
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);
    vtkXMLDataElement *root = projectToXML(project);
    vtkIndent indent(0);
    vtkXMLUtilities::WriteElementToFile(root,file.toStdString().c_str(),&indent);
    root->Delete();
}

void SimpleView::loadProject() {
    QString dirPath = QFileDialog::getExistingDirectory(this,tr("Select Project Directory (New or Existing)"), "./");
    if (dirPath.length() == 0) {
        return;
    }
    // clean up old project
    this->ui->qvtkWidget->GetRenderWindow()->RemoveRenderer(renderer);
    renderer = vtkSmartPointer<vtkRenderer>::New();

    vtkSmartPointer<vtkCamera> camera =
            vtkSmartPointer<vtkCamera>::New();
    camera->SetPosition(0, 0, 50);
    camera->SetFocalPoint(0, 0, 30);
    renderer->SetActiveCamera(camera);
    delete project;
    // create new one
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);
    project = new SketchProject(renderer,buttonDown,analog);
    project->setProjectDir(dirPath);
    this->ui->actionPose_Mode_1->setChecked(true);
    // load project into new one
    QDir dir(project->getProjectDir());
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);
    QFile f(file);
    // only load if xml file exists
    if (f.exists()) {
        vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());
        xmlToProject(project,root);
        root->Delete();
    }
}

bool SimpleView::simplifyObjectByName(const QString name)
{
    fprintf(stderr, "XXX Use importPDBId as a model to call Blender and have it simplify\n");
    fprintf(stderr, "XXX   (Should do several levels of simplification, or parameterize)\n");
    return false;
}

void SimpleView::simplifyOBJFile()
{
    // Ask the user for the name of the file to open.
    QString fn = QFileDialog::getOpenFileName(this,
                                              tr("Simplify OBJ file"),
                                              "./",
                                              tr("OBJ Files (*.obj)"));

    // Open the file for reading.
    if (fn.length() > 0) {
        printf("Loading %s\n", fn.toStdString().c_str());
        simplifyObjectByName(fn);
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
