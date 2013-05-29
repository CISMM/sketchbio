#ifndef SKETCHMODEL_H
#define SKETCHMODEL_H

#include <vtkSmartPointer.h>
#include <QString>
#include <QVector>
#include <QHash>
#include <quat.h>

// forward declarations
class PQP_Model;
class vtkPolyDataAlgorithm;
class vtkTransformPolyDataFilter;
class vtkPolyData;

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

vtkPolyDataAlgorithm *read(QString filename);
}

/*
 *
 * This class holds general data about a type of object such as a protein.  The type
 * can have subtypes, known as configurations, that can vary in geometry while still
 * being the same king of object.  The model also holds filenames for multiple resolutions
 * of each configuration and counts how many current uses each one as, allowing for dynamic
 * control of the resolution (and thus render/collision detection time)
 *
 */

class SketchModel : public QObject
{
    Q_OBJECT
public:
    // Creates a SketchModel with no configurations and sets the only parameters that
    // are the same across all configurations.
    // Parameters: iMass - the inverse mass of the model
    //              iMoment - the invers moment of inertia of the model
    //              applyRotations - leave as default unless there is a good reason
    //                              that the model's orientation relative to the object
    //                              that contains it matters.  In that case set to false.
    SketchModel(double iMass, double iMoment, bool applyRotations = true,
                QObject *parent = 0);
    // cleans up the SketchModel
    virtual ~SketchModel();

    // gets the number of configurations
    int getNumberOfConformations() const;
    // gets the resolution level being used for the given configuration
    ModelResolution::ResolutionType getResolutionLevel(int configurationNum) const;
    // gets the vtk "source" for the data of the given configuration
    // note: this source will already have been passed through a transform
    // filter to center it (translation) and put its oriented bounding box in line with
    // its local coordinate axes (so that the axis aligned bounding box given by the
    /// vtkPolyData's GetBounds() will be the oriented bounding box)
    // note: rotation is only applied if shouldRotateToAxisAligned is true
    vtkPolyDataAlgorithm *getVTKSource(int configurationNum);
    // Gets the translation that was applied to the data read in
    // for the given configuration
    void getTranslation(q_vec_type out, int configuration) const;
    // Gets the rotation that was applied to the data read in
    // for the given configuration
    void getRotation(q_type rotationOut, int configuration) const;
    bool shouldRotate() const;
    // Gets the scale that was applied to the data read in
    // for the given configuration
    double getScale(int configuration) const;
    // Gets the collision model for the given configuration
    PQP_Model *getCollisionModel(int configurationNum);
    // Gets the number of uses for a configuration
    int getNumberOfUses(int configuration) const;
    // Gets the filename for the geometry file that is used by the given
    // configuration at the given resolution.  Will return an empty string
    // if the the configuration at that resolution has no geometry file yet.
    QString getFileNameFor(int configuration,
                           ModelResolution::ResolutionType resolution) const;
    // Gets the source for the given configuration
    // The source is the data from which the geometry was created, such as a
    // PDB file.  If the geometry is only geometry and was not generated from
    // some other source, then this will be the full resolution geometry file
    QString getSource(int configuration) const;
    // Gets the inverse mass of the model (constant across all configurations)
    double getInverseMass() const;
    // Gets the inverse moment of inertial of the model (constant across all configurations)
    double getInverseMomentOfInertia() const;

    // Adds a new configuration with the given source and full resolution filename
    // other resolutions will be generated later
    void addConformation(QString src, QString fullResolutionFileName);
    // Incrementes the use count on a configuration (used to determine which
    // configurations need simplifying). This should be called whenever an
    // object is created that uses the configuration or when the configuration
    // of an object changes so that it now uses the configuration.
    void incrementUses(int configuration);
    // Decrements the use count on a configuration (used to determine which
    // configurations need simplifying). This should be called whenever an
    // object is deleted that uses the configuration, or when the configuration
    // of an object changes so that it no longer uses the configuration.
    void decrementUses(int configuration);
public slots:
    // Sets the geometery file for the given configuration and resolution
    void addSurfaceFileForResolution(int configuration,
                                     ModelResolution::ResolutionType resolution,
                                     QString filename);
    // Tells the given configuration to use the given resolution.  If no
    // geometry file for the given resolution exists, then it does nothing.
    void setReslutionForConfiguration(int configuration,
                                      ModelResolution::ResolutionType resolution);
private:
    // Note: configurations are unique "models" that happen to the the same
    //       real world object in a different state.  Thus they are stored
    //       in the same SketchModel object so that configuration can be changed
    //       without looking up a new model.
    // the number of configurations
    int numConformations;
    // the resolution, indexed by the configuration
    QVector< ModelResolution::ResolutionType > resolutionLevelForConf;
    // the data (or more exactly, the transform filter that is attached to it)
    // indexed by configuration
    QVector< vtkSmartPointer< vtkTransformPolyDataFilter > > modelDataForConf;
    // the collision model, indexed by configuration
    QVector< PQP_Model * > collisionModelForConf;
    // the number of uses of the model, indexed by configuration
    QVector< int > useCount;
    // the source of the data (pdb id/file, etc)  -- not the surface but what made it
    // unless the object is nothing but a surface with no underlying meaning, then this
    // is the file that defines it -- again indexed by configuration
    QVector< QString > source;
    // the file names of the structure data, hashed by a combination of
    // configuration number and resolution
    QHash< QPair< int, ModelResolution::ResolutionType >, QString > fileNames;
    // mass, but save the trouble of inverting it to divide
    double invMass;
    // moment of inerita, but save the trouble of inverting it to divide
    double invMomentOfInertia;
    // true (default) means that the models should be rotated so that the oriented
    // bounding box is also the axis-aligned bounding box.
    bool shouldRotateToAxisAligned;
};


#endif // SKETCHMODEL_H
