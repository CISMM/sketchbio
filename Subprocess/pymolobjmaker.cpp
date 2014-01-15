#include "pymolobjmaker.h"

#include <QProcess>
#include <QTimer>
#include <QTemporaryFile>
#include <QDir>
#include <QDebug>

#include "subprocessutils.h"

PymolOBJMaker::PymolOBJMaker(const QString &pdbID, const QString &dirName,
                             QObject *parent)
    : SubprocessRunner(parent),
      pymol(new QProcess(this)),
      blender(new QProcess(this)),
      pmlFile(new QTemporaryFile(QDir::tempPath() + "/XXXXXX.pml", this)),
      bpyFile(new QTemporaryFile(QDir::tempPath() + "/XXXXXX.py", this)),
      saveDir(dirName),
      objFile(NULL),
      valid(true)
{
  QString pdb = pdbID.toLower();
  QDir dir(dirName);
  QFile mtlFile(dir.absoluteFilePath(pdb + ".mtl"));
  if (mtlFile.exists())
    if (!mtlFile.remove()) valid = false;
  QFile finalFile(dir.absoluteFilePath(pdb + ".obj"));
  if (finalFile.exists())
    if (!finalFile.remove()) valid = false;
  objFile = new QFile(dir.absoluteFilePath(pdb + "-tmp.obj"), this);
  if (objFile->exists())
    if (!objFile->remove()) valid = false;
  if (pmlFile->open()) {
    QString cmd =
        "load "
        "http://www.pdb.org/pdb/download/"
        "downloadFile.do?fileFormat=pdb&compression=NO&structureId=%1\n";
    cmd = cmd.arg(pdb);
    pmlFile->write(cmd.toStdString().c_str());
    cmd = "save %1/%2.pdb\n";
    cmd = cmd.arg(dirName, pdb);
    pmlFile->write(cmd.toStdString().c_str());
    pmlFile->write("hide all\n");
    pmlFile->write("show surface\n");
    cmd = "save %1\n";
    cmd = cmd.arg(objFile->fileName());
    pmlFile->write(cmd.toStdString().c_str());
    // ls *should* force it to wait for the file to be written
    pmlFile->write("ls\n");
    pmlFile->write("print \"readytoquit\"\n");
    if (pmlFile->error() != QFile::NoError) valid = false;
    pmlFile->close();
  } else
    valid = false;
  if (bpyFile->open()) {
    bpyFile->write("import bpy\n");
    // Read the data file
    QString line = "bpy.ops.import_scene.obj(filepath='%1')\n";
    line = line.arg(objFile->fileName());
    bpyFile->write(line.toStdString().c_str());
    // Delete the cube object
    bpyFile->write("bpy.ops.object.select_all(action='DESELECT')\n");
    bpyFile->write("myobject = bpy.data.objects['Cube']\n");
    bpyFile->write("myobject.select = True\n");
    bpyFile->write("bpy.context.scene.objects.active = myobject\n");
    bpyFile->write("bpy.ops.object.delete()\n");
    // Select the data object
    bpyFile->write("bpy.ops.object.select_all(action='DESELECT')\n");
    bpyFile->write("keys = bpy.data.objects.keys()\n");
    bpyFile->write("for key in keys:\n");
    bpyFile->write("\tif key != 'Camera' and key != 'Lamp':\n");
    bpyFile->write("\t\tmyobject = bpy.data.objects[key]\n");
    bpyFile->write("\t\tbreak\n");
    bpyFile->write("myobject.select = True\n");
    bpyFile->write("bpy.context.scene.objects.active = myobject\n");
    // Turn on edit mode and adjust the mesh to remove all non-manifold
    // vertices.
    bpyFile->write("bpy.ops.object.mode_set(mode='EDIT')\n");
    bpyFile->write("bpy.ops.mesh.remove_doubles()\n");
    bpyFile->write("bpy.ops.mesh.select_all(action='DESELECT')\n");
    bpyFile->write("bpy.ops.mesh.select_non_manifold()\n");
    bpyFile->write("bpy.ops.mesh.delete()\n");
    // Switch back to object mode.
    bpyFile->write("bpy.ops.object.mode_set(mode='OBJECT')\n");
    // Save the resulting simplified object.
    line = "bpy.ops.export_scene.obj(filepath='%1')\n";
    line = line.arg(finalFile.fileName());
    bpyFile->write(line.toStdString().c_str());
    if (bpyFile->error() != QFile::NoError) valid = false;
    bpyFile->close();
  } else
    valid = false;
  pymol->setProcessChannelMode(QProcess::MergedChannels);
  connect(pymol, SIGNAL(finished(int)), this, SLOT(pymolFinished(int)));
  connect(pymol, SIGNAL(readyRead()), this, SLOT(newOutputReady()));
}

