#include "projecttoblenderanimation.h"

#include <cmath>
#include <iostream>

#include <QFile>
#include <QScopedPointer>

#include <vtkSmartPointer.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkColorTransferFunction.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <vtkVRMLWriter.h>

#include <modelutilities.h>
#include <sketchmodel.h>
#include <modelmanager.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>

/*
 * This object is the combined key for the model and color to vrml file hash.
 */
struct ModelColorMapKey
{
    SketchModel *model;
    int conformation;
    ColorMapType::Type cmap;
    QString arrayToColorBy;
    ModelColorMapKey(SketchModel *m, int c, ColorMapType::Type cm,
                     const QString &a)
        :
          model(m),
          conformation(c),
          cmap(cm),
          arrayToColorBy(a)
    {
    }
};

// make sure equal keys compare as equal
inline bool operator ==(const ModelColorMapKey &m1, const ModelColorMapKey &m2)
{
    return (m1.model == m2.model) && (m1.conformation == m2.conformation) &&
            (m1.cmap == m2.cmap) && (m1.arrayToColorBy == m2.arrayToColorBy);
}

// make sure we can hash the key
inline uint qHash(const ModelColorMapKey &key)
{
    return qHash(key.model) ^ qHash(key.conformation) * qHash(key.cmap) ^
            qHash(key.arrayToColorBy);
}

//#########################################################################################
//#########################################################################################
unsigned ProjectToBlenderAnimation::timeToBlenderFrameNum(double time, unsigned frameRate) {
    return floor(time * frameRate + 0.5);
}

