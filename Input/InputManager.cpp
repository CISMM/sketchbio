#include "InputManager.h"

#include <cassert>
#include <limits>
#include <cstring>

#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>

#include <quat.h>

#include <QString>
#include <QSharedPointer>
#include <QProcess>

#include <sketchioconstants.h>
#include <transformmanager.h>
#include <sketchproject.h>
#include <hand.h>
#include <controlFunctions.h>

namespace SketchBio {

// ########################################################################
// ########################################################################
// Button handling code
// ########################################################################
// ########################################################################
// A class so that ButtonDeviceInfo can call something on InputManagerImpl
// without being a private class of InputManager
class ButtonHandler {
 public:
  virtual ~ButtonHandler() {}
  virtual void buttonStateChange(int buttonIndex, bool wasJustPressed) = 0;
};

// A class that holds the info about the device to pass the correct button
// number to the ButtonHandler
class ButtonDeviceInfo {
 public:
  ButtonDeviceInfo(ButtonHandler &b, const char *devName, int currentOffset,
                   int maxButtons)
      : buttonHandler(b),
        deviceName(devName),
        offset(currentOffset),
        max(maxButtons) {}
  int getOffset() { return offset; }
  int getMax() { return max; }
  void handleVrpnInput(int buttonId, bool wasJustPressed) {
    assert(buttonId >= 0 && buttonId < max);
    buttonHandler.buttonStateChange(buttonId + offset, wasJustPressed);
  }

 private:
  ButtonHandler &buttonHandler;
  QString deviceName;
  int offset;  // offset this device's buttons by this much
  int max;
};

// The VRPN callback to use for button presses.  The user data to go with it
// is a pointer to a ButtonDeviceInfo
static void VRPN_CALLBACK handleButtonPressWithDeviceInfo(
    void *userdata, const vrpn_BUTTONCB b) {
  ButtonDeviceInfo *devInfo = reinterpret_cast<ButtonDeviceInfo *>(userdata);
  devInfo->handleVrpnInput(b.button, b.state == 1);
}
// ########################################################################
// ########################################################################
// Analog handling code
// ########################################################################
// ########################################################################
// A class so that AnalogDeviceInfo can call something on InputManagerImpl
// without being a private class of InputManager
class AnalogHandler {
 public:
  virtual ~AnalogHandler() {}
  virtual void analogStateChange(int analogIndex, double newValue) = 0;
};

// A class that holds the info about the device to pass the correct analog
// number to the AnalogHandler
class AnalogDeviceInfo {
 public:
  AnalogDeviceInfo(AnalogHandler &a, const char *devName, int currentOffset,
                   int maxAnalogs)
      : analogHandler(a),
        deviceName(devName),
        offset(currentOffset),
        max(maxAnalogs) {}
  int getOffset() { return offset; }
  int getMax() { return max; }
  void handleVrpnInput(int analogId, double newValue) {
    assert(analogId >= 0 && analogId < max);
    analogHandler.analogStateChange(analogId + offset, newValue);
  }

 private:
  AnalogHandler &analogHandler;
  QString deviceName;
  int offset;  // offset this device's analogs by this much
  int max;
};

// The VRPN callback to use for analog presses.  The user data to go with it
// is a pointer to a AnalogDeviceInfo
static void VRPN_CALLBACK handleAnalogChangedWithDeviceInfo(
    void *userdata, const vrpn_ANALOGCB a) {
  AnalogDeviceInfo *devInfo = reinterpret_cast<AnalogDeviceInfo *>(userdata);
  for (int i = 0; i < a.num_channel; ++i) {
    devInfo->handleVrpnInput(i, a.channel[i]);
  }
}

// ########################################################################
// ########################################################################
// Tracker handling code
// ########################################################################
// ########################################################################
struct TrackerInfo {
  TrackerInfo(Project *&p, int tNum, SketchBioHandId::Type s)
      : proj(p), trackerNum(tNum), side(s) {}

