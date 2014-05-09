#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
namespace SketchBio {
class Project;

/*
 * This class handles input from arbitrary VRPN devices and loads how the input
 * is interpreted from a XML configuration file.
 */
class InputManager : public QObject
{
    Q_OBJECT
public:
    // Create a new InputManager from the given XML configuration file
    explicit InputManager(const QString &inputConfigFileName, QObject *parent = 0);
    // Destroy the input manager
    virtual ~InputManager();

    // polls vrpn for input and handles it
    void handleCurrentInput();

    // Get the name of the current mode (loaded from XML configuration)
    const QString &getModeName();
    // Sets the project that this input manager's input will affect
    void setProject(SketchBio::Project *proj);

public slots:
	// Resets the vrpn server
	void resetDevices();

signals:
    // emitted whenever the mode changes
    void modeChanged();

private slots:
    void notifyModeChanged();

private:
    // Private Implementation pattern
    class InputManagerImpl;
    InputManagerImpl *impl;
};

}

#endif // INPUTMANAGER_H
