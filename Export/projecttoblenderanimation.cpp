#include "projecttoblenderanimation.h"
#include <sketchmodel.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <sketchproject.h>
#include <QFile>
#include <QScopedPointer>

unsigned ProjectToBlenderAnimation::timeToBlenderFrameNum(double time, unsigned frameRate) {
    return floor(time * frameRate + 0.5);
}

bool ProjectToBlenderAnimation::writeProjectBlenderFile(QFile &file, SketchProject *proj) {
    // python imports... math and bpy
    file.write("import bpy\n");
    file.write("import math\n");
    bool success = writeHelperFunctions(file);
    file.write("select_named('Cube')\n");
    file.write("bpy.ops.object.delete()\n");
    file.write("select_named('Lamp')\n");
    file.write("bpy.ops.object.delete()\n");
    file.write("modelObjects = list()\n");
    file.write("myObjects = list()\n");
    QHash< QPair< SketchModel *, int >, int> modelIdxs;
    QHash<SketchObject *, int> objectIdxs;
    success &= writeCreateModels(file,modelIdxs,proj->getModelManager());
    success &= writeCreateObjects(file,modelIdxs,objectIdxs, proj);
    success &= writeObjectKeyframes(file,objectIdxs, proj);
    QScopedPointer<char,QScopedPointerArrayDeleter<char> > buf(new char[4096]);
    sprintf(buf.data(),"bpy.context.scene.render.fps = %u\n",BLENDER_RENDERER_FRAMERATE);
    file.write(buf.data());
    file.write("bpy.ops.object.lamp_add(type='SUN')\n");
    file.write("bpy.context.active_object.data.energy = 0.16\n");
    file.write("bpy.context.scene.render.fps_base = 1\n");
    file.write("bpy.context.scene.world.light_settings.use_ambient_occlusion = True\n");
    file.write("bpy.context.scene.world.light_settings.ao_factor = 1.0\n");
    file.write("bpy.context.scene.world.light_settings.ao_blend_type = 'MULTIPLY'\n");
    file.write("bpy.context.scene.world.light_settings.distance = 40\n");
    file.write("bpy.context.scene.world.light_settings.samples = 5\n");
    file.write("bpy.context.scene.world.horizon_color = (0.0, 0.0, 0.0)\n");
    file.write("bpy.ops.render.render(animation=True)\n");
    file.write("bpy.ops.wm.quit_blender()\n");
    return success && file.error() == QFile::NoError;
}

bool ProjectToBlenderAnimation::writeCreateModels(QFile &file, QHash< QPair< SketchModel *, int >, int> &modelIdxs,
                                                  const ModelManager *models) {
    file.write("\n\n");
    file.write("# Create template objects for each model\n");
    int modelObjectsLen = 0;
    QVectorIterator<SketchModel *> it = models->getModelIterator();
    while (it.hasNext()) {
        SketchModel *model = it.next();
        for (int i = 0; i < model->getNumberOfConformations(); i++)
        {
            modelIdxs.insert(QPair< SketchModel *, int >(model,i),modelObjectsLen++);
            if (model->getSource(i) == CAMERA_MODEL_KEY) {
                file.write("obj = getDefaultCamera()\n");
            } else {
                file.write("obj = read_obj_model('");
                file.write(model->getFileNameFor(
                               i,
                               ModelResolution::FULL_RESOLUTION).toStdString().c_str());
                file.write("')\n");
            }
            file.write("select_object(obj)\n");
            // throw them way out in space
            file.write("bpy.ops.object.shade_smooth()\n");
            file.write("bpy.context.active_object.location = (float(50000), float(50000), float(50000))\n");
            file.write("modelObjects.append(obj)\n");
        }
    }
    return file.error() == QFile::NoError;
}

bool ProjectToBlenderAnimation::writeCreateObjects(QFile &file, QHash< QPair< SketchModel *, int >, int> &modelIdxs,
                                                   QHash<SketchObject *, int> &objectIdxs, SketchProject *proj) {
    WorldManager *world = proj->getWorldManager();
    file.write("\n\n");
    file.write("# Duplicate objects so we have on for each object in SketchBio, plus a template for each type.\n");
    int objectsLen = 0;
//    unsigned maxFrame = 0;
    QScopedPointer<char,QScopedPointerArrayDeleter<char> > buf(new char[4096]);
    QListIterator<SketchObject *> it = world->getObjectIterator();
    while (it.hasNext()) {
        writeCreateObject(file,modelIdxs, objectIdxs, objectsLen, proj, it.next());
    }
    file.write("\n\n");
    return file.error() == QFile::NoError;
}