  Project *&proj;
  int trackerNum;
  SketchBioHandId::Type side;
};

static void VRPN_CALLBACK handleTrackerData(void *userdata,
                                            const vrpn_TRACKERCB t) {
  TrackerInfo *info = reinterpret_cast<TrackerInfo *>(userdata);
  if (t.sensor == info->trackerNum) {
    q_xyz_quat_type data;
    q_vec_copy(data.xyz, t.pos);
    q_copy(data.quat, t.quat);
    info->proj->getTransformManager().setHandTransform(&data, info->side);
  }
}

// ########################################################################
// ########################################################################
// Classes to handle button and analog input and pass it to control functions
// ########################################################################
// ########################################################################

void doNothingButton(SketchBio::Project *, int, bool) {
  std::cout << "Did nothing button" << std::endl;
}
void doNothingAnalog(SketchBio::Project *, int, double) {
  std::cout << "Did nothing analog" << std::endl;
}

// Passes the button pressed to its control function
class ButtonControlFunction {
 public:
  ButtonControlFunction()
      : function(&doNothingButton), hand(SketchBioHandId::LEFT) {}
  void init(void (*f)(SketchBio::Project *, int, bool),
            SketchBioHandId::Type h) {
    function = f;
    hand = h;
  }
  void callFunction(SketchBio::Project *proj, bool wasPressed) {
    function(proj, hand, wasPressed);
  }

 private:
  // if this class is ever changed to have data that is not
  // faithfully copied by memcpy, then you will have to
  // change the Mode class's copy and assignment operators
  // to deal with that
  void (*function)(SketchBio::Project *, int, bool);
  SketchBioHandId::Type hand;
};

// Abstract class to handle analog control functions
class AnalogControlFunction {
 public:
  AnalogControlFunction(SketchBioHandId::Type h = SketchBioHandId::LEFT)
      : hand(h) {}
  virtual ~AnalogControlFunction() {}
  virtual void callFunction(SketchBio::Project *proj, double newValue) = 0;

 protected:
  SketchBioHandId::Type hand;
};

// Analog control function to handle threshold values
class AnalogThresholdControlFunction : public AnalogControlFunction {
 public:
  enum ThresholdType {
    GREATER_THAN,
    LESS_THAN
  };

  AnalogThresholdControlFunction(SketchBioHandId::Type h)
      : AnalogControlFunction(h),
        oldValue(std::numeric_limits<double>::quiet_NaN()),
        numThresholds(0) {}
  virtual ~AnalogThresholdControlFunction() {}
  // adds a threshold at the given value that will count as pressed
  // if the value of the analog is t (GREATER_THAN/LESS_THAN) the
  // threshold and calling the button control function f
  void addThreshold(double value, ThresholdType t,
                    void (*f)(SketchBio::Project *, int, bool)) {
    assert(numThresholds < 2);
    threshold[numThresholds] = value;
    type[numThresholds] = t;
    function[numThresholds] = f;
    ++numThresholds;
  }
  virtual void callFunction(SketchBio::Project *proj, double newValue) {
    for (int i = 0; i < numThresholds; ++i) {
      bool cond1 = (oldValue < threshold[i]);
      bool cond2 = (newValue < threshold[i]);
      // this conditional tests if we crossed the threshold from oldValue to
      // newValue
      if (cond1 != cond2) {
        // If the type is greater than and the old value was less than the
        // threshold
        // then it should be a pressed event since we are now greater than
        // If the type is less than and the new value is less than the threshold
        // then it should be a pressed event
        bool pressed = type[i] == GREATER_THAN ? cond1 : cond2;
        // call the function for that threshold (hand is a protected variable in
        // the
        // superclass
        function[i](proj, hand, pressed);
      }
    }
    oldValue = newValue;
  }

 private:
  double oldValue;
  double threshold[2];
  ThresholdType type[2];
  void (*function[2])(SketchBio::Project *, int, bool);
  int numThresholds;
};

// An analog control function handler to handle the case where the input
// should be normalized and passed through
class AnalogNormalizeControlFunction : public AnalogControlFunction {
 public:
  AnalogNormalizeControlFunction(double mmin, double mmax,
                                 SketchBioHandId::Type h,
                                 void (*f)(SketchBio::Project *, int, double) =
                                     &doNothingAnalog)
      : AnalogControlFunction(h), min(mmin), max(mmax), function(f) {}
  virtual ~AnalogNormalizeControlFunction() {}
  // normalize the value and call the function pointer
  virtual void callFunction(SketchBio::Project *proj, double newValue) {
    double vNorm = (newValue - min) / (max - min);
    function(proj, hand, vNorm);
  }

