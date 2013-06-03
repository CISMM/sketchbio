#include "modelfrompdbrunner.h"
#include "subprocessutils.h"

#include <sketchproject.h>
#include <sketchmodel.h>
#include <sketchioconstants.h>

#include <QDebug>

ModelFromPDBRunner::ModelFromPDBRunner(SketchProject *proj, QString &pdb, QObject *parent) :
    SubprocessRunner(parent),
    pdbId(pdb.toLower()),
    project(proj),
    model(NULL),
    currentRunner(NULL),
    stepNum(0)
{
}

ModelFromPDBRunner::~ModelFromPDBRunner()
{
}

void ModelFromPDBRunner::start()
{
    currentRunner = SubprocessUtils::makeChimeraOBJFor(
                pdbId, project->getProjectDir() + "/" + pdbId + ".obj");
    if (currentRunner == NULL) {
        emit finished(false);
        deleteLater();
    }
    connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
    currentRunner->start();
    emit statusChanged("Creating Connolly surface for " + pdbId);
    qDebug() << "Creating Connolly surface for " << pdbId;
}

void ModelFromPDBRunner::cancel()
{
    currentRunner->cancel();
    deleteLater();
}

bool ModelFromPDBRunner::isValid()
{
    return true;
}

void ModelFromPDBRunner::stepFinished(bool succeeded)
{
    if (succeeded)
    {
        QString simplified = (project->getProjectDir() + "/" + pdbId + "_isosurface.obj").trimmed();
        switch (stepNum)
        {
        case 0:
            model = project->addModelFromFile(
                        "PDB:" + pdbId, project->getProjectDir() + "/" + pdbId + ".obj",
                        DEFAULT_INVERSE_MASS, DEFAULT_INVERSE_MOMENT);
            if (! model->hasFileNameFor(0,ModelResolution::SIMPLIFIED_1000))
            {
                currentRunner = SubprocessUtils::makeChimeraOBJFor(pdbId,simplified,5);
                if (currentRunner == NULL)
                {
                    emit finished(true);
                    deleteLater();
                }
                else
                {
                    connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
                    currentRunner->start();
                    emit statusChanged("Creating simpler surface geometry for " + pdbId);
                    qDebug() << "Creating a simpler surface geometry for " << pdbId;
                }
            }
            else
            {
                emit finished(true);
                deleteLater();
            }
            break;
        case 1:
            model->addSurfaceFileForResolution(0,ModelResolution::SIMPLIFIED_FULL_RESOLUTION,
                                               simplified);
            model->setReslutionForConfiguration(0,ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
        {
            q_vec_type pos = Q_NULL_VECTOR;
            q_type orient = Q_ID_QUAT;
            project->addObject(model,pos,orient);
        }
            emit finished(true);
            currentRunner = SubprocessUtils::simplifyObjFile(simplified,5000);
            if (currentRunner == NULL)
                deleteLater();
            else
            {
                connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
                currentRunner->start();
                emit statusChanged("Decimating surface geometry to 5000 triangles.");
                qDebug() << "Decimating surface geometry to 5000 triangles.";
            }
            break;
        case 2:
            model->addSurfaceFileForResolution(0,ModelResolution::SIMPLIFIED_5000,
                                               simplified + ".decimated.5000.obj");
            currentRunner = SubprocessUtils::simplifyObjFile(simplified,2000);
            if (currentRunner == NULL)
                deleteLater();
            else
            {
                connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
                currentRunner->start();
                emit statusChanged("Decimating surface geometry to 2000 triangles.");
                qDebug() << "Decimating surface geometry to 2000 triangles.";
            }
            break;
        case 3:
            model->addSurfaceFileForResolution(0,ModelResolution::SIMPLIFIED_2000,
                                               simplified + ".decimated.2000.obj");
            currentRunner = SubprocessUtils::simplifyObjFile(simplified,1000);
            if (currentRunner == NULL)
                deleteLater();
            else
            {
                connect(currentRunner, SIGNAL(finished(bool)), this, SLOT(stepFinished(bool)));
                currentRunner->start();
                emit statusChanged("Decimating surface geometry to 1000 triangles.");
                qDebug() << "Decimating surface geometry to 1000 triangles.";
            }
            break;
        case 4:
            model->addSurfaceFileForResolution(0,ModelResolution::SIMPLIFIED_1000,
                                               simplified + ".decimated.1000.obj");
            deleteLater();
            break;
        }
        stepNum++;
    }
    else
    {
        if (stepNum <= 1)
            emit finished(false);
        deleteLater();
    }
}
