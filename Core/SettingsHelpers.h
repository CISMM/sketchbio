#ifndef SETTINGSHELPERS_H
#define SETTINGSHELPERS_H

#include <QString>

namespace SettingsHelpers
{

/*
 * This function gets (and caches) the executable path for the given executable
 * name.  If the program cannot be found, it opens a dialog to allow the user to
 * choose the executable to use.  This checks most system locations before asking
 * the user.
 */
QString getSubprocessExecutablePath(const QString &executableName);

/*
 * This function sets the executable path for the given executable name
 * to the given path.  Use with care, since the path is assumed to be correct.
 */
void setSubprocessExecutablePath(const QString &executableName,
                       const QString &executablePath);
}

#endif // SETTINGSHELPERS_H
