#include "ProjectToFlorosim.h"

static const char TOP_OF_FLOROSIM_FILE[] =
        #include "FlorosimExportTopOfFile.h"
        ;
static const char BOTTOM_OF_FLOROSIM_FILE[] =
        #include "FlorosimExportBottomOfFile.h"
        ;
#include <iostream>

#include <QFile>
#include <QString>

#include <vtkTransform.h>

#include <sketchmodel.h>
#include <sketchobject.h>
#include <worldmanager.h>
#include <sketchproject.h>

#define PRINT_ERROR(x) std::cout << "ERROR: " << x << std::endl

namespace ProjectToFlorosim
{

/*
 * Writes out the florosim xml for the object (or its sub-objects) to
 * be loaded into Florosim
 *
 */
static inline void writeObjToFlorosim(QFile& file, SketchObject* obj)
{
    if (obj->numInstances() != 1)
    {
        const QList< SketchObject* >& objs = *obj->getSubObjects();
        for (int i = 0; i < objs.size(); i++)
        {
            writeObjToFlorosim(file,objs[i]);
        }
        return;
    }
    q_vec_type pos;
    vtkTransform* tfrm = obj->getLocalTransform();
    tfrm->GetPosition(pos);
    q_vec_scale(pos,0.1,pos);
    pos[0] += 500;
    pos[1] += 500;
    pos[2] += 500;
    double wxyz[4];
    tfrm->GetOrientationWXYZ(wxyz);
    QString fname = obj->getModel()->getFileNameFor(obj->getModelConformation(),
                                                    ModelResolution::SIMPLIFIED_1000);
    QString line;
    file.write("<ImportedGeometry>\n");
    file.write("<Name value=\"Geometry\"/>\n");
    file.write("<Visible value=\"true\"/>\n");
    line = "<PositionX value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(pos[Q_X]).toStdString().c_str());
    line = "<PositionY value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(pos[Q_Y]).toStdString().c_str());
    line = "<PositionZ value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(pos[Q_Z]).toStdString().c_str());
    line = "<RotationAngle value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(wxyz[0]).toStdString().c_str());
    line = "<RotationVectorX value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(wxyz[1]).toStdString().c_str());
    line = "<RotationVectorY value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(wxyz[2]).toStdString().c_str());
    line = "<RotationVectorZ value=\"%1\" optimize=\"false\"/>\n";
    file.write(line.arg(wxyz[3]).toStdString().c_str());
    // scale of 0.1, sketchbio uses angstroms (pdb units) and Florosim uses nanometers
    file.write("<Scale value=\"0.100000\" optimize=\"false\"/>\n");
    line = "<FileName value=\"%1\"/>\n";
    file.write(line.arg(fname).toStdString().c_str());
    file.write("<SurfaceFluorophoreModel enabled=\"true\" channel=\"green\" "
               "intensityScale=\"1.000000\" optimize=\"false\" density=\""
               "400.000000\" numberOfFluorophores=\"1\" samplingMode=\""
               "fixedDensity\" samplePattern=\"singlePoint\" numberOfRing"
               "Fluorophores=\"2000\" ringRadius=\"10.000000\" randomizePattern"
               "Orientations=\"false\"/>\n");
    file.write("<VolumeFluorophoreModel enabled=\"false\" channel=\"all\" "
               "intensityScale=\"1.000000\" optimize=\"false\" density=\""
               "100.000000\" numberOfFluorophores=\"0\" samplingMode=\""
               "fixedDensity\" samplePattern=\"singlePoint\" numberOfRing"
               "Fluorophores=\"2\" ringRadius=\"10.000000\" randomizePattern"
               "Orientations=\"false\"/>\n");
    file.write("<GridFluorophoreModel enabled=\"false\" channel=\"all\" "
               "intensityScale=\"1.000000\" optimize=\"false\" sampleSpacing="
               "\"50.000000\"/>\n");
    file.write("</ImportedGeometry>\n");
}

bool writeProjectToFlorosim(SketchBio::Project* project, const QString& filename)
{
    QFile f(filename);
    if (f.exists())
    {
        if (!f.remove())
        {
            PRINT_ERROR("Could not remove existing file to rewrite.");
            return false;
        }
    }
    if (!f.open(QFile::WriteOnly))
    {
        PRINT_ERROR("Could not open file to write.");
        return false;
    }
    f.write(TOP_OF_FLOROSIM_FILE);
    const QList< SketchObject* >& objs = *project->getWorldManager().getObjects();
    for (int i = 0; i < objs.size(); i++)
    {
        writeObjToFlorosim(f,objs[i]);
    }
    f.write(BOTTOM_OF_FLOROSIM_FILE);
    f.close();
    if (f.error() != QFile::NoError)
    {
        PRINT_ERROR(f.errorString().toStdString());
        return false;
    }
    return true;
}

}
