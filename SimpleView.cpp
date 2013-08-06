#include "ui_SimpleView.h"
#include "SimpleView.h"

#include <stdexcept>

#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>
// TODO - take out next 3 when moving text stuff
#include <vtkTextProperty.h>
#include <vtkTextMapper.h>
#include <vtkActor2D.h>

#include <QDebug>
#include <QInputDialog>
#include <QFileDialog>
#include <QCloseEvent>
#include <QProgressDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

#include <sketchioconstants.h>
#include <transformmanager.h>
#include <modelutilities.h>
#include <sketchmodel.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <modelinstance.h>
#include <sketchproject.h>

#include <projecttoxml.h>

#include <hydrainputmanager.h>
#include <vrpnserver.h>

#include <subprocessrunner.h>
#include <subprocessutils.h>

// default number extra fibers
#define NUM_EXTRA_FIBERS 14
// fibrin default spring constant
#define BOND_SPRING_CONSTANT .5
// timestep
#define TIMESTEP (16/1000.0)


// Constructor
SimpleView::SimpleView(QString projDir, bool load_example) :
    timer(new QTimer()),
    server(new vrpnServer()),
    serverThread(new QThread(this)),
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

    if (VRPN_USE_INTERNAL_SERVER)
    {
        serverThread->start();
        server->moveToThread(serverThread);
        QTimer::singleShot(0,server,SLOT(startServer()));
    }

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
    if (f.exists())
    {
        vtkSmartPointer< vtkXMLDataElement > root =
                vtkSmartPointer< vtkXMLDataElement >::Take(
                    vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str())
                    );
        ProjectToXML::xmlToProject(project,root);
        project->setViewTime(0.0);
    }
    else if (load_example)
    {
        // eventually we will just load the example from a project directory...
        // example of keyframes this time

        SketchModel *model = project->addModelFromFile("PDB:1m1j","./models/1m1j.obj",
                                                       DEFAULT_INVERSE_MASS,DEFAULT_INVERSE_MOMENT);
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

    vtkSmartPointer< vtkTextProperty > textPropBottomRight =
            vtkSmartPointer< vtkTextProperty >::New();
    textPropBottomRight->SetFontFamilyToCourier();
    textPropBottomRight->SetFontSize(16);
    textPropBottomRight->SetVerticalJustificationToBottom();
    textPropBottomRight->SetJustificationToRight();

    directionsTextMapper = vtkSmartPointer<vtkTextMapper>::New();
    directionsTextMapper->SetInput(" ");
    directionsTextMapper->SetTextProperty(textPropTop);

    statusTextMapper = vtkSmartPointer<vtkTextMapper>::New();
    QString modeString("Collisions: ON\nSprings: ON\nMode: %1");
    statusTextMapper->SetInput(modeString.arg(inputManager->getModeName()).toStdString().c_str());
    statusTextMapper->SetTextProperty(textPropBottom);

    timeTextMapper = vtkSmartPointer<vtkTextMapper>::New();
    timeTextMapper->SetInput(QString("0.0").toStdString().c_str());
    timeTextMapper->SetTextProperty(textPropBottomRight);

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

    timeTextActor = vtkSmartPointer< vtkActor2D >::New();
    timeTextActor->SetMapper(timeTextMapper);
    timeTextActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    timeTextActor->GetPositionCoordinate()->SetValue(0.95,0.05);
    renderer->AddActor2D(timeTextActor);

    // Set up action signals and slots
    connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
    connect(this->inputManager, SIGNAL(toggleWorldSpringsEnabled()),
            this, SLOT(toggleWorldSpringsEnabled()));
    connect(this->inputManager, SIGNAL(toggleWorldCollisionsEnabled()),
            this, SLOT(toggleWorldCollisionTestsOn()));
    connect(this->inputManager, SIGNAL(newDirectionsString(QString)),
            this, SLOT(setTextMapperString(QString)));
    connect(this->inputManager, SIGNAL(changedModes(QString)),
            this, SLOT(updateStatusText()));
    connect(this->inputManager, SIGNAL(viewTimeChanged(double)),
            this, SLOT(updateViewTime(double)));

    // start timer for frame update
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_frameLoop()));
    timer->start(16);
}

SimpleView::~SimpleView() {
    timer->stop();
    delete inputManager;
    delete project;
    delete collisionModeGroup;
    if (server != NULL)
        delete server;
    delete timer;
}

void SimpleView::closeEvent(QCloseEvent *event)
{
    timer->stop();
    if (VRPN_USE_INTERNAL_SERVER)
    {
        QObject::connect(server,SIGNAL(destroyed()),serverThread,SLOT(quit()));
        QTimer::singleShot(0,server,SLOT(deleteLater()));
        server = NULL;
        while (!serverThread->isFinished())
        {
            QApplication::instance()->processEvents(QEventLoop::AllEvents,20);
        }
    }
    event->accept();
}

