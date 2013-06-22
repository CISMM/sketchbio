#include "blenderdecimationrunner.h"

#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>

#include "subprocessutils.h"

inline void writeBlenderHelpers(QFile &file)
{
    // borrowed from Russ's code, he got them from:
    // http://wiki.blender.org/index.php/Dev:2.5/Py/Scripts/Cookbook/Code_snippets/Materials_and_textures
    // helper functions for selecting objects
    file.write("# selects the given object and deselects all others\n");
    file.write("def select_object(ob):\n");
    file.write("\tbpy.ops.object.select_all(action='DESELECT')\n");
    file.write("\tob.select = True\n");
    file.write("\tbpy.context.scene.objects.active = ob\n");
    file.write("\n");
    file.write("# selects the object with the given name\n");
    file.write("def select_named(name):\n");
    file.write("\tmyobject = bpy.data.objects[name]\n");
    file.write("\tselect_object(myobject)\n");
    file.write("\n");
    // helper function for reading obj models
    // this one I wrote myself
    file.write("# reads in an obj file and joins all the objects that were\n"
               "# read out of it into a single object\n");
    file.write("def read_obj_model(filename):\n");
    file.write("\toldKeys = list(bpy.data.objects.keys()) # copies the object names list\n");
    file.write("\tbpy.ops.import_scene.obj(filepath=filename)\n");
    file.write("\tsomethingSelected = False\n");
    file.write("\tobject = None\n");
    file.write("\tfor key in bpy.data.objects.keys():\n");
    file.write("\t\tif not (key in oldKeys):\n");
    file.write("\t\t\tif somethingSelected:\n");
    file.write("\t\t\t\tbpy.data.objects[key].select = True\n");
    file.write("\t\t\telse:\n");
    file.write("\t\t\t\tselect_named(key)\n");
    file.write("\t\t\t\tobject = bpy.data.objects[key]\n");
    file.write("\t\t\t\tsomethingSelected = True\n");
    file.write("\tbpy.ops.object.join() # merge all the new objects into the first one we found\n");
    file.write("\tnewName = filename[filename.rfind('/')+1:-4]\n");
    file.write("\tobject.name = newName # set the name to something based on the filename\n");
    file.write("\tselect_named(newName)\n\n");

    // helper function to get the number of triangles
    file.write("# gets the number of triangles in the object\n");
    file.write("# technique may vary by blender version\n");
    file.write("def get_triangle_count(obj):\n");
    file.write("\ttriangle_count = 0\n");
    file.write("\tif ('faces' in dir(obj.data)):\n");
    file.write("\t\tfor f in myobject.data.faces:\n");
    file.write("\t\t\tcount = len(f.vertices)\n");
    file.write("\t\t\tif count > 2:\n");
    file.write("\t\t\t\ttriangle_count += count - 2\n");
    file.write("\telif ('polygons' in dir(obj.data)):\n");
    file.write("\t\tfor p in myobject.data.polygons:\n");
    file.write("\t\t\tcount = p.loop_total\n");
    file.write("\t\t\tif count > 2:\n");
    file.write("\t\t\t\ttriangle_count += count - 2\n");
    file.write("\treturn triangle_count\n");
}

BlenderDecimationRunner::BlenderDecimationRunner(const QString &objFile, DecimationType::Type type,
                                                 double param, QObject *parent) :
    AbstractSingleProcessRunner(parent),
    tempFile(new QTemporaryFile("XXXXXX.py",this)),
    valid(true)
{
    if (tempFile->open())
    {
        tempFile->write("import bpy\n");
        writeBlenderHelpers(*tempFile);
        // Read the data file
        // Delete the cube object
        tempFile->write("bpy.ops.object.select_all(action='DESELECT')\n");
        tempFile->write("myobject = bpy.data.objects['Cube']\n");
        tempFile->write("myobject.select = True\n");
        tempFile->write("bpy.context.scene.objects.active = myobject\n");
        tempFile->write("bpy.ops.object.delete()\n");
        // Select the data object
        QString line = "read_obj_model('%1')\n";
        line = line.arg(objFile);
        tempFile->write(line.toStdString().c_str());
        tempFile->write("myobject = bpy.context.scene.objects.active\n");
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
        if (type == DecimationType::PERCENT)
        {
            line = "ratio = %1\n";
            line = line.arg(param);
            tempFile->write(line.toStdString().c_str());
        }
        else if (type == DecimationType::POLYGON_COUNT)
        {
            tempFile->write("triangle_count = get_triangle_count(myobject)\n");
            line = "ratio = min(%1 * 1.0 / triangle_count,1)\n";
            line = line.arg(param);
            tempFile->write(line.toStdString().c_str());
        }
        tempFile->write("bpy.context.active_object.modifiers[0].ratio = ratio\n");
        tempFile->write("bpy.ops.object.modifier_apply()\n");
        // go to edit mode and try to remove bad faces.
        tempFile->write("bpy.ops.object.mode_set(mode='EDIT')\n");
        tempFile->write("bpy.ops.mesh.select_all(action='DESELECT')\n");
        tempFile->write("bpy.ops.mesh.select_non_manifold()\n");
        tempFile->write("bpy.ops.mesh.delete()\n");
        // Switch back to object mode.
        tempFile->write("bpy.ops.object.mode_set(mode='OBJECT')\n");
        // Save the resulting simplified object.
        line = "bpy.ops.export_scene.obj(filepath='%1.decimated.%2.obj',use_edges=False)\n";
        line = line.arg(objFile).arg(param);
        tempFile->write(line.toStdString().c_str());
        tempFile->close();
        if (tempFile->error() != QFile::NoError)
            valid = false;
    }
    else
        valid = false;
}

void BlenderDecimationRunner::start()
{
    // -P: Run the specified Python script
    // -b: Run in background
    process->start(SubprocessUtils::getSubprocessExecutablePath("blender"),
                   QStringList() << "-noaudio" << "-b" << "-P" << tempFile->fileName());
    if (!process->waitForStarted()) {
        qDebug() << "Could not run Blender to simplify file";
        emit finished(false);
        deleteLater();
    }
    else
        emit statusChanged("Simplifying object...");
}

bool BlenderDecimationRunner::isValid()
{
    return valid;
}

bool BlenderDecimationRunner::didProcessSucceed()
{
    QByteArray result = process->readAll();

    if (!result.contains("OBJ Export time:") )
    {
        printf("Blender problem:\n%s\n", result.data());
        return false;
    }
    else
        return true;
}