static inline void computeNewTranslate(q_vec_type pos, q_type orient, SketchObject *obj)
{
    q_vec_type tmp;
    obj->getPosition(pos);
    obj->getOrientation(orient);
    double bb[6];
    obj->getBoundingBox(bb);
    q_vec_type boundsCenter = {bb[1] + bb[0], bb[3] + bb[2], bb[5] + bb[4]};
    q_vec_scale(boundsCenter,0.5,boundsCenter);
    q_xform(tmp,orient,boundsCenter);
    q_vec_add(pos,tmp,pos);
}

void ProjectToBlenderAnimation::writeCreateObject(QFile &file, QHash< QPair< SketchModel *, int >, int> &modelIdxs,
                                                  QHash<SketchObject *, int> &objectIdxs, int &objectsLen,
                                                  SketchProject *proj, SketchObject *obj)
{
    if (obj->numInstances() != 1) {
        QListIterator<SketchObject *> it(*obj->getSubObjects());
        while (it.hasNext()) {
            writeCreateObject(file,modelIdxs, objectIdxs, objectsLen, proj, it.next());
        }
    } else {
        QScopedPointer<char, QScopedPointerArrayDeleter<char> > buf(new char[4096]);
        SketchModel *model  = obj->getModel();
        int idx = modelIdxs.value(
                    QPair< SketchModel *, int >(model,obj->getModelConformation()));
        sprintf(buf.data(),"select_object(modelObjects[%u])\n",idx);
        file.write(buf.data());
        file.write("bpy.ops.object.duplicate(linked=True)\n");
        file.write("myObjects.append(bpy.context.active_object)\n");
        objectIdxs.insert(obj,objectsLen++);
        q_vec_type pos;
        q_type orient, tmp;
        computeNewTranslate(pos,orient,obj);
        q_from_axis_angle(tmp,0,1,0,Q_PI);
        bool isCamera = false;
        if (model->getSource(obj->getModelConformation()) == CAMERA_MODEL_KEY) {
            q_mult(orient,orient,tmp);
            isCamera = true;
        }
        file.write("bpy.context.active_object.location = (0,0,0)\n");
        file.write("bpy.context.active_object.rotation_mode = 'QUATERNION'\n");
        file.write("bpy.context.active_object.rotation_quaternion = (0,0,0,0)\n");
        if (isCamera) {
            // set the far plane out enough to see all the objects (we hope)
            file.write("bpy.context.active_object.data.clip_end = 2000\n");
            // give the camera a light that will follow it around
            file.write("cam = bpy.context.active_object\n");
            file.write("bpy.ops.object.lamp_add(type='POINT')\n");
            file.write("bpy.context.active_object.location = (0,-80000,0)\n");
            file.write("bpy.context.active_object.data.energy = 0.46\n");
            file.write("bpy.context.active_object.data.falloff_type = 'CONSTANT'\n");
            file.write("bpy.context.active_object.data.distance = 0.0\n");
            file.write("bpy.context.active_object.data.shadow_method = 'RAY_SHADOW'\n");
            file.write("lamp = bpy.context.active_object\n");
            file.write("select_object(cam)\n");
            file.write("lamp.select = True\n");
            file.write("bpy.ops.object.parent_set()\n");
        }
        sprintf(buf.data(),"bpy.context.active_object.location = (float(%f), float(%f), float(%f))\n",
                pos[Q_X], pos[Q_Y], pos[Q_Z]);
        file.write(buf.data());
        sprintf(buf.data(), "bpy.context.active_object.rotation_quaternion = "
                "(float(%f), float(%f), float(%f), float(%f))\n", orient[Q_W], orient[Q_X],
                orient[Q_Y], orient[Q_Z]);
        file.write(buf.data());
        // not sure if this needs modification to do something different for cameras
        if (!obj->isVisible()) {
            // hide it in the main view?
            file.write("bpy.context.active_object.hide = True\n");
            // hide it during rendering
            file.write("bpy.context.active_object.hide_render = True\n");
        }
        if (obj->isActive()) {
            file.write("bpy.context.scene.camera = bpy.context.active_object\n");
        }
    }
}

