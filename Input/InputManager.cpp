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

#include <vtkSmartPointer.h>
#include <vtkXMLUtilities.h>
#include <vtkXMLDataElement.h>

#include <sketchioconstants.h>
#include <transformmanager.h>
#include <sketchproject.h>
#include <hand.h>

#include <controlFunctions.h>
#include <signalemitter.h>

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
  //  std::cout << "Did nothing analog" << std::endl;
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
        analogFunctions(),
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
  bool readDevices(vtkXMLDataElement *root);
  bool readInputTransform(vtkXMLDataElement *root);
  bool readModeSwitchButton(vtkXMLDataElement *root);
  bool readModes(vtkXMLDataElement *root);

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

 public:
  SignalEmitter emitter;
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
  project->updateTrackerPositions();
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
      emitter.emitSignal();
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

static const char CONFIG_XML_ROOT_NAME[] = "SketchBioDeviceConfig";
static const char CONFIG_XML_VERSION_ATTRIBUTE[] = "fileVersion";
static const int CONFIG_FILE_READER_MAJOR_VERSION = 0;
static const int CONFIG_FILE_READER_MINOR_VERSION = 0;

static inline void convertToCurrentConfigFile(vtkXMLDataElement *xmlRoot,
                                              int minorVersion) {}

void InputManager::InputManagerImpl::parseXML(
    const QString &inputConfigFileName) {
  vtkSmartPointer<vtkXMLDataElement> root =
      vtkSmartPointer<vtkXMLDataElement>::Take(
          vtkXMLUtilities::ReadElementFromFile(
              inputConfigFileName.toStdString().c_str()));
  if (root->GetName() != QString(CONFIG_XML_ROOT_NAME)) {
    std::cout << "Error reading xml, wrong root element" << std::endl;
    return;
  }
  if (root->GetAttribute(CONFIG_XML_VERSION_ATTRIBUTE) == NULL) {
    std::cout << "Config xml has no file version" << std::endl;
    return;
  }
  QStringList version =
      QString(root->GetAttribute(CONFIG_XML_VERSION_ATTRIBUTE)).split(".");
  bool ok1, ok2;
  int majorVersion = version[0].toInt(&ok1);
  int minorVersion = version[1].toInt(&ok2);
  if (!ok1 || !ok2) {
    std::cout << "Bad file version number" << std::endl;
    return;
  }
  if (majorVersion != CONFIG_FILE_READER_MAJOR_VERSION) {
    std::cout << "Major file version is different from reader." << std::endl;
    return;
  }
  if (minorVersion > CONFIG_FILE_READER_MINOR_VERSION) {
    std::cout << "Config file newer than config file reader." << std::endl;
    return;
  }
  if (minorVersion < CONFIG_FILE_READER_MINOR_VERSION) {
    convertToCurrentConfigFile(root, minorVersion);
  }
  if (!readDevices(root)) return;
  if (!readInputTransform(root)) return;
  if (!readModeSwitchButton(root)) return;
  if (!readModes(root)) return;
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
  editObjects.buttonFunctions.data()[5]
      .init(&ControlFunctions::grabObjectOrWorld, SketchBioHandId::LEFT);
  editObjects.buttonFunctions.data()[13]
      .init(&ControlFunctions::grabObjectOrWorld, SketchBioHandId::RIGHT);
  modes.append(Mode(maxButtons, maxAnalogs, "Edit Springs",
                    SketchBio::OutlineType::CONNECTORS));
  Mode &editSprings = modes.last();
  editSprings.buttonFunctions.data()[5]
      .init(&ControlFunctions::grabSpringOrWorld, SketchBioHandId::LEFT);
  editSprings.buttonFunctions.data()[13]
      .init(&ControlFunctions::grabSpringOrWorld, SketchBioHandId::RIGHT);

  currentMode = 0;
  modeSwitchButtonNum = 1;
}

