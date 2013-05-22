#include "blenderdecimationrunner.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>
#include "subprocessutils.h"

BlenderDecimationRunner::BlenderDecimationRunner(QString objFile, QObject *parent) :
    SubprocessRunner(parent),
    blender(new QProcess(this)),
    tempFile(new QTemporaryFile("XXXXXX.py",this)),
    valid(true)
{
    if (tempFile->open())
    {
        tempFile->write("import bpy\n");
        // Read the data file
        QString line = "bpy.ops.import_scene.obj(filepath='%1')\n";
        line.arg(objFile);
        tempFile->write(line.toStdString().c_str());
        // Delete the cube object
        tempFile->write("bpy.ops.object.select_all(action='DESELECT')\n");
        tempFile->write("myobject = bpy.data.objects['Cube']\n");
        tempFile->write("myobject.select = True\n");
        tempFile->write("bpy.context.scene.objects.active = myobject\n");
        tempFile->write("bpy.ops.object.delete()\n");
        // Select the data object
        tempFile->write("bpy.ops.object.select_all(action='DESELECT')\n");
        tempFile->write("keys = bpy.data.objects.keys()\n");
        tempFile->write("for key in keys:\n");
        tempFile->write("\tif key != 'Camera' and key != 'Lamp':\n");
        tempFile->write("\t\tmyobject = bpy.data.objects[key]\n");
        tempFile->write("\t\tbreak\n");
        tempFile->write("myobject.select = True\n");
        tempFile->write("bpy.context.scene.objects.active = myobject\n");
        // Turn on edit mode and adjust the mesh to remove all non-manifold
        // vertices.
        tempFile->write("bpy.ops.object.mode_set(mode='EDIT')\n");
        tempFile->write("bpy.ops.mesh.remove_doubles()\n");
        tempFile->write("bpy.ops.mesh.select_all(action='DESELECT')\n");
        tempFile->write("bpy.ops.mesh.select_non_manifold()\n");
        tempFile->write("bpy.ops.mesh.delete()\n");
        // Switch back to object mode.
        tempFile->write("bpy.ops.object.mode_set(mode='OBJECT')\n");
        // Run the decimate modifier.
        tempFile->write("bpy.ops.object.modifier_add(type='DECIMATE')\n");
        tempFile->write("bpy.context.active_object.modifiers[0].ratio = 0.1\n");
        tempFile->write("bpy.ops.object.modifier_apply()\n");
        // Save the resulting simplified object.
        line = "bpy.ops.export_scene.obj(filepath='%1.decimated.0.1.obj')\n";
        line.arg(objFile);
        tempFile->write(line.toStdString().c_str());
        if (tempFile->error() != QFile::NoError)
            valid = false;
        tempFile->close();

        blender->setProcessChannelMode(QProcess::MergedChannels);

        connect(blender, SIGNAL(finished(int)), this, SLOT(blenderFinished(int)));
    }
    else
        valid = false;
}

void BlenderDecimationRunner::start()
{
    // -P: Run the specified Python script
    // -b: Run in background
    blender->start(SubprocessUtils::getSubprocessExecutablePath("blender"),
                  QStringList() << "-noaudio" << "-b" << "-P" << tempFile->fileName());
    if (!blender->waitForStarted()) {
        qDebug() << "Could not run Blender to simplify file";
        emit finished(false);
        deleteLater();
    }
}

void BlenderDecimationRunner::cancel()
{
    if (blender->state() == QProcess::Running)
    {
        disconnect(blender, SIGNAL(finished(int)), this, SLOT(blenderFinished(int)));
        connect(blender, SIGNAL(finished(int)), this, SLOT(deleteLater()));
        blender->kill();
    } else {
        deleteLater();
    }
}

bool BlenderDecimationRunner::isValid()
{
    return valid;
}

void BlenderDecimationRunner::blenderFinished(int status)
{
    QByteArray result = blender->readAll();
    if ( status != QProcess::NormalExit ||
         blender->exitCode() != 0 ||
         !result.contains("OBJ Export time:") )
    {
        qDebug() << "Error while simplifying";
        printf("Blender problem:\n%s\n", result.data());
        emit finished(false);
    }
    else
    {
        emit finished(true);
    }
    deleteLater();
}
