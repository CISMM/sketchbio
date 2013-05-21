#include "ui_SimpleView.h"
#include "SimpleView.h"
#include <QDebug>
#include <QInputDialog>
#include <QFileDialog>
#include <QProcess>
#include <QProgressDialog>
#include <QMessageBox>
#include <QThread>
#include <QTemporaryFile>
#include <QSettings>

#include <stdexcept>

#include <vtkPolyDataMapper.h>
#include <vtkOBJReader.h>
#include <vtkSphereSource.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkVRMLExporter.h>
// TODO - take out next 3 when moving text stuff
#include <vtkTextProperty.h>
#include <vtkTextMapper.h>
#include <vtkActor2D.h>
#include <limits>

#include <sketchioconstants.h>
#include <vtkXMLUtilities.h>
#include <projecttoxml.h>
#include <transformequals.h>

#include "subprocesses/blenderanimationrunner.h"

// default number extra fibers
#define NUM_EXTRA_FIBERS 14
// fibrin default spring constant
#define BOND_SPRING_CONSTANT .5
// timestep
#define TIMESTEP (16/1000.0)


// Constructor
SimpleView::SimpleView(QString projDir, bool load_example) :
    timer(new QTimer()),
    collisionModeGroup(new QActionGroup(this)),
    renderer(vtkSmartPointer<vtkRenderer>::New()),
    project(new SketchProject(renderer.GetPointer())),
    inputManager(new HydraInputManager(project))
{
    this->ui = new Ui_SimpleView;
    this->ui->setupUi(this);
    collisionModeGroup->addAction(this->ui->actionPose_Mode_1);
    collisionModeGroup->addAction(this->ui->actionOld_Style);
    collisionModeGroup->addAction(this->ui->actionBinary_Collision_Search);
    collisionModeGroup->addAction(this->ui->actionPose_Mode_PCA);
    this->ui->actionPose_Mode_1->setChecked(true);

    renderer->InteractiveOff();
    renderer->SetViewport(0,0,1,1);
    vtkSmartPointer<vtkRenderer> dummyRenderer = vtkSmartPointer<vtkRenderer>::New();
    dummyRenderer->InteractiveOn();
    dummyRenderer->SetViewport(0,0,1,1);
    dummyRenderer->SetBackground(0,0,0);

    project->setProjectDir(projDir);
    QDir pd(project->getProjectDir());
    QString file = pd.absoluteFilePath(PROJECT_XML_FILENAME);
    QFile f(file);
    if (f.exists()) {
        vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());
        ProjectToXML::xmlToProject(project,root);
        root->Delete();
    } else if (load_example) {
        // eventually we will just load the example from a project directory...
        // example of keyframes this time

        SketchModel *model = project->addModelFromFile("./models/1m1j.obj",INVERSEMASS,INVERSEMOMENT,1);
        q_vec_type position = {200,0,0};
        q_type orientation;
        q_from_axis_angle(orientation,0,1,0,0);
        SketchObject *object1 = new ModelInstance(model);
        object1->setPosAndOrient(position,orientation);
        object1->addKeyframeForCurrentLocation(0.0);

        q_vec_set(position,-200,0,0);
        q_from_axis_angle(orientation,0,1,0,Q_PI);
        object1->setPosAndOrient(position,orientation);
        object1->addKeyframeForCurrentLocation(10.0);

        q_vec_set(position,200,0,0);
        q_from_axis_angle(orientation,0,1,0,0);
        object1->setPosAndOrient(position,orientation);
        object1->addKeyframeForCurrentLocation(20.0);
        project->addObject(object1);

        q_vec_set(position,0,100,0);
        q_from_axis_angle(orientation,1,0,0,0);
        project->addCamera(position,orientation);
    }

    // VTK/Qt wedded
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(dummyRenderer);
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

    // test of text stuff
    vtkSmartPointer<vtkTextProperty> textPropTop= vtkSmartPointer<vtkTextProperty>::New();
    textPropTop->SetFontFamilyToCourier();
    textPropTop->SetFontSize(16);
    textPropTop->SetVerticalJustificationToTop();
    textPropTop->SetJustificationToLeft();

    vtkSmartPointer<vtkTextProperty> textPropBottom = vtkSmartPointer<vtkTextProperty>::New();
    textPropBottom->SetFontFamilyToCourier();
    textPropBottom->SetFontSize(16);
    textPropBottom->SetVerticalJustificationToBottom();
    textPropBottom->SetJustificationToLeft();

    directionsTextMapper = vtkSmartPointer<vtkTextMapper>::New();
    directionsTextMapper->SetInput(" ");
    directionsTextMapper->SetTextProperty(textPropTop);

    statusTextMapper = vtkSmartPointer<vtkTextMapper>::New();
    QString modeString("Collisions: ON\nSprings: ON\nMode: %1");
    statusTextMapper->SetInput(modeString.arg(inputManager->getModeName()).toStdString().c_str());
    statusTextMapper->SetTextProperty(textPropBottom);

    directionsTextActor = vtkSmartPointer<vtkActor2D>::New();
    directionsTextActor->SetMapper(directionsTextMapper);
    directionsTextActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    directionsTextActor->GetPositionCoordinate()->SetValue(0.05,0.95);
    renderer->AddActor2D(directionsTextActor);

    statusTextActor = vtkSmartPointer<vtkActor2D>::New();
    statusTextActor->SetMapper(statusTextMapper);
    statusTextActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    statusTextActor->GetPositionCoordinate()->SetValue(0.05,0.05);
    renderer->AddActor2D(statusTextActor);

    // Set up action signals and slots
    connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
    connect(this->inputManager, SIGNAL(toggleWorldSpringsEnabled()), this, SLOT(toggleWorldSpringsEnabled()));
    connect(this->inputManager, SIGNAL(toggleWorldCollisionsEnabled()), this, SLOT(toggleWorldCollisionTestsOn()));
    connect(this->inputManager, SIGNAL(newDirectionsString(QString)), this, SLOT(setTextMapperString(QString)));
    connect(this->inputManager, SIGNAL(changedModes(QString)), this, SLOT(updateStatusText()));

    // start timer for frame update
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_frameLoop()));
    timer->start(16);
}

