#ifndef PYMOLOBJMAKER_H
#define PYMOLOBJMAKER_H

#include "subprocessrunner.h"

class QProcess;
class QFile;
class QTemporaryFile;

/*
 *
 * This is a SubprocessRunner to use PyMOL to generate an obj file surface
 * for a PDB ID.
 *
 * For more information about use see subprocessrunner.h
 *
 */
class PymolOBJMaker : public SubprocessRunner
{
    Q_OBJECT
public:
    explicit PymolOBJMaker(const QString &pdbId, const QString &dirName, QObject *parent = 0);

    virtual ~PymolOBJMaker();
    virtual void start();
    virtual void cancel();
    virtual bool isValid();
private slots:
    void newOutputReady();
    void checkForFileWritten();
    void pymolFinished(int status);
    void blenderFinished(int status);
private:
    QProcess *pymol;
    QProcess *blender;
    QTemporaryFile *pmlFile;
    QTemporaryFile *bpyFile;
    QString saveDir;
    QFile *objFile;
    bool valid;
};

#endif // PYMOLOBJMAKER_H
