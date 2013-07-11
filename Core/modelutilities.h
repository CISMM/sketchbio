#ifndef MODELUTILITIES_H
#define MODELUTILITIES_H

#include <QString>
class QDir;

class vtkPolyData;
class vtkAlgorithm;
class vtkPolyDataAlgorithm;

class PQP_Model;

/*
 * This is a namespace for utility methods used for operations to do with models.
 *
 * It mostly contains methods that perform multi-step vtk operations and return the
 * result
 *
 */
namespace ModelUtilities
{
/*
 * Initializes a PQP_Model from the given vtkPolyData
 */
void makePQP_Model(PQP_Model *m1, vtkPolyData *polyData);

#ifdef PQP_UPDATE_EPSILON
/*
 * Updates a PQP_Model with the given vtkPolyData.  Note, may not
 * work with the new makePQP_Model code since that changes the ids
 * of the triangles after building the model to help with indexing
 * later
 */
void updatePQP_Model(PQP_Model &model,vtkPolyData &polyData);
#endif

/*
 * Reads the given file and returns the resulting vtkPolyDataAlgorithm or throws
 * a const char * if it does not know how to read the file.
 */
vtkPolyDataAlgorithm *read(const QString &filename);
/*
 * Returns a vtkPolyDataAlgorithm whose output is the surface part of the model
 */
vtkPolyDataAlgorithm *modelSurfaceFrom(vtkPolyDataAlgorithm *rawModel);
/*
 * Returns a vtkAlgorithm whose output is the atoms and bonds of the model, if
 * available (or NULL if no atom data is available).
 */
vtkAlgorithm *modelAtomsFrom(vtkPolyDataAlgorithm *rawModel);

// Writes the given algorithm's output to a file whose name is based on the descr string
// and returns the name of the file it created (file is in the current directory by
// default)
QString createFileFromVTKSource(vtkPolyDataAlgorithm *algorithm, const QString &descr);
QString createFileFromVTKSource(vtkPolyDataAlgorithm *algorithm, const QString &descr,
                                const QDir &dir);
/*
 * Returns a standard formatted source for the model containing the given pdb id with
 * the given chains left out.
 */
QString createSourceNameFor(const QString &pdbId, const QString &chainsLeftOut);

void vtkConvertAsciiToBinary(const QString &filename);
}


#endif // MODELUTILITIES_H