SimpleView::~SimpleView() {
    delete inputManager;
    delete project;
    delete collisionModeGroup;
    delete timer;
}

void SimpleView::slotExit() 
{
    qApp->exit();
}


/*
 * The method called once per frame to update things and redraw
 */
void SimpleView::slot_frameLoop() {
    // input
    inputManager->handleCurrentInput();

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

void SimpleView::setWorldSpringsEnabled(bool enabled) {
    project->setWorldSpringsEnabled(enabled);
    updateStatusText();
}

void SimpleView::toggleWorldSpringsEnabled() {
    this->ui->actionWorld_Springs_On->trigger();
}

void SimpleView::setCollisionTestsOn(bool on) {
    project->setCollisionTestsOn(on);
    updateStatusText();
}

void SimpleView::toggleWorldCollisionTestsOn() {
    this->ui->actionCollision_Tests_On->trigger();
}

SketchObject *SimpleView::addObject(QString name)
{
    return project->addObject(name);
}

bool SimpleView::addObjects(QVector<QString> names)
{
    return project->addObjects(names);
}

void SimpleView::setTextMapperString(QString str) {
    directionsTextMapper->SetInput(str.toStdString().c_str());
}

void SimpleView::updateStatusText()
{
    bool cOn = this->ui->actionCollision_Tests_On->isChecked();
    bool springsEnabled = this->ui->actionWorld_Springs_On->isChecked();
    QString status("Collisions: %1\nSprings: %2\nMode: %3");
    status = status.arg(cOn ? "ON" : "OFF", springsEnabled ? "ON" : "OFF",
                        inputManager->getModeName());
    statusTextMapper->SetInput(status.toStdString().c_str());
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
    project->getTransformManager()->setRoomEyeOrientation(0,0);
    if (path.length() == 0) {
        // maybe eventually allow them to select if nothing exists... but for now,
        // enforce dir as part of project.
//        saveProjectAs();
        return;
    }
    QDir dir(path);
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);
    vtkXMLDataElement *root = ProjectToXML::projectToXML(project);
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
    renderer->InteractiveOff();
    renderer->SetViewport(0,0,1,1);
    renderer->AddActor2D(directionsTextActor);
    renderer->AddActor2D(statusTextActor);

    delete project;
    // create new one
    this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);
    project = new SketchProject(renderer);
    inputManager->setProject(project);
    project->setProjectDir(dirPath);
    project->setCollisionTestsOn(this->ui->actionCollision_Tests_On->isChecked());
    project->setWorldSpringsEnabled(this->ui->actionWorld_Springs_On->isChecked());
    this->ui->actionPose_Mode_1->setChecked(true);
    // load project into new one
    QDir dir(project->getProjectDir());
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);
    QFile f(file);
    // only load if xml file exists
    if (f.exists()) {
        vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());
        ProjectToXML::xmlToProject(project,root);
        root->Delete();
    }
}

