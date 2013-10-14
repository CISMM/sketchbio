#ifndef CHIMERAOBJMAKER_H
#define CHIMERAOBJMAKER_H

#include "abstractsingleprocessrunner.h"

class QString;
class QProcess;
class QTemporaryFile;

/*
 * This class is designed to run UCSF Chimera as a subprocess and create OBJ files to
 * be read as models.  It uses Qt's signal/slots system to do this in the background.
 *
 * For usage, see the parent class, SubprocessRunner.  There is a factory method in
 * SubprocessUtils to create one of these... this class should not be used directly
 *
 */

class ChimeraVTKExportRunner : public AbstractSingleProcessRunner
{
    Q_OBJECT
public:
    // constructor
    ChimeraVTKExportRunner(const QString &pdbId, const QString &vtkFile, int threshold,
                           const QString &chainsToDelete, bool shouldExportWholeBioUnit,
                           QObject *parent = 0);
    // destructor
    virtual ~ChimeraVTKExportRunner();
    virtual bool isValid();
    
    // starts the subprocess
    virtual void start();

    // checks if the subprocess succeeded
    virtual bool didProcessSucceed(QString output);


private:

    // fields
    QTemporaryFile *cmdFile;
    QString resultFile;
    bool valid;
};

#endif // CHIMERAOBJMAKER_H