bool ProjectToBlenderAnimation::writeProjectBlenderFile(QFile &file, SketchProject *proj,
                                                        const QString &modulePath)
{
    // python imports... math and bpy
    file.write("import bpy\n");
    file.write("import math\n");
    bool success = writeHelperFunctions(file,modulePath);
    file.write("select_named('Cube')\n");
    file.write("bpy.ops.object.delete()\n");
    file.write("select_named('Lamp')\n");
    file.write("bpy.ops.object.delete()\n");
    file.write("modelObjects = list()\n");
    file.write("myObjects = list()\n");
    QHash<SketchObject *, int> objectIdxs;
    success &= writeCreateObjects(file,objectIdxs, proj);
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

// Writes code to create a base object to copy for the model and adds it to the list modelObjects
// with indices that populate the given QHash so that a model's value in the map is its base object's
// index in the list
static inline void writeCreateModel(
        QFile &file, QHash< ModelColorMapKey, int > &modelIdxs,
        ModelColorMapKey &model)
{
    modelIdxs.insert(model, modelIdxs.size());
    QString source = model.model->getSource(model.conformation);
    QString fname = model.model->getFileNameFor(model.conformation,
                                                ModelResolution::FULL_RESOLUTION);
    if (source != CAMERA_MODEL_KEY)
    {
        if (fname.endsWith(".vtk"))
        {
            double range[2] = {0.0, 1.0};
            vtkPointData *pointData = model.model->getVTKSurface(model.conformation)
                    ->GetOutput()->GetPointData();
            if (pointData->HasArray(model.arrayToColorBy.toStdString().c_str()))
            {
                pointData->GetArray(model.arrayToColorBy.toStdString().c_str())
                        ->GetRange(range);
            }
            vtkSmartPointer< vtkColorTransferFunction > colors =
                    vtkSmartPointer< vtkColorTransferFunction >::Take(
                        ColorMapType::getColorMap(model.cmap,range[0],range[1])
                    );
            fname = ProjectToBlenderAnimation::generateVRMLFileFor(
                        fname, model.arrayToColorBy.toStdString().c_str(),colors);
        }
    }
#if defined(_WIN32)
	fname = fname.replace("/","\\\\");
#endif
    QString line = "obj = makeDefaultObject('%1','%2')\n";
    line = line.arg(source,fname);
    file.write(line.toStdString().c_str());
    file.write("modelObjects.append(obj)\n");
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

// Writes code to create a single object and recurse on groups (only objects with the an individual model
// are keyframed, groups are ignored and each object within them is added)
static inline void writeCreateObject(
        QFile &file, QHash< ModelColorMapKey, int> &modelIdxs,
        QHash<SketchObject *, int> &objectIdxs, int &objectsLen,
        SketchProject *proj, SketchObject *obj)
{
    if (obj->numInstances() != 1)
    {
        QListIterator<SketchObject *> it(*obj->getSubObjects());
        while (it.hasNext())
        {
            writeCreateObject(file,modelIdxs, objectIdxs, objectsLen, proj, it.next());
        }
    }
    else
    {
        QScopedPointer<char, QScopedPointerArrayDeleter<char> > buf(new char[4096]);
        SketchModel *model  = obj->getModel();
        int conformation = obj->getModelConformation();
        ModelColorMapKey key(model,conformation,
                             obj->getColorMapType(),
                             obj->getArrayToColorBy());
        if (!modelIdxs.contains(key))
        {
            writeCreateModel(file,modelIdxs,key);
        }
        int idx = modelIdxs.value(key);
        objectIdxs.insert(obj,objectsLen++);
        q_vec_type pos;
        q_type orient, tmp;
        computeNewTranslate(pos,orient,obj);
        q_from_axis_angle(tmp,0,1,0,Q_PI);
        bool isCamera = false;
        if (model->getSource(obj->getModelConformation()) == CAMERA_MODEL_KEY)
        {
            q_mult(orient,orient,tmp);
            isCamera = true;
        }
        const char *isCameraString, *isVisibleString, *isActiveString;
        isCameraString = (isCamera) ? "True" : "False";
        isVisibleString = (obj->isVisible()) ? "True" : "False";
        isActiveString = (obj->isActive()) ? "True" : "False";
        sprintf(buf.data(),"obj = createInstance(modelObjects[%u],%s,"
                "(float(%f),float(%f),float(%f)),(float(%f),float(%f),float(%f),"
                "float(%f)),%s,%s)\n",
                idx,isCameraString,pos[Q_X],pos[Q_Y],
                pos[Q_Z],orient[Q_W],orient[Q_X],orient[Q_Y],orient[Q_Z],
                isVisibleString,isActiveString);
        file.write(buf.data());
        file.write("myObjects.append(obj)\n");
    }
}

bool ProjectToBlenderAnimation::writeCreateObjects(
        QFile &file,
        QHash<SketchObject *, int> &objectIdxs, SketchProject *proj)
{
    QHash< ModelColorMapKey, int > modelIdxs;
    proj->goToAnimationTime(0.0);
    WorldManager *world = proj->getWorldManager();
    file.write("\n\n");
    file.write("# Duplicate objects so we have on for each object in SketchBio, plus a template for each type.\n");
    int objectsLen = 0;
    QScopedPointer<char,QScopedPointerArrayDeleter<char> > buf(new char[4096]);
    QListIterator<SketchObject *> it = world->getObjectIterator();
    while (it.hasNext())
    {
        writeCreateObject(file,modelIdxs, objectIdxs, objectsLen, proj, it.next());
    }
    file.write("\n\n");
    return file.error() == QFile::NoError;
}


bool ProjectToBlenderAnimation::writeObjectKeyframes(QFile &file, QHash<SketchObject *, int> &objectIdxs,
                                                     SketchProject *proj, unsigned frameRate)
{
    QScopedPointer<char, QScopedPointerArrayDeleter<char> > buf(new char[4096]);
    unsigned frame = 0;
    file.write("bpy.data.scenes[\"Scene\"].frame_start = 0\n");
    double time = static_cast<double>(frame) / static_cast<double>(frameRate);
    while (!proj->goToAnimationTime(time))
    {
        q_vec_type pos;
        q_type orient;
        sprintf(buf.data(), "\nbpy.context.scene.frame_set(%u)\n", frame);
        file.write(buf.data());
        for (QHashIterator<SketchObject *, int> it(objectIdxs); it.hasNext();)
        {
            SketchObject * obj = it.peekNext().key();
            int idx = it.next().value();
            computeNewTranslate(pos,orient,obj);
            if (obj->getModel()->getSource(obj->getModelConformation())
                    == CAMERA_MODEL_KEY)
            {
                q_type tmp;
                q_from_axis_angle(tmp,0,1,0,Q_PI);
                q_mult(orient,orient,tmp);
            }
            sprintf(buf.data(),"makeKeyframe(myObjects[%d], (%g, %g, %g), (%g, %g, %g, %g))\n",
                    idx, pos[Q_X], pos[Q_Y], pos[Q_Z], orient[Q_W], orient[Q_X], orient[Q_Y],
                    orient[Q_Z]);
            file.write(buf.data());
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

bool ProjectToBlenderAnimation::writeHelperFunctions(QFile &file, const QString& modulePath)
{
    file.write("import sys\n");
    QString line("sys.path.insert(0,'%1')\n");
    line = line.arg(modulePath);
    file.write(line.toStdString().c_str());
    file.write("from mybpyhelpers import *\n\n");
    return file.error() == QFile::NoError;
}

QString ProjectToBlenderAnimation::generateVRMLFileFor(
        QString vtkFile, const char *arrayName, vtkColorTransferFunction *colorMap)
{
    vtkSmartPointer< vtkPolyDataAlgorithm > model =
            vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                ModelUtilities::read(vtkFile.toStdString().c_str()));
    vtkSmartPointer< vtkPolyDataAlgorithm > surface =
            vtkSmartPointer< vtkPolyDataAlgorithm >::Take(
                ModelUtilities::modelSurfaceFrom(model));
    vtkSmartPointer< vtkVRMLWriter > writer =
            vtkSmartPointer< vtkVRMLWriter >::New();
    writer->SetInputConnection(surface->GetOutputPort());
    writer->SetFileName((vtkFile + ".wrl").toStdString().c_str());
    if (arrayName != NULL)
    {
        writer->SetArrayToColorBy(arrayName);
    }
    if (colorMap != NULL)
    {
        writer->SetColorMap(colorMap);
    }
    writer->Write();
    return vtkFile + ".wrl";
}
