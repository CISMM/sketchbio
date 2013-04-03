#include "vrpnServer.h"
#include <iostream>
#include <vrpn_Connection.h>
#include <vrpn_Tracker_RazerHydra.h>
#include <sketchioconstants.h>

/*
 * This is pretty much Cory's code from VPAW with the Phantom stuff removed
 */

//----------------------------------------------------------------------------
vrpnServer::vrpnServer()
{
  this->Stopped = true;
}

//----------------------------------------------------------------------------
vrpnServer::~vrpnServer()
{
}

//----------------------------------------------------------------------------
void vrpnServer::Start()
{
  this->Stopped = false;
  this->start();
}

//----------------------------------------------------------------------------
void vrpnServer::Stop()
{
  this->Stopped = true;
  QThread::wait();
}

//----------------------------------------------------------------------------
void vrpnServer::run()
{
  vrpn_Connection * connection = vrpn_create_server_connection();
  // Create the various device objects
  vrpn_Tracker_RazerHydra *razer =
    new vrpn_Tracker_RazerHydra( VRPN_RAZER_HYDRA_DEVICE_NAME, connection );

  while ( !this->Stopped ) {
    razer->mainloop();
    connection->mainloop();

    QThread::msleep( 1 );
    }

  delete razer;
  delete connection;
}