static const char CONFIG_FILE_DEVICE_ELEMENT_NAME[] = "device";
static const char CONFIG_FILE_DEVICE_VRPN_NAME_ATTRIBUTE[] = "name";
static const char CONFIG_FILE_BUTTON_DEVICE_ELEMENT_NAME[] = "buttons";
static const char CONFIG_FILE_ANALOG_DEVICE_ELEMENT_NAME[] = "analogs";
static const char CONFIG_FILE_TRACKER_DEVICE_ELEMENT_NAME[] = "tracker";
static const char CONFIG_FILE_DEVICE_NUM_INPUTS_ATTRIBUTE[] = "num";
static const char CONFIG_FILE_DEVICE_TRACKER_CHANNEL_ATTRIBUTE[] = "channel";
static const char CONFIG_FILE_HAND_ATTRIBUTE[] = "hand";
static const char CONFIG_FILE_HAND_LEFT[] = "left";
static const char CONFIG_FILE_HAND_RIGHT[] = "right";

bool InputManager::InputManagerImpl::readDevices(vtkXMLDataElement *root) {
  int buttonOffset = 0;
  int analogOffset = 0;
  int numSubElements = root->GetNumberOfNestedElements();
  for (int i = 0; i < numSubElements; ++i) {
    vtkXMLDataElement *deviceElt = root->GetNestedElement(i);
    if (deviceElt->GetName() == QString(CONFIG_FILE_DEVICE_ELEMENT_NAME)) {
      if (deviceElt->GetAttribute(CONFIG_FILE_DEVICE_VRPN_NAME_ATTRIBUTE) ==
          NULL) {
        std::cout << "Device does not have a vrpn name" << std::endl;
        return false;
      }
      bool hasTrackerRemote = false;
      vrpn_Tracker_Remote *trackerRemote = NULL;
      const char *devName =
          deviceElt->GetAttribute(CONFIG_FILE_DEVICE_VRPN_NAME_ATTRIBUTE);
      std::string vrpn_full_devName = devName;
      vrpn_full_devName += "@localhost";
      int numDeviceSubElements = deviceElt->GetNumberOfNestedElements();
      for (int j = 0; j < numDeviceSubElements; ++j) {
        vtkXMLDataElement *subElt = deviceElt->GetNestedElement(j);
        if (subElt->GetName() ==
            QString(CONFIG_FILE_BUTTON_DEVICE_ELEMENT_NAME)) {
          if (subElt->GetAttribute(CONFIG_FILE_DEVICE_NUM_INPUTS_ATTRIBUTE) ==
              NULL) {
            std::cout << "No attribute for number of buttons" << std::endl;
            return false;
          }
          int numInputs;
          if (!subElt->GetScalarAttribute(
                  CONFIG_FILE_DEVICE_NUM_INPUTS_ATTRIBUTE, numInputs)) {
            std::cout << "Number of buttons is not an integer" << std::endl;
            return false;
          }
          QSharedPointer<vrpn_Button_Remote> buttonRemote(
              new vrpn_Button_Remote(vrpn_full_devName.c_str()));
          QSharedPointer<ButtonDeviceInfo> buttonInfo(
              new ButtonDeviceInfo(*this, devName, buttonOffset, numInputs));
          buttonRemote->register_change_handler(
              reinterpret_cast<void *>(buttonInfo.data()),
              &handleButtonPressWithDeviceInfo);
          this->buttonDevices.append(ButtonPair(buttonRemote, buttonInfo));
          buttonOffset += numInputs;
        } else if (subElt->GetName() ==
                   QString(CONFIG_FILE_ANALOG_DEVICE_ELEMENT_NAME)) {
          if (subElt->GetAttribute(CONFIG_FILE_DEVICE_NUM_INPUTS_ATTRIBUTE) ==
              NULL) {
            std::cout << "No attribute for number of analogs" << std::endl;
            return false;
          }
          int numInputs;
          if (!subElt->GetScalarAttribute(
                  CONFIG_FILE_DEVICE_NUM_INPUTS_ATTRIBUTE, numInputs)) {
            std::cout << "Number of analogs is not an integer" << std::endl;
            return false;
          }
          QSharedPointer<vrpn_Analog_Remote> analogRemote(
              new vrpn_Analog_Remote(vrpn_full_devName.c_str()));
          QSharedPointer<AnalogDeviceInfo> analogInfo(
              new AnalogDeviceInfo(*this, devName, analogOffset, numInputs));
          analogRemote->register_change_handler(
              reinterpret_cast<void *>(analogInfo.data()),
              &handleAnalogChangedWithDeviceInfo);
          this->analogDevices.append(AnalogPair(analogRemote, analogInfo));
          analogOffset += numInputs;
        } else if (subElt->GetName() ==
                   QString(CONFIG_FILE_TRACKER_DEVICE_ELEMENT_NAME)) {
          if (subElt->GetAttribute(
                  CONFIG_FILE_DEVICE_TRACKER_CHANNEL_ATTRIBUTE) == NULL) {
            std::cout << "No attribute for tracker channel" << std::endl;
            return false;
          }
          const char *hand = subElt->GetAttribute(CONFIG_FILE_HAND_ATTRIBUTE);
          if (hand == NULL) {
            std::cout << "No attribute for tracker hand" << std::endl;
            return false;
          }
          int channel;
          if (!subElt->GetScalarAttribute(
                  CONFIG_FILE_DEVICE_TRACKER_CHANNEL_ATTRIBUTE, channel)) {
            std::cout << "Tracker channel attribute is not an integer"
                      << std::endl;
            return false;
          }
          SketchBioHandId::Type side;
          if (hand == QString(CONFIG_FILE_HAND_LEFT)) {
            side = SketchBioHandId::LEFT;
          } else if (hand == QString(CONFIG_FILE_HAND_RIGHT)) {
            side = SketchBioHandId::RIGHT;
          } else {
            std::cout << "Which hand to tie tracker to is spelled wrong"
                      << std::endl;
            return false;
          }
          if (!hasTrackerRemote) {
            QSharedPointer<vrpn_Tracker_Remote> tracker(
                new vrpn_Tracker_Remote(vrpn_full_devName.c_str()));
            trackerRemote = tracker.data();
            this->trackerDevices.append(tracker);
          }
          assert(trackerRemote != NULL);
          QSharedPointer<TrackerInfo> info(
              new TrackerInfo(project, channel, side));
          trackerRemote->register_change_handler(
              reinterpret_cast<void *>(info.data()), &handleTrackerData);
          this->trackerInfos.append(info);
        } else {
          std::cout << "Warning: Unknown element in device tag" << std::endl;
        }
      }
    }
  }
  return true;
}

bool InputManager::InputManagerImpl::readInputTransform(
    vtkXMLDataElement *root) {
  return true;
}

bool InputManager::InputManagerImpl::readModeSwitchButton(
    vtkXMLDataElement *root) {
  return true;
}

bool InputManager::InputManagerImpl::readModes(vtkXMLDataElement *root) {
  return true;
}

// ########################################################################
// ########################################################################
// Input Manager class
// ########################################################################
// ########################################################################
InputManager::InputManager(const QString &inputConfigFileName, QObject *parent)
    : QObject(parent), impl(new InputManagerImpl(inputConfigFileName)) {
  connect(&impl->emitter, SIGNAL(somethingHappened()), this,
          SLOT(notifyModeChanged()));
}

InputManager::~InputManager() { delete impl; }

void InputManager::handleCurrentInput() { impl->handleCurrentInput(); }

const QString &InputManager::getModeName() { return impl->getModeName(); }

void InputManager::setProject(SketchBio::Project *proj) {
  impl->setProject(proj);
}

void InputManager::notifyModeChanged() { emit modeChanged(); }

}
