#include "modelfrompdbrunner.h"

#include <QDebug>

#include <sketchproject.h>
#include <sketchmodel.h>
#include <sketchioconstants.h>

#include "subprocessutils.h"

ModelFromPDBRunner::ModelFromPDBRunner(SketchProject *proj, const QString &pdb,
                                       const QString &toDelete, QObject *parent) :
    SubprocessRunner(parent),
    pdbId(pdb.toLower()),
    chainsToDelete(toDelete),
    project(proj),
    model(NULL),
    conformation(-1),
    currentRunner(NULL),
    stepNum(0)
{
}

ModelFromPDBRunner::~ModelFromPDBRunner()
{
}

void ModelFromPDBRunner::start()
{
    QString filename = (project->getProjectDir() + "/" + pdbId
                        + (chainsToDelete.isEmpty() ? "" : "-" + chainsToDelete)
                        + ".obj").trimmed();
    currentRunner = SubprocessUtils::makeChimeraOBJFor(
                pdbId,filename,0,chainsToDelete);
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
        QString filename = (project->getProjectDir() + "/" + pdbId
                              + (chainsToDelete.isEmpty() ? "" : "-" + chainsToDelete)
                              + ".obj").trimmed();
        QString simplified = (project->getProjectDir() + "/" + pdbId
                              + (chainsToDelete.isEmpty() ? QString("") : "-" + chainsToDelete)
                              + "_isosurface.obj").trimmed();
        switch (stepNum)
        {
        case 0:
            model = project->addModelFromFile(
                        ModelUtilities::createSourceNameFor(pdbId,chainsToDelete),
                        filename, DEFAULT_INVERSE_MASS, DEFAULT_INVERSE_MOMENT);
            for (int conf = 0; conf < model->getNumberOfConformations(); conf++)
            {
                if (model->getSource(conf) ==
                        ModelUtilities::createSourceNameFor(pdbId,chainsToDelete))
                {
                    conformation = conf;
                    break;
                }
            }
            if (! model->hasFileNameFor(conformation,ModelResolution::SIMPLIFIED_1000))
            {
                currentRunner = SubprocessUtils::makeChimeraOBJFor(pdbId,simplified,
                                                                   5,chainsToDelete);
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
            model->addSurfaceFileForResolution(conformation,
                                               ModelResolution::SIMPLIFIED_FULL_RESOLUTION,
                                               simplified);
            model->setReslutionForConformation(conformation,
                                                ModelResolution::SIMPLIFIED_FULL_RESOLUTION);
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
            model->addSurfaceFileForResolution(conformation,ModelResolution::SIMPLIFIED_5000,
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
            model->addSurfaceFileForResolution(conformation,ModelResolution::SIMPLIFIED_2000,
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
            model->addSurfaceFileForResolution(conformation,ModelResolution::SIMPLIFIED_1000,
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