void SimpleView::slotExit() 
{
    if (VRPN_USE_INTERNAL_SERVER)
    {
        QObject::connect(server, SIGNAL(destroyed()),serverThread,SLOT(quit()));
        QTimer::singleShot(0,server,SLOT(deleteLater()));
        server = NULL;
        QObject::connect(serverThread, SIGNAL(finished()),qApp,SLOT(quit()));
    }
    else
    {
        qApp->exit();
    }
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
    project->setCollisionMode(PhysicsMode::ORIGINAL_COLLISION_RESPONSE);
}

void SimpleView::poseModeTry1() {
    project->setCollisionMode(PhysicsMode::POSE_MODE_TRY_ONE);
}

void SimpleView::binaryCollisionSearch() {
    project->setCollisionMode(PhysicsMode::BINARY_COLLISION_SEARCH);
}

void SimpleView::poseModePCA() {
    project->setCollisionMode(PhysicsMode::POSE_WITH_PCA_COLLISION_RESPONSE);
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
    return project->addObject(name,name);
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

void SimpleView::updateViewTime(double time)
{
    QString t("%1");
    timeTextMapper->SetInput(t.arg(time,8,'f',1).toStdString().c_str());
}

void SimpleView::goToViewTime()
{
    bool ok = true;
    double time = QInputDialog::getDouble(this,tr("Set Time to View"),tr("Time: "),
                                          0,0,std::numeric_limits<double>::max(),1,
                                          &ok);
    if (ok)
    {
        project->setViewTime(time);
        updateViewTime(time);
    }
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

void SimpleView::restartVRPNServer()
{
    if (VRPN_USE_INTERNAL_SERVER)
    {
        QMessageBox::information(this,"Restart VRPN Server",
                                 "Place the trackers on the correct sides of the base\n"
                                 "Then click 'OK'.\nThere will be a delay while the"
                                 " device is reset.");
        qDebug() << "Restarting VRPN server.";
        // signal the server to restart
        QTimer::singleShot(0,server,SLOT(restartServer()));
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
    renderer->AddActor2D(timeTextActor);

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
        vtkSmartPointer< vtkXMLDataElement > root =
                vtkSmartPointer< vtkXMLDataElement >::Take(
                    vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str())
                    );
        ProjectToXML::xmlToProject(project,root);
        project->setViewTime(0.0);
        updateViewTime(0.0);
    }
}

void SimpleView::simplifyObjectByName(const QString name)
{
    if (name.length() == 0) {
        return;
    }
    printf("Simplifying %s \n", name.toStdString().c_str());

    SubprocessRunner *runner = SubprocessUtils::simplifyObjFileByPercent(name,10);
    if (runner == NULL)
    {
        QMessageBox::warning(NULL,"Could not run Blender to simplify model.", name);
    }
    else
    {
        runSubprocessAndFreezeGUI(runner);
    }
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
    bool ok2;
    QString toDelete = QInputDialog::getText(this,tr("Specify chains to delete (if any)"),
                                             tr("Unneeded Chain IDS:"), QLineEdit::Normal,
                                             "", &ok2);
    if (ok && ok2 && !text.isEmpty()) {

        QString source = ModelUtilities::createSourceNameFor(text,toDelete);
        if (project->getModelManager()->hasModel(source))
        {
            SketchModel *model = project->getModelManager()->getModel(source);
            q_vec_type pos = Q_NULL_VECTOR;
            q_type orient = Q_ID_QUAT;
            project->addObject(model,pos,orient);
        }
        else
        {

            printf("Importing %s from PDB\n", text.toStdString().c_str());
            SubprocessRunner *objMaker = SubprocessUtils::loadFromPDB(project,text,toDelete);
            if (objMaker == NULL)
            {
                QMessageBox::warning(NULL, "Could not run subprocess to import molecule ", text);
            }
            else
            {
                runSubprocessAndFreezeGUI(objMaker);
            }
        }
    }
}

void SimpleView::exportBlenderAnimation() {
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("Select Save Location"),
                                              "./",
                                              tr("AVI Files (*.avi)"));
    if (fn.isEmpty())
        return;
    QFile f(fn);
    if (f.exists()) {
        if (!f.remove()) {
            return;
        }
    }
    SubprocessRunner *r = SubprocessUtils::createAnimationFor(project,fn);
    if (r == NULL) {
        QMessageBox::warning(NULL, "Error while setting up animation", "See log for details");
    } else {
        runSubprocessAndFreezeGUI(r);
    }
}

void SimpleView::runSubprocessAndFreezeGUI(SubprocessRunner *runner)
{
    if (runner == NULL)
        return;
    timer->stop();
    QProgressDialog *dialog = new QProgressDialog(".","Cancel",0,0,NULL);
    connect(runner, SIGNAL(statusChanged(QString)), dialog, SLOT(setLabelText(QString)));
    connect(runner, SIGNAL(finished(bool)), dialog, SLOT(reset()));
    connect(runner, SIGNAL(finished(bool)), timer, SLOT(start()));
    connect(runner, SIGNAL(destroyed()), dialog, SLOT(deleteLater()));
    connect(dialog, SIGNAL(canceled()), runner, SLOT(cancel()));
    connect(dialog, SIGNAL(canceled()), timer, SLOT(start()));
    connect(dialog, SIGNAL(canceled()), dialog, SLOT(reset()));
    dialog->open();
    runner->start();
}
