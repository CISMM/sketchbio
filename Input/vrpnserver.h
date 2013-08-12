#ifndef VRPNSERVER_H
#define VRPNSERVER_H

#include <QObject>

class QTimer;
class vrpn_Connection;
class vrpn_Tracker_RazerHydra;
class vrpn_Tracker_FilterOneEuro;

/*
 * This class is a Qt signals/slots driven vrpn server.  Ideally create a
 * new QThread and use moveToThread to make this run in that thread.  Then
 * use QTimer::singleShot to start/stop/restart/delete the server via its
 * slots.
 */
class vrpnServer : public QObject
{
    Q_OBJECT
public:
    vrpnServer();
    virtual ~vrpnServer();

public slots:
    // Initialize and start the server
    void startServer();

    // stop the server and remove the devices
    void stopServer();

    // restart the server
    void restartServer();

private slots:
    // do a timestep of the server mainloop
    void mainloopServer();
private:
    QTimer *timer;
    vrpn_Connection *connection;
    vrpn_Tracker_RazerHydra *hydra;
    vrpn_Tracker_FilterOneEuro *filter;
};


#endif // VRPNSERVER_H