bool ProjectToBlenderAnimation::writeObjectKeyframes(QFile &file, QHash<SketchObject *, int> &objectIdxs,
                                                     SketchProject *proj, unsigned frameRate)
{
    QScopedPointer<char, QScopedPointerArrayDeleter<char> > buf(new char[4096]);
    unsigned frame = 0;
    double time = static_cast<double>(frame) / static_cast<double>(frameRate);
    while (!proj->goToAnimationTime(time)) {
        q_vec_type pos;
        q_type orient;
        sprintf(buf.data(), "\nbpy.context.scene.frame_set(%u)\n", frame);
        file.write(buf.data());
        for (QHashIterator<SketchObject *, int> it(objectIdxs); it.hasNext();) {
            SketchObject * obj = it.peekNext().key();
            int idx = it.next().value();
            sprintf(buf.data(), "select_object(myObjects[%d])\n", idx);
            file.write(buf.data());
            computeNewTranslate(pos,orient,obj);
            if (obj->getModel()->getSource(obj->getModelConformation())
                    == CAMERA_MODEL_KEY) {
                q_type tmp;
                q_from_axis_angle(tmp,0,1,0,Q_PI);
                q_mult(orient,orient,tmp);
            }
            sprintf(buf.data(),"bpy.context.active_object.location = (float(%f), float(%f), float(%f))\n",
                    pos[Q_X], pos[Q_Y], pos[Q_Z]);
            file.write(buf.data());
            file.write("bpy.context.active_object.keyframe_insert(data_path='location')\n");
            sprintf(buf.data(), "bpy.context.active_object.rotation_quaternion = "
                    "(float(%f), float(%f), float(%f), float(%f))\n", orient[Q_W], orient[Q_X],
                    orient[Q_Y], orient[Q_Z]);
            file.write(buf.data());
            file.write("bpy.context.active_object.keyframe_insert(data_path='rotation_quaternion')\n");
            // TODO - visibility and active camera stuff
        }
        frame++;
        time = static_cast<double>(frame) / static_cast<double>(frameRate);
    }
    sprintf(buf.data(), "bpy.data.scenes[\"Scene\"].frame_end = %u\n", frame);
    file.write(buf.data());
    file.write("\n\n");
    return file.error() == QFile::NoError;
}

bool ProjectToBlenderAnimation::writeHelperFunctions(QFile &file) {
    // make sure we have a clean tab context for helper functions
    file.write("\n\n");
    // helper functions for material creation and assignment
    // borrowed from Russ's code, he got them from:
    // http://wiki.blender.org/index.php/Dev:2.5/Py/Scripts/Cookbook/Code_snippets/Materials_and_textures
    file.write("# assigns a material to an object\n");
    file.write("def setMaterial(ob,mat):\n");
    file.write("\tme = ob.data\n");
    file.write("\tme.materials.append(mat)\n");
    file.write("\n");
    file.write("# creates a new material with the specified parameters\n");
    file.write("def makeMaterial(name, diffuse, specular, alpha):\n");
    file.write("\tmat = bpy.data.materials.new(name)\n");
    file.write("\tmat.diffuse_color = diffuse\n");
    file.write("\tmat.diffuse_shader = 'LAMBERT'\n");
    file.write("\tmat.diffuse_intensity = 1.0\n");
    file.write("\tmat.specular_color = specular\n");
    file.write("\tmat.specular_shader = 'COOKTORR'\n");
    file.write("\tmat.specular_intensity = 0.5\n");
    file.write("\tmat.alpha = alpha\n");
    file.write("\tmat.ambient = 1\n");
    file.write("\treturn mat\n");
    file.write("\n");
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
    // helper function to merge vertices that are too close together
    // used in the simplification code Russ wrote, but I made it into a helper
    file.write("# merge vertices that are should be shared between neighboring triangles\n"
               "# into one vertex.  Edits the selected object.\n");
    file.write("def merge_vertices():\n");
    file.write("\tbpy.ops.object.mode_set(mode='EDIT')\n");
    file.write("\tbpy.ops.mesh.remove_doubles()\n");
    file.write("\tbpy.ops.mesh.select_all(action='DESELECT')\n");
    file.write("\tbpy.ops.mesh.select_non_manifold()\n");
    file.write("\tbpy.ops.mesh.delete()\n");
    file.write("\tbpy.ops.object.mode_set(mode='OBJECT')\n");
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
    file.write("\tselect_named(newName)\n");
    // Set the object origin to its center of geometry (done by me in ModelManager code)
    file.write("\tbpy.ops.object.origin_set(type='ORIGIN_GEOMETRY',center='BOUNDS')\n");
    file.write("\tmerge_vertices()\n");
    file.write("\tbpy.ops.object.select_all(action='DESELECT')\n");
    file.write("\treturn object\n");
    file.write("\n");
    // helper function to grab the default camera that blender provides
    // so we only have to update one place when they change it
    file.write("# Gets the default camera object in blender\n"
               "# When blender changes, change this function\n");
    file.write("def getDefaultCamera():\n");
    file.write("\t return bpy.data.objects['Camera']\n");
    file.write("\n\n");
    return file.error() == QFile::NoError;
}