 private:
  double min;
  double max;
  void (*function)(SketchBio::Project *, int, double);
};

// ########################################################################
// ########################################################################
// Mode
// ########################################################################
// ########################################################################
struct Mode {
 public:
  Mode(int numBtns = 0, int numAnalogs = 0, const char *n = "",
       SketchBio::OutlineType::Type defaultO = SketchBio::OutlineType::OBJECTS)
      : buttonFunctions(new ButtonControlFunction[numBtns + 1]),
        numButtons(numBtns),
        analogFunctions(numAnalogs + 1),
        name(n),
        defaultOutline(defaultO) {
    for (int i = 0; i < numAnalogs; ++i) {
      analogFunctions.append(QSharedPointer<AnalogControlFunction>(
          new AnalogNormalizeControlFunction(0, 1, SketchBioHandId::LEFT)));
    }
  }
  Mode(const Mode &other)
      : buttonFunctions(new ButtonControlFunction[other.numButtons + 1]),
        numButtons(other.numButtons),
        analogFunctions(other.analogFunctions),
        name(other.name),
        defaultOutline(other.defaultOutline) {
    std::memcpy(buttonFunctions.data(), other.buttonFunctions.data(),
                numButtons * sizeof(ButtonControlFunction));
  }

  Mode &operator=(const Mode &other) {
    buttonFunctions.reset(new ButtonControlFunction[other.numButtons + 1]);
    numButtons = other.numButtons;
    std::memcpy(buttonFunctions.data(), other.buttonFunctions.data(),
                numButtons * sizeof(ButtonControlFunction));
    analogFunctions = other.analogFunctions;
    name = other.name;
    defaultOutline = other.defaultOutline;
  }

