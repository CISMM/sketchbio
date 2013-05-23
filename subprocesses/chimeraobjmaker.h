#ifndef CHIMERAOBJMAKER_H
#define CHIMERAOBJMAKER_H

#include "subprocessrunner.h"

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

class ChimeraOBJMaker : public SubprocessRunner
{
    Q_OBJECT
public:
    // constructor
    ChimeraOBJMaker(QString pdbId, QString objFile, int threshold = 0,
                    QObject *parent = 0);
    // destructor
    virtual ~ChimeraOBJMaker();
    virtual bool isValid();
    
public:
    // starts the subprocess
    virtual void start();
    // cancels the subprocess
    virtual void cancel();

private slots:
    void processResult(int status);

private:

    // fields
    QProcess *chimera;
    QTemporaryFile *cmdFile;
    bool valid;
};

#endif // CHIMERAOBJMAKER_H