PymolOBJMaker::~PymolOBJMaker() {}

void PymolOBJMaker::start()
{
  qDebug() << "Starting pymol.";
  QStringList args;
  args << "-c" << pmlFile->fileName() << "-p";
  //    qDebug() << args;
  pymol->start(SubprocessUtils::getSubprocessExecutablePath("pymol"), args);
  // -c  -- console only (no gui)
  // filename - read commands from this file
  // -p  -- also allow reading commands from stdin after the file
  //          finishes (we can check for the file existing before
  //          quitting)
  if (!pymol->waitForStarted()) {
    qDebug() << "Pymol failed to start.";
    emit finished(false);
    deleteLater();
  } else
    emit statusChanged("Creating surface in PyMOL...");
}

void PymolOBJMaker::cancel()
{
  if (pymol->state() == QProcess::Running) {
    disconnect(pymol, SIGNAL(finished(int)), this, SLOT(pymolFinished(int)));
    disconnect(pymol, SIGNAL(readyRead()), this, SLOT(newOutputReady()));
    connect(pymol, SIGNAL(finished(int)), this, SLOT(deleteLater()));
    pymol->kill();
  } else if (blender != NULL && blender->state() == QProcess::Running) {
    disconnect(blender, SIGNAL(finished(int)), this,
               SLOT(blenderFinished(int)));
    connect(blender, SIGNAL(finished(int)), this, SLOT(deleteLater()));
    blender->kill();
    objFile->remove();
  } else
    deleteLater();
}

bool PymolOBJMaker::isValid() { return valid; }

void PymolOBJMaker::newOutputReady()
{
  while (pymol->canReadLine()) {
    QString line(pymol->readLine());
    if (line.trimmed().endsWith("readytoquit")) {
      checkForFileWritten();
    }
  }
}

void PymolOBJMaker::checkForFileWritten()
{
  if (!objFile->exists() || objFile->size() == 0)
    QTimer::singleShot(200, this, SLOT(checkForFileWritten()));
  else
    pymol->write("quit\n");
}

void PymolOBJMaker::pymolFinished(int status)
{
  if (status != QProcess::NormalExit || pymol->exitCode() != 0 ||
      !objFile->exists() || objFile->size() == 0) {
    qDebug() << "Pymol failed to create the file.";
    qDebug() << pymol->readAll();
    emit finished(false);
    deleteLater();
  } else {
    qDebug() << "Pymol finished writing obj file.";
    qDebug() << "Starting blender to clean up obj file from pymol.";
    QStringList args;
    args << "-noaudio"
         << "-b"
         << "-P" << bpyFile->fileName();
    // -noaudio  -- self explainatory
    // -b        -- no gui, console only
    // -P        -- read from given python file
    connect(blender, SIGNAL(finished(int)), this, SLOT(blenderFinished(int)));
    blender->start(SubprocessUtils::getSubprocessExecutablePath("blender"),
                   args);
    if (!blender->waitForStarted(1000)) {
      objFile->remove();
      emit finished(false);
      deleteLater();
    } else {
      emit statusChanged("Cleaning up PyMOL's exported surface.");
    }
  }
}

void PymolOBJMaker::blenderFinished(int status)
{
  bool success = true;
  if (status != QProcess::NormalExit || blender->exitCode() != 0) {
    success = false;
    qDebug() << "Cleaning up obj file failed.";
  } else {
    qDebug() << "Finished cleaning up obj file.";
  }
  //    qDebug() << blender->readAll();
  objFile->remove();  // try to clean up temp file
  emit finished(success);
  deleteLater();
}
