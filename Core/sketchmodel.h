#ifndef SKETCHMODEL_H
#define SKETCHMODEL_H

#include <quat.h>

#include <vtkSmartPointer.h>
class vtkPolyDataAlgorithm;
class vtkTransformPolyDataFilter;
class vtkPolyData;

class QString;
class QDir;
#include <QVector>
#include <QHash>

class PQP_Model;

namespace ModelResolution
{
enum ResolutionType
{
    FULL_RESOLUTION,
    SIMPLIFIED_FULL_RESOLUTION,
    SIMPLIFIED_5000,
    SIMPLIFIED_2000,
    SIMPLIFIED_1000
};
}

namespace ModelUtilities
{
void makePQP_Model(PQP_Model *m1, vtkPolyData *polyData);

#ifdef PQP_UPDATE_EPSILON
void updatePQP_Model(PQP_Model &model,vtkPolyData &polyData);
#endif

vtkPolyDataAlgorithm *read(const QString &filename);

// Writes the given algorithm's output to a file whose name is based on the descr string
// and returns the name of the file it created (file is in the current directory by
// default)
QString createFileFromVTKSource(vtkPolyDataAlgorithm *algorithm, const QString &descr);
QString createFileFromVTKSource(vtkPolyDataAlgorithm *algorithm, const QString &descr,
                                const QDir &dir);
QString createSourceNameFor(const QString &pdbId, const QString &chainsLeftOut);

void vtkConvertAsciiToBinary(const QString &filename);
}

/*
 *
 * This class holds general data about a type of object such as a protein.  The type
 * can have subtypes, known as conformations, that can vary in geometry while still
 * being the same king of object.  The model also holds filenames for multiple resolutions
 * of each conformation and counts how many current uses each one as, allowing for dynamic
 * control of the resolution (and thus render/collision detection time)
 *
 */

class SketchModel : public QObject
{
    Q_OBJECT
public:
    // Creates a SketchModel with no conformations and sets the only parameters that
    // are the same across all conformations.
    // Parameters: iMass - the inverse mass of the model
    //              iMoment - the invers moment of inertia of the model
    SketchModel(double iMass, double iMoment, QObject *parent = 0);
    // cleans up the SketchModel
    virtual ~SketchModel();

    // gets the number of conformations
    int getNumberOfConformations() const;
    // gets the resolution level being used for the given conformation
    ModelResolution::ResolutionType getResolutionLevel(int conformationNum) const;
    // gets the vtk "source" for the data of the given conformation
    // note: this source will already have been passed through a transform
    // filter to center it (translation) and put its oriented bounding box in line with
    // its local coordinate axes (so that the axis aligned bounding box given by the
    /// vtkPolyData's GetBounds() will be the oriented bounding box)
    // note: rotation is only applied if shouldRotateToAxisAligned is true
    vtkPolyDataAlgorithm *getVTKSource(int conformationNum);
    // Gets the collision model for the given conformation
    PQP_Model *getCollisionModel(int conformationNum);
    // Gets the number of uses for a conformation
    int getNumberOfUses(int conformation) const;
    bool hasFileNameFor(int conformation,
                        ModelResolution::ResolutionType resolution) const;
    // Gets the filename for the geometry file that is used by the given
    // conformation at the given resolution.  Will return an empty string
    // if the the conformation at that resolution has no geometry file yet.
    QString getFileNameFor(int conformation,
                           ModelResolution::ResolutionType resolution) const;
    // Gets the source for the given conformation
    // The source is the data from which the geometry was created, such as a
    // PDB file.  If the geometry is only geometry and was not generated from
    // some other source, then this will be the full resolution geometry file
    const QString &getSource(int conformation) const;
    // Gets the inverse mass of the model (constant across all conformations)
    double getInverseMass() const;
    // Gets the inverse moment of inertial of the model (constant across all conformations)
    double getInverseMomentOfInertia() const;

    // Adds a new conformation with the given source and full resolution filename
    // other resolutions will be generated later.  Returns the conformation number
    // of the conformation added.
    int addConformation(const QString &src, const QString &fullResolutionFileName);
    // Incrementes the use count on a conformation (used to determine which
    // conformations need simplifying). This should be called whenever an
    // object is created that uses the conformation or when the conformation
    // of an object changes so that it now uses the conformation.
    void incrementUses(int conformation);
    // Decrements the use count on a conformation (used to determine which
    // conformations need simplifying). This should be called whenever an
    // object is deleted that uses the conformation, or when the conformation
    // of an object changes so that it no longer uses the conformation.
    void decrementUses(int conformation);
public slots:
    // Sets the geometery file for the given conformation and resolution
    void addSurfaceFileForResolution(int conformation,
                                     ModelResolution::ResolutionType resolution,
                                     const QString &filename);
    // Tells the given conformation to use the given resolution.  If no
    // geometry file for the given resolution exists, then it does nothing.
    void setReslutionForConformation(int conformation,
                                      ModelResolution::ResolutionType resolution);
private:
    // sets the resolution level based on the number of uses of the given
    // conformation
    void setResolutionLevelByUses(int conformation);
    // Note: conformations are unique "models" that happen to the the same
    //       real world object in a different state.  Thus they are stored
    //       in the same SketchModel object so that conformation can be changed
    //       without looking up a new model.
    // the number of conformations
    int numConformations;
    // the resolution, indexed by the conformation
    QVector< ModelResolution::ResolutionType > resolutionLevelForConf;
    // the data (or more exactly, the transform filter that is attached to it)
    // indexed by conformation
    QVector< vtkSmartPointer< vtkTransformPolyDataFilter > > modelDataForConf;
    // the collision model, indexed by conformation
    QVector< PQP_Model * > collisionModelForConf;
    // the number of uses of the model, indexed by conformation
    QVector< int > useCount;
    // the source of the data (pdb id/file, etc)  -- not the surface but what made it
    // unless the object is nothing but a surface with no underlying meaning, then this
    // is the file that defines it -- again indexed by conformation
    QVector< QString > source;
    // the file names of the structure data, hashed by a combination of
    // conformation number and resolution
    QHash< QPair< int, ModelResolution::ResolutionType >, QString > fileNames;
    // mass, but save the trouble of inverting it to divide
    double invMass;
    // moment of inerita, but save the trouble of inverting it to divide
    double invMomentOfInertia;
};


#endif // SKETCHMODEL_H