bool SimpleView::simplifyObjectByName(const QString name)
{
    if (name.length() == 0) {
        return false;
    }
    printf("Simplifying %s ", name.toStdString().c_str());

    // Create a temporary file to hold the Python script that we're going to pass
    // to Blender.  It will be automatically deleted by the destructor.
    QTemporaryFile  py_file("XXXXXX.py");
    if (!py_file.open()) {
        fprintf(stderr,"Could not open temporary file %s\n", py_file.fileName().toStdString().c_str());
        return false;
    }
    printf("using %s as a temporary file\n", py_file.fileName().toStdString().c_str());

    //----------------------------------------------------------------
    // Write the Python script we'll pass to Blender into the file.
    char    *buf = new char[name.size() + 4096];
    if (buf == NULL) {
        fprintf(stderr,"Out of memory simplifying object\n");
        return false;
    }
    py_file.write("import bpy\n");
    // Read the data file
    sprintf(buf, "bpy.ops.import_scene.obj(filepath='%s')\n", name.toStdString().c_str());
    py_file.write(buf);
    // Delete the cube object
    py_file.write("bpy.ops.object.select_all(action='DESELECT')\n");
    py_file.write("myobject = bpy.data.objects['Cube']\n");
    py_file.write("myobject.select = True\n");
    py_file.write("bpy.context.scene.objects.active = myobject\n");
    py_file.write("bpy.ops.object.delete()\n");
    // Select the data object
    py_file.write("bpy.ops.object.select_all(action='DESELECT')\n");
//    py_file.write("myobject = bpy.data.objects['Mesh']\n");
    py_file.write("keys = bpy.data.objects.keys()\n");
    py_file.write("for key in keys:\n");
    py_file.write("\tif key != 'Camera' and key != 'Lamp':\n");
    py_file.write("\t\tmyobject = bpy.data.objects[key]\n");
    py_file.write("\t\tbreak\n");
    py_file.write("myobject.select = True\n");
    py_file.write("bpy.context.scene.objects.active = myobject\n");
    // Turn on edit mode and adjust the mesh to remove all non-manifold
    // vertices.
    py_file.write("bpy.ops.object.mode_set(mode='EDIT')\n");
    py_file.write("bpy.ops.mesh.remove_doubles()\n");
    py_file.write("bpy.ops.mesh.select_all(action='DESELECT')\n");
    py_file.write("bpy.ops.mesh.select_non_manifold()\n");
    py_file.write("bpy.ops.mesh.delete()\n");
    // Switch back to object mode.
    py_file.write("bpy.ops.object.mode_set(mode='OBJECT')\n");
    // Run the decimate modifier.
    py_file.write("bpy.ops.object.modifier_add(type='DECIMATE')\n");
    py_file.write("bpy.context.active_object.modifiers[0].ratio = 0.1\n");
    py_file.write("bpy.ops.object.modifier_apply()\n");
    // Save the resulting simplified object.
    sprintf(buf, "bpy.ops.export_scene.obj(filepath='%s.decimated.0.1.obj')\n", name.toStdString().c_str());
    py_file.write(buf);
    py_file.close();

    //----------------------------------------------------------------
    // Start the Blender process and then write the commands to it
    // that will cause it to load the OBJ file, simplify it,
    // and then save the file.
    QProcess blender;
    // Combine stderr with stdout and send back back together.
    blender.setProcessChannelMode(QProcess::MergedChannels);
    // -P: Run the specified Python script
    // -b: Run in background
    blender.start(getSubprocessExecutablePath("blender"),
                  QStringList() << "-noaudio" << "-b" << "-P" << py_file.fileName());
    if (!blender.waitForStarted()) {
      QMessageBox::warning(NULL, "Could not run Blender to simplify file ", name);
    }

    //----------------------------------------------------------------
    // Time out after 2 minutes if the process does not exit
    // on its own by then.
    if (!blender.waitForFinished(120000)) {
        QMessageBox::warning(NULL, "Could not complete blender simplify", name);
    }

    //----------------------------------------------------------------
    // Read all of the output from the program and then
    // see if we got what we expected.  We look for reports
    // that it saved the file and that it exited normally.
    // XXX This does not guarantee that it saved the file.
    // can we check return codes in blender and print an error?
    QByteArray result = blender.readAll();
    if ( !result.contains("OBJ Export time:") ) {
    QMessageBox::warning(NULL, "Error while simplifying", name);
        printf("Blender problem:\n%s\n", result.data());
    }
    delete [] buf;
    return true;
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
          pymol.start(getSubprocessExecutablePath("pymol"), QStringList() << "-c" << "-p");
	  if (!pymol.waitForStarted()) {
        QMessageBox::warning(NULL, "Could not run pymol to import molecule ", text);
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
	    // problem.  It looks like the ls() command causes things to
	    // wait until the file is there.  Not sure why this works
	    // when sync() does not, but what the heck.
	    // XXX Wait here until the file shows up, rather than doing
	    // a print and then waiting until the print shows up and then
	    // some random time longer.
	    // XXX I submitted a bug report to the Pymol user list to try
	    // and get this race condition fixed.  Remove all of the hanky-
	    // panky about reading and printing and waiting when it is
	    // fixed.
	    cmd = "ls\n";
	    printf("... %s", cmd.toStdString().c_str());
	    pymol.write(cmd.toStdString().c_str());

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

void SimpleView::exportBlenderAnimation() {
    timer->stop();
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("Select Save Location"),
                                              "./",
                                              tr("AVI Files (*.avi)"));
    QFile f(fn);
    if (f.exists()) {
        if (!f.remove()) {
            return;
        }
    }
    BlenderAnimationRunner *r = BlenderAnimationRunner::createAnimationFor(project,fn);
    if (r == NULL) {
        QMessageBox::warning(NULL, "Error while setting up animation", "See log for details");
        timer->start();
    } else {
        MyDialog *dialog = new MyDialog("Rendering animation...","Cancel",0,0,this);
        connect(r, SIGNAL(statusChanged(QString)), dialog, SLOT(setLabelText(QString)));
        connect(r, SIGNAL(finished(bool)), dialog, SLOT(resetAndSignal()));
        connect(r, SIGNAL(finished(bool)), timer, SLOT(start()));
        connect(dialog, SIGNAL(canceled()), r, SLOT(canceled()));
        connect(dialog, SIGNAL(canceled()), timer, SLOT(start()));
        connect(dialog, SIGNAL(canceled()), dialog, SLOT(resetAndSignal()));
        connect(dialog, SIGNAL(deleteMe()), dialog, SLOT(deleteLater()));
        dialog->open();
    }
}

QString SimpleView::getSubprocessExecutablePath(QString executableName) {
    QSettings settings; // default parameters specified to the QCoreApplication at startup
    QString executablePath = settings.value("subprocesses/" + executableName + "/path",QString("")).toString();
    if (executablePath.length() == 0 || ! QFile(executablePath).exists()) {
#if defined(__APPLE__) && defined(__MACH__)
        // test /Applications/appName and /usr/bin then ask
        if (QFile("/Applications/" + executableName + ".app/Contents/MacOS/" + executableName).exists()) {
            executablePath = "/Applications/" + executableName + ".app/Contents/MacOS/" + executableName;
        } else if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else if (QFile("/usr/local/bin/" + executableName).exists()) {
            executablePath = "/usr/local/bin/" + executableName;
        } else {
            executablePath = QFileDialog::getOpenFileName(this,"Specify location of '" + executableName + "'","/Applications");
            if (executablePath.endsWith(".app")) {
                executablePath = executablePath + "/Contents/MacOS/" + executableName;
            }
        }
#elif defined(_WIN32)
        // test default locations C:/Program Files and C:/Program Files(x86) then ask for a .exe file
        if (QFile("C:/Program Files/" + executableName + "/" + executableName + ".exe").exists()) {
            executablePath = "C:/Program Files/" + executableName + "/" + executableName;
        }
#ifdef _WIN64
        else if (QFile("C:/Program Files(x86)/" + executableName + "/" + executableName + ".exe").exists()) {
            executablePath = "C:/Program Files(x86)/" + executableName + "/" + executableName;
        }
#endif
        else {
            executablePath = QFileDialog::getOpenFileName(this, "Specify location of '" + executableName + "'","C:/Program Files","", &QString(".exe"));
        }
#elif defined(__linux__)
        // test /usr/bin then ask for the file
        if (QFile("/usr/bin/" + executableName).exists()) {
            executablePath = "/usr/bin/" + executableName;
        } else {
            executablePath = QFileDialog::getOpenFileName(this,"Specify location of '" + executableName + "'");
        }
#endif
        settings.setValue("subprocesses/" + executableName + "/path",executablePath);
    }
    return executablePath;
}