 public:
  QScopedPointer<ButtonControlFunction,
                 QScopedPointerArrayDeleter<ButtonControlFunction> >
      buttonFunctions;
  int numButtons;
  QVector<QSharedPointer<AnalogControlFunction> > analogFunctions;
  QString name;
  SketchBio::OutlineType::Type defaultOutline;
};

// ########################################################################
// ########################################################################
// Input Manager Implementation class
// ########################################################################
// ########################################################################
// This class is the internal implementation of the InputManager
class InputManager::InputManagerImpl : public ButtonHandler,
                                       public AnalogHandler {
 public:
  InputManagerImpl(const QString &inputConfigFileName);
  ~InputManagerImpl();

  void handleCurrentInput();

  const QString &getModeName();
  void setProject(SketchBio::Project *proj);

  virtual void buttonStateChange(int buttonIndex, bool wasJustPressed);
  virtual void analogStateChange(int analogIndex, double newValue);

 private:
  void parseXML(const QString &inputConfigFileName);

 private:
  typedef QPair<QSharedPointer<vrpn_Button_Remote>,
                QSharedPointer<ButtonDeviceInfo> > ButtonPair;
  typedef QPair<QSharedPointer<vrpn_Analog_Remote>,
                QSharedPointer<AnalogDeviceInfo> > AnalogPair;
  QProcess vrpn_server;
  QVector<ButtonPair> buttonDevices;
  QVector<AnalogPair> analogDevices;
  QVector<QSharedPointer<vrpn_Tracker_Remote> > trackerDevices;
  QVector<QSharedPointer<TrackerInfo> > trackerInfos;

  QVector<Mode> modes;
  int currentMode;
  int modeSwitchButtonNum;

  SketchBio::Project *project;
};

InputManager::InputManagerImpl::InputManagerImpl(
    const QString &inputConfigFileName) {
  parseXML(inputConfigFileName);
}

InputManager::InputManagerImpl::~InputManagerImpl() {}

void InputManager::InputManagerImpl::handleCurrentInput() {
  foreach(const ButtonPair & b, buttonDevices) { b.first->mainloop(); }
  foreach(const AnalogPair & a, analogDevices) { a.first->mainloop(); }
  foreach(const QSharedPointer<vrpn_Tracker_Remote> & ptr, trackerDevices) {
    ptr->mainloop();
  }
}

const QString &InputManager::InputManagerImpl::getModeName() {
  return modes[currentMode].name;
}

void InputManager::InputManagerImpl::setProject(Project *proj) {
  project = proj;
}

void InputManager::InputManagerImpl::buttonStateChange(int buttonIndex,
                                                       bool wasJustPressed) {
  assert(buttonIndex >= 0 && buttonIndex < modes[currentMode].numButtons);
  if (buttonIndex == modeSwitchButtonNum) {
    if (!wasJustPressed) {
      currentMode = (currentMode + 1) % modes.size();
      project->setOperationState(NULL);
      project->clearDirections();
      SketchBio::Hand *h;
      h = &project->getHand(SketchBioHandId::LEFT);
      h->clearState();
      h->setSelectionType(modes[currentMode].defaultOutline);
      h = &project->getHand(SketchBioHandId::RIGHT);
      h->clearState();
      h->setSelectionType(modes[currentMode].defaultOutline);
    }
  } else {
    modes[currentMode].buttonFunctions.data()[buttonIndex]
        .callFunction(project, wasJustPressed);
  }
}

void InputManager::InputManagerImpl::analogStateChange(int analogIdx,
                                                       double newValue) {
  assert(analogIdx >= 0 &&
         analogIdx < modes[currentMode].analogFunctions.size());
  modes[currentMode].analogFunctions[analogIdx]
      ->callFunction(project, newValue);
}

void InputManager::InputManagerImpl::parseXML(
    const QString &inputConfigFileName) {
  // make vrpn server
  // initialize devices
  buttonDevices.append(
      ButtonPair(QSharedPointer<vrpn_Button_Remote>(
                     new vrpn_Button_Remote("razer@localhost")),
                 QSharedPointer<ButtonDeviceInfo>(
                     new ButtonDeviceInfo(*this, "razer", 0, 16))));
  buttonDevices[0].first->register_change_handler(
      reinterpret_cast<void *>(buttonDevices[0].second.data()),
      &handleButtonPressWithDeviceInfo);
  analogDevices.append(
      AnalogPair(QSharedPointer<vrpn_Analog_Remote>(
                     new vrpn_Analog_Remote("razer@localhost")),
                 QSharedPointer<AnalogDeviceInfo>(
                     new AnalogDeviceInfo(*this, "razer", 0, 6))));
  analogDevices[0].first->register_change_handler(
      reinterpret_cast<void *>(analogDevices[0].second.data()),
      &handleAnalogChangedWithDeviceInfo);
  trackerDevices.append(QSharedPointer<vrpn_Tracker_Remote>(
      new vrpn_Tracker_Remote("filteredRazer@localhost")));
  trackerInfos.append(QSharedPointer<TrackerInfo>(
      new TrackerInfo(project, 0, SketchBioHandId::LEFT)));
  trackerInfos.append(QSharedPointer<TrackerInfo>(
      new TrackerInfo(project, 1, SketchBioHandId::RIGHT)));
  trackerDevices[0]->register_change_handler(
      reinterpret_cast<void *>(trackerInfos[0].data()), &handleTrackerData);
  trackerDevices[0]->register_change_handler(
      reinterpret_cast<void *>(trackerInfos[1].data()), &handleTrackerData);
  int maxButtons = buttonDevices.last().second->getOffset() +
                   buttonDevices.last().second->getMax();
  int maxAnalogs = analogDevices.last().second->getOffset() +
                   analogDevices.last().second->getMax();
  // initialize mapping for modes
  modes.append(Mode(maxButtons, maxAnalogs, "Edit Objects",
                    SketchBio::OutlineType::OBJECTS));
  Mode &editObjects = modes.last();
  editObjects.buttonFunctions.data()[6]
      .init(&ControlFunctions::grabObjectOrWorld, SketchBioHandId::LEFT);
  editObjects.buttonFunctions.data()[14]
      .init(&ControlFunctions::grabSpringOrWorld, SketchBioHandId::RIGHT);
  modes.append(Mode(maxButtons, maxAnalogs, "Edit Springs",
                    SketchBio::OutlineType::CONNECTORS));
  Mode &editSprings = modes.last();
  editSprings.buttonFunctions.data()[6]
      .init(&ControlFunctions::grabObjectOrWorld, SketchBioHandId::LEFT);
  editSprings.buttonFunctions.data()[14]
      .init(&ControlFunctions::grabSpringOrWorld, SketchBioHandId::RIGHT);

  currentMode = 0;
  modeSwitchButtonNum = 0;
}

// ########################################################################
// ########################################################################
// Input Manager class
// ########################################################################
// ########################################################################
InputManager::InputManager(const QString &inputConfigFileName, QObject *parent)
    : QObject(parent), impl(new InputManagerImpl(inputConfigFileName)) {}

InputManager::~InputManager() { delete impl; }

void InputManager::handleCurrentInput() { impl->handleCurrentInput(); }

const QString &InputManager::getModeName() { return impl->getModeName(); }

void InputManager::setProject(SketchBio::Project *proj) {
  impl->setProject(proj);
}

}
