#include "vrpnserver.h"

#include <vrpn_Connection.h>
#include <vrpn_Tracker_RazerHydra.h>
#include <vrpn_Tracker_Filter.h>

#include <QTimer>

#include <sketchioconstants.h>

vrpnServer::vrpnServer() :
    QObject(NULL),
    timer(new QTimer(this)),
    connection(NULL),
    hydra(NULL),
    filter(NULL)
{
    timer->setInterval(1);
    QObject::connect(timer,SIGNAL(timeout()),this, SLOT(mainloopServer()));
}

vrpnServer::~vrpnServer()
{
    timer->stop();
    if (filter != NULL)
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
    if (filter == NULL)
        filter = new vrpn_Tracker_FilterOneEuro(
                    VRPN_ONE_EURO_FILTER_DEVICE_NAME,
                    connection,"*"VRPN_RAZER_HYDRA_DEVICE_NAME,
                    2, 1.15,0.5,1.2,1.5,0.5,1.2);
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
    if (filter != NULL)
    {
        delete filter;
        filter = NULL;
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
    filter->mainloop();
    connection->mainloop();
}

