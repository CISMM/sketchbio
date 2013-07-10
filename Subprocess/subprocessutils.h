#ifndef SUBPROCESSUTILS_H
#define SUBPROCESSUTILS_H

class QString;
class SubprocessRunner;
class SketchProject;

namespace SubprocessUtils
{

/*
 * This method gets (and caches) the executable path for the given executable
 * name.  If the program cannot be found, it opens a dialog to allow the user to
 * choose the executable to use.  This checks most system locations before asking
 * the user.
 */
QString getSubprocessExecutablePath(const QString &executableName);

/*
 * This method gets the directory to add to the python path so that the ExportVTK
 * module can be imported correctly with 'import ExportVTK'.  As this will vary by
 * operating system, it is getting abstracted out to here.
 */
QString getChimeraVTKExtensionDir();


/*
 * This method returns a valid SubprocessRunner to make an obj file
 * from a pdb id in Chimera or NULL.  There is no need to check if
 * the returned object is valid.  Simply check for NULL.  Then connect
 * it to the signals/slots and call start().
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *makeChimeraSurfaceFor(const QString &pdbID, const QString &vtkFile, int threshold,
                                    const QString &chainsToDelete);

/*
 * This method returns a valid SubprocessRunner to make an obj file
 * from a pdb id in PyMOL or NULL.  There is no need to check if
 * the returned object is valid.  Simply check for NULL.  Then connect
 * it to the signals/slots and call start().
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *makePyMolOBJFor(const QString &pdbID, const QString &saveDir);

/*
 * This method returns a valid SubprocessRunner to make a blender animation
 * from a project or NULL.  There is no need to check if the returned object
 * is valid, simply check for NULL.  Then connect it to the signals/slots
 * and call start().
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *createAnimationFor(SketchProject *proj, const QString &animationFile);

/*
 * This method returns a valid SubprocessRunner to run Blender to decimate an
 * obj file or NULL.  There is no need to check if the returned object
 * is valid, simply check for NULL.  Then connect it to the signals/slots
 * and call start().
 *
 * The parameter percentOfOriginal controls how many triangles will be in the
 * resulting file (in percent of the original amount [ .5 is 50% ] ).
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *simplifyObjFileByPercent(const QString &objFile, double percentOfOriginal);

/*
 * This method returns a valid SubprocessRunner to run Blender to decimate an
 * obj file or NULL.  There is no need to check if the returned object
 * is valid, simply check for NULL.  Then connect it to the signals/slots
 * and call start().
 *
 * The parameter percentOfOriginal controls how many triangles will be
 * (absolute amount)
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *simplifyObjFile(const QString &objFile, int triangles);

/*
 * This method returns a valid SubprocessRunner to run various subprocesses to
 * create a model and object from a PDB id or NULL.  There is no need to
 * check if the returned object is valid, simply check for NULL.  Then
  *connect it to the signals/slots and call start().
 *
 * Note: chainsToDelete - the ids (letters) of chains to delete before surfacing
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *loadFromPDB(SketchProject *proj, const QString &pdb,
                              const QString &chainsToDelete);
}

#endif // SUBPROCESSUTILS_H
