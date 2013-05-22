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
QString getSubprocessExecutablePath(QString executableName);


/*
 * This method returns a valid SubprocessRunner to make an obj file
 * from a pdb id in Chimera or NULL.  There is no need to check if
 * the returned object is valid.  Simply check for NULL.  Then connect
 * it to the signals/slots and call start().
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *makeChimeraOBJFor(QString pdbID, QString objFile);


/*
 * This method returns a valid SubprocessRunner to make a blender animation
 * from a project or NULL.  There is no need to check if the returned object
 * is valid, simply check for NULL.  Then connect it to the signals/slots
 * and call start().
 *
 * For detailed usage information, see subprocessrunner.h
 */
SubprocessRunner *createAnimationFor(SketchProject *proj, QString &animationFile);

}

#endif // SUBPROCESSUTILS_H
