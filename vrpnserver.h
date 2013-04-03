#ifndef VRPNSERVER_H
#define VRPNSERVER_H

#include <QThread>

/*
 * This is pretty much a copy of Cory's code from VPAW with the
 * phantom device code removed
 */
class vrpnServer : public QThread
{
    Q_OBJECT
public:
    vrpnServer();
    virtual ~vrpnServer();

    // Start the thread
    void Start();

    // Stop the thread
    void Stop();

protected slots:
    void run();
private:
    vrpnServer(const vrpnServer& other); // not implemented
    void operator =(const vrpnServer& other); // not implemented

    bool Stopped;
};

#endif // VRPNSERVER_H
