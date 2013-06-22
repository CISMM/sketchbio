#include "vrpnserver.h"

#include <vrpn_Connection.h>
#include <vrpn_Tracker_RazerHydra.h>

#include <QTimer>

#include <sketchioconstants.h>

vrpnServer::vrpnServer() :
    QObject(NULL),
    timer(new QTimer(this)),
    connection(NULL),
    hydra(NULL)
{
    timer->setInterval(1);
    QObject::connect(timer,SIGNAL(timeout()),this, SLOT(mainloopServer()));
}

vrpnServer::~vrpnServer()
{
    timer->stop();
    if (hydra != NULL)
        delete hydra;
    if (connection != NULL)
        delete connection;
}

void vrpnServer::startServer()
{
    if (timer->isActive())
        return;
    if (connection == NULL)
        connection = vrpn_create_server_connection();
    if (hydra == NULL)
        hydra = new vrpn_Tracker_RazerHydra( VRPN_RAZER_HYDRA_DEVICE_NAME, connection);
    timer->start();
}

void vrpnServer::stopServer()
{
    timer->stop();
    if (hydra != NULL)
    {
        delete hydra;
        hydra = NULL;
    }
    if (connection != NULL)
    {
        delete connection;
        connection = NULL;
    }
}

void vrpnServer::restartServer()
{
    stopServer();
    startServer();
}

void vrpnServer::mainloopServer()
{
    hydra->mainloop();
    connection->mainloop();
}

