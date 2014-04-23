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
  int getOffset() const { return offset; }
  int getMax() const { return max; }
  const QString &getDeviceName() const { return deviceName; }
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
  const QString &getDeviceName() const { return deviceName; }
  int getOffset() const { return offset; }
  int getMax() const { return max; }
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
  void init(ControlFunctions::ButtonControlFunctionPtr f,
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
  ControlFunctions::ButtonControlFunctionPtr function;
  SketchBioHandId::Type hand;
};

// Abstract class to handle analog control functions
class AnalogControlFunction {
 public:
  AnalogControlFunction() {}
  virtual ~AnalogControlFunction() {}
  virtual void callFunction(SketchBio::Project *proj, double newValue) = 0;
};

// Analog control function to handle threshold values
class AnalogThresholdControlFunction : public AnalogControlFunction {
 public:
  enum ThresholdType {
    GREATER_THAN,
    LESS_THAN
  };

  AnalogThresholdControlFunction()
      : AnalogControlFunction(),
        oldValue(std::numeric_limits<double>::quiet_NaN()),
        numThresholds(0) {}
  virtual ~AnalogThresholdControlFunction() {}
  // adds a threshold at the given value that will count as pressed
  // if the value of the analog is t (GREATER_THAN/LESS_THAN) the
  // threshold and calling the button control function f
  void addThreshold(double value, ThresholdType t, SketchBioHandId::Type h,
                    ControlFunctions::ButtonControlFunctionPtr f) {
    assert(numThresholds < 2);
    threshold[numThresholds] = value;
    type[numThresholds] = t;
    function[numThresholds] = f;
    hand[numThresholds] = h;
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
        function[i](proj, hand[i], pressed);
      }
    }
    oldValue = newValue;
  }

 private:
  double oldValue;
  double threshold[2];
  ThresholdType type[2];
  SketchBioHandId::Type hand[2];
  ControlFunctions::ButtonControlFunctionPtr function[2];
  int numThresholds;
};

// An analog control function handler to handle the case where the input
// should be normalized and passed through
class AnalogNormalizeControlFunction : public AnalogControlFunction {
 public:
  AnalogNormalizeControlFunction(double mmin, double mmax,
                                 SketchBioHandId::Type h,
                                 ControlFunctions::AnalogControlFunctionPtr f =
                                     &doNothingAnalog)
      : AnalogControlFunction(), min(mmin), max(mmax), function(f), hand(h) {}
  virtual ~AnalogNormalizeControlFunction() {}
  // normalize the value and call the function pointer
  virtual void callFunction(SketchBio::Project *proj, double newValue) {
    double vNorm = (newValue - min) / (max - min);
    function(proj, hand, vNorm);
  }

 private:
  double min;
  double max;
  ControlFunctions::AnalogControlFunctionPtr function;
  SketchBioHandId::Type hand;
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
    const QString &inputConfigFileName)
    : currentMode(0), modeSwitchButtonNum(0) {
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
  assert(currentMode >= 0 && currentMode < modes.size());
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
  //  buttonDevices.append(
  //      ButtonPair(QSharedPointer<vrpn_Button_Remote>(
  //                     new vrpn_Button_Remote("razer@localhost")),
  //                 QSharedPointer<ButtonDeviceInfo>(
  //                     new ButtonDeviceInfo(*this, "razer", 0, 16))));
  //  buttonDevices[0].first->register_change_handler(
  //      reinterpret_cast<void *>(buttonDevices[0].second.data()),
  //      &handleButtonPressWithDeviceInfo);
  //  analogDevices.append(
  //      AnalogPair(QSharedPointer<vrpn_Analog_Remote>(
  //                     new vrpn_Analog_Remote("razer@localhost")),
  //                 QSharedPointer<AnalogDeviceInfo>(
  //                     new AnalogDeviceInfo(*this, "razer", 0, 6))));
  //  analogDevices[0].first->register_change_handler(
  //      reinterpret_cast<void *>(analogDevices[0].second.data()),
  //      &handleAnalogChangedWithDeviceInfo);
  //  trackerDevices.append(QSharedPointer<vrpn_Tracker_Remote>(
  //      new vrpn_Tracker_Remote("filteredRazer@localhost")));
  //  trackerInfos.append(QSharedPointer<TrackerInfo>(
  //      new TrackerInfo(project, 0, SketchBioHandId::LEFT)));
  //  trackerInfos.append(QSharedPointer<TrackerInfo>(
  //      new TrackerInfo(project, 1, SketchBioHandId::RIGHT)));
  //  trackerDevices[0]->register_change_handler(
  //      reinterpret_cast<void *>(trackerInfos[0].data()), &handleTrackerData);
  //  trackerDevices[0]->register_change_handler(
  //      reinterpret_cast<void *>(trackerInfos[1].data()), &handleTrackerData);
  //  int maxButtons = buttonDevices.last().second->getOffset() +
  //                   buttonDevices.last().second->getMax();
  //  int maxAnalogs = analogDevices.last().second->getOffset() +
  //                   analogDevices.last().second->getMax();
  // initialize mapping for modes
  //  modes.append(Mode(maxButtons, maxAnalogs, "Edit Objects",
  //                    SketchBio::OutlineType::OBJECTS));
  //  Mode &editObjects = modes.last();
  //  editObjects.buttonFunctions.data()[5]
  //      .init(&ControlFunctions::grabObjectOrWorld, SketchBioHandId::LEFT);
  //  editObjects.buttonFunctions.data()[13]
  //      .init(&ControlFunctions::grabObjectOrWorld, SketchBioHandId::RIGHT);
  //  modes.append(Mode(maxButtons, maxAnalogs, "Edit Springs",
  //                    SketchBio::OutlineType::CONNECTORS));
  //  Mode &editSprings = modes.last();
  //  editSprings.buttonFunctions.data()[5]
  //      .init(&ControlFunctions::grabSpringOrWorld, SketchBioHandId::LEFT);
  //  editSprings.buttonFunctions.data()[13]
  //      .init(&ControlFunctions::grabSpringOrWorld, SketchBioHandId::RIGHT);

  //  currentMode = 0;
  //  modeSwitchButtonNum = 1;
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

static const bool getHand(vtkXMLDataElement *elt,
                          SketchBioHandId::Type &output) {
  const char *hand = elt->GetAttribute(CONFIG_FILE_HAND_ATTRIBUTE);
  if (hand == NULL) {
    std::cout << "No attribute for hand" << std::endl;
    return false;
  }
  if (hand == QString(CONFIG_FILE_HAND_LEFT)) {
    output = SketchBioHandId::LEFT;
  } else if (hand == QString(CONFIG_FILE_HAND_RIGHT)) {
    output = SketchBioHandId::RIGHT;
  } else {
    std::cout << "Unknown hand type, check your spelling" << std::endl;
    return false;
  }
  return true;

}

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
          //          std::cout << "Adding button device" << std::endl;
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
          //          std::cout << "Adding analog device" << std::endl;
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
          int channel;
          if (!subElt->GetScalarAttribute(
                  CONFIG_FILE_DEVICE_TRACKER_CHANNEL_ATTRIBUTE, channel)) {
            std::cout << "Tracker channel attribute is not an integer"
                      << std::endl;
            return false;
          }
          SketchBioHandId::Type side;
          if (!getHand(subElt, side)) {
            return false;
          }
          if (!hasTrackerRemote) {
            QSharedPointer<vrpn_Tracker_Remote> tracker(
                new vrpn_Tracker_Remote(vrpn_full_devName.c_str()));
            trackerRemote = tracker.data();
            //            std::cout << "Adding tracker device" << std::endl;
            this->trackerDevices.append(tracker);
          }
          assert(trackerRemote != NULL);
          QSharedPointer<TrackerInfo> info(
              new TrackerInfo(project, channel, side));
          trackerRemote->register_change_handler(
              reinterpret_cast<void *>(info.data()), &handleTrackerData);
          //          std::cout << "Adding tracker hand" << std::endl;
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

static const char CONFIG_FILE_MODE_SWITCH_BUTTON_ELEMENT[] = "modeSwitchButton";
static const char CONFIG_FILE_DEVICE_NAME_ATTRIBUTE[] = "deviceName";
static const char CONFIG_FILE_BUTTON_NUMBER_ATTRIBUTE[] = "buttonNumber";
static const char CONFIG_FILE_ANALOG_NUMBER_ATTRIBUTE[] = "analogNumber";

static inline bool getButtonNumber(
    vtkXMLDataElement *elt,
    const QVector<QPair<QSharedPointer<vrpn_Button_Remote>,
                        QSharedPointer<ButtonDeviceInfo> > > &buttonDevices,
    int &output) {
  const char *devName = elt->GetAttribute(CONFIG_FILE_DEVICE_NAME_ATTRIBUTE);
  if (devName == NULL) {
    std::cout << "Button device could not be determined" << std::endl;
    return false;
  }
  int buttonNum;
  if (!elt->GetScalarAttribute(CONFIG_FILE_BUTTON_NUMBER_ATTRIBUTE,
                               buttonNum)) {
    std::cout << "Could not find button number" << std::endl;
    return false;
  }
  int max = -1;
  int offset = -1;
  for (int i = 0; i < buttonDevices.size(); ++i) {
    if (buttonDevices[i].second->getDeviceName() == devName) {
      offset = buttonDevices[i].second->getOffset();
      max = buttonDevices[i].second->getMax();
    }
  }
  if (offset == -1) {
    std::cout << "Could not find device: Device " << devName << " unknown"
              << std::endl;
    return false;
  }
  if (buttonNum >= max) {
    std::cout << "Specified button is out of range for device" << std::endl;
    return false;
  }
  output = offset + buttonNum;
  return true;
}

static inline bool getAnalogNumber(
    vtkXMLDataElement *elt,
    const QVector<QPair<QSharedPointer<vrpn_Analog_Remote>,
                        QSharedPointer<AnalogDeviceInfo> > > &analogDevices,
    int &output) {
  const char *devName = elt->GetAttribute(CONFIG_FILE_DEVICE_NAME_ATTRIBUTE);
  if (devName == NULL) {
    std::cout << "Analog device could not be determined" << std::endl;
    return false;
  }
  int analogNum;
  if (!elt->GetScalarAttribute(CONFIG_FILE_ANALOG_NUMBER_ATTRIBUTE,
                               analogNum)) {
    std::cout << "Could not find analog number" << std::endl;
    return false;
  }
  int max = -1;
  int offset = -1;
  for (int i = 0; i < analogDevices.size(); ++i) {
    if (analogDevices[i].second->getDeviceName() == devName) {
      offset = analogDevices[i].second->getOffset();
      max = analogDevices[i].second->getMax();
    }
  }
  if (offset == -1) {
    std::cout << "Could not find device: Device " << devName << " unknown"
              << std::endl;
    return false;
  }
  if (analogNum >= max) {
    std::cout << "Specified analog is out of range for device " << devName
              << std::endl;
    return false;
  }
  output = offset + analogNum;
  return true;
}

bool InputManager::InputManagerImpl::readModeSwitchButton(
    vtkXMLDataElement *root) {
  vtkXMLDataElement *modeSwitchButton =
      root->FindNestedElementWithName(CONFIG_FILE_MODE_SWITCH_BUTTON_ELEMENT);
  if (modeSwitchButton == NULL) {
    std::cout << "Could not find mode switch button" << std::endl;
    return false;
  }
  if (!getButtonNumber(modeSwitchButton, buttonDevices, modeSwitchButtonNum)) {
    return false;
  }
  //  std::cout << "Mode switch button set to " << modeSwitchButtonNum <<
  // std::endl;
  return true;
}

static const char CONFIG_FILE_MODE_ELEMENT_NAME[] = "mode";
static const char CONFIG_FILE_MODE_NAME_ATTRIBUTE[] = "name";
static const char CONFIG_FILE_MODE_DEFAULT_SELECTION_ATTRIBUTE[] =
    "defaultSelection";
static const char CONFIG_FILE_SELECTION_TYPE_OBJECTS[] = "Objects";
static const char CONFIG_FILE_SELECTION_TYPE_CONNECTOR[] = "Connectors";

static const char CONFIG_FILE_BUTTON_CTRL_ELEMENT_NAME[] = "button";
static const char CONFIG_FILE_ANALOG_CTRL_ELEMENT_NAME[] = "analog";
static const char CONFIG_FILE_FUNCTION_ATTRIBUTE[] = "function";

static const char CONFIG_FILE_ANALOG_TYPE_ATTRIBUTE[] = "type";
static const char CONFIG_FILE_ANALOG_TYPE_NORMALIZE[] = "normalize";
static const char CONFIG_FILE_ANALOG_TYPE_THRESHOLD[] = "threshold";

static const char CONFIG_FILE_ANALOG_INTERVAL_ELEMENT[] = "interval";
static const char CONFIG_FILE_ANALOG_INTERVAL_MIN_ATTRIBUTE[] = "min";
static const char CONFIG_FILE_ANALOG_INTERVAL_MAX_ATTRIBUTE[] = "max";

static const char CONFIG_FILE_ANALOG_THRESHOLD_ELEMENT[] = "threshold";
static const char CONFIG_FILE_ANALOG_THRESHOLD_PRESSED_WHEN_ATTRIBUTE[] =
    "pressedWhen";
static const char CONFIG_FILE_ANALOG_THRESHOLD_LESS_THAN[] = "less than";
static const char CONFIG_FILE_ANALOG_THRESHOLD_GREATER_THAN[] = "greater than";
static const char CONFIG_FILE_ANALOG_THRESHOLD_VALUE_ATTRIBUTE[] = "value";

// ########################################################################
// ########################################################################
// CONTROL FUNCTIONS AS STRINGS

// BUTTONS

//   ANIMATION functions:
static const char FUNC_NAME_KEYFRAME_ALL[] = "keyframeAll";
static const char FUNC_NAME_TOGGLE_KEYFRAME_OBJECT[] = "toggleKeyframeObject";
static const char FUNC_NAME_ADD_CAMERA[] = "addCamera";
static const char FUNC_NAME_SHOW_ANIMATION_PREVIEW[] = "showAnimationPreview";

//   GROUP EDITING functions:
static const char FUNC_NAME_TOGGLE_GROUP_MEMBERSHIP[] = "toggleGroupMembership";

//   COLOR EDITING functions:
static const char FUNC_NAME_CHANGE_OBJECT_COLOR[] = "changeObjectColor";
static const char FUNC_NAME_CHANGE_OBJECT_COLOR_VARIABLE[] =
    "changeObjectColorVariable";
static const char FUNC_NAME_TOGGLE_OBJECT_VISIBILITY[] =
    "toggleObjectVisibility";
static const char FUNC_NAME_TOGGLE_SHOW_INVISIBLE_OBJECTS[] =
    "toggleShowInvisibleObjects";

//   SPRING EDITING functions:
static const char FUNC_NAME_DELETE_SPRING[] = "deleteConnector";
static const char FUNC_NAME_SNAP_SPRING_TO_TERMINUS[] =
    "snapConnectorToTerminus";
static const char FUNC_NAME_SET_TERMINUS_TO_SNAP_SPRING[] =
    "setTerminusToSnapSpring";
static const char FUNC_NAME_CREATE_SPRING[] = "createSpring";
static const char FUNC_NAME_CREATE_TRANSPARENT_CONENCTOR[] =
    "createTransparentConnector";

//   GRAB functions:
static const char FUNC_NAME_GRAB_OBJECT_OR_WORLD[] = "grabObjectOrWorld";
static const char FUNC_NAME_GRAB_SPRING_OR_WORLD[] = "grabConnectorOrWorld";
static const char FUNC_NAME_SELECT_PARENT_OF_CURRENT[] =
    "selectParentOfCurrent";
static const char FUNC_NAME_SELECT_CHILD_OF_CURRENT[] = "selectChildOfCurrent";

//   TRANSFORM functions:
static const char FUNC_NAME_DELETE_OBJECT[] = "deleteObject";
static const char FUNC_NAME_REPLICATE_OBJECT[] = "createCrystalByExample";
static const char FUNC_NAME_SET_TRANSFORMS[] = "setTransform";

//   UTILITY functions:
static const char FUNC_NAME_RESET_VIEWPOINT[] = "resetCameraAngle";
static const char FUNC_NAME_COPY_OBJECT[] = "copyObject";
static const char FUNC_NAME_PASTE_OBJECT[] = "pasteObject";

// ANALOGS

//   TODO

// ########################################################################
// ########################################################################

ControlFunctions::ButtonControlFunctionPtr readButtonFunction(
    const char *funcName) {

  QString funcToAssign(funcName);

  if (funcToAssign == FUNC_NAME_KEYFRAME_ALL) {
    return &ControlFunctions::keyframeAll;
  } else if (funcToAssign == FUNC_NAME_TOGGLE_KEYFRAME_OBJECT) {
    return &ControlFunctions::toggleKeyframeObject;
  } else if (funcToAssign == FUNC_NAME_ADD_CAMERA) {
    return &ControlFunctions::addCamera;
  } else if (funcToAssign == FUNC_NAME_SHOW_ANIMATION_PREVIEW) {
    return &ControlFunctions::showAnimationPreview;
  } else if (funcToAssign == FUNC_NAME_TOGGLE_GROUP_MEMBERSHIP) {
    return &ControlFunctions::toggleGroupMembership;
  } else if (funcToAssign == FUNC_NAME_CHANGE_OBJECT_COLOR) {
    return &ControlFunctions::changeObjectColor;
  } else if (funcToAssign == FUNC_NAME_CHANGE_OBJECT_COLOR_VARIABLE) {
    return &ControlFunctions::changeObjectColorVariable;
  } else if (funcToAssign == FUNC_NAME_TOGGLE_OBJECT_VISIBILITY) {
    return &ControlFunctions::toggleObjectVisibility;
  } else if (funcToAssign == FUNC_NAME_TOGGLE_SHOW_INVISIBLE_OBJECTS) {
    return &ControlFunctions::toggleShowInvisibleObjects;
  } else if (funcToAssign == FUNC_NAME_DELETE_SPRING) {
    return &ControlFunctions::deleteSpring;
  } else if (funcToAssign == FUNC_NAME_SNAP_SPRING_TO_TERMINUS) {
    return &ControlFunctions::snapSpringToTerminus;
  } else if (funcToAssign == FUNC_NAME_SET_TERMINUS_TO_SNAP_SPRING) {
    return &ControlFunctions::setTerminusToSnapSpring;
  } else if (funcToAssign == FUNC_NAME_CREATE_SPRING) {
    return &ControlFunctions::createSpring;
  } else if (funcToAssign == FUNC_NAME_CREATE_TRANSPARENT_CONENCTOR) {
    return &ControlFunctions::createTransparentConnector;
  } else if (funcToAssign == FUNC_NAME_GRAB_OBJECT_OR_WORLD) {
    return &ControlFunctions::grabObjectOrWorld;
  } else if (funcToAssign == FUNC_NAME_GRAB_SPRING_OR_WORLD) {
    return &ControlFunctions::grabSpringOrWorld;
  } else if (funcToAssign == FUNC_NAME_SELECT_PARENT_OF_CURRENT) {
    return &ControlFunctions::selectParentOfCurrent;
  } else if (funcToAssign == FUNC_NAME_SELECT_CHILD_OF_CURRENT) {
    return &ControlFunctions::selectChildOfCurrent;
  } else if (funcToAssign == FUNC_NAME_DELETE_OBJECT) {
    return &ControlFunctions::deleteObject;
  } else if (funcToAssign == FUNC_NAME_REPLICATE_OBJECT) {
    return &ControlFunctions::replicateObject;
  } else if (funcToAssign == FUNC_NAME_SET_TRANSFORMS) {
    return &ControlFunctions::setTransforms;
  } else if (funcToAssign == FUNC_NAME_RESET_VIEWPOINT) {
    return &ControlFunctions::resetViewPoint;
  } else if (funcToAssign == FUNC_NAME_COPY_OBJECT) {
    return &ControlFunctions::copyObject;
  } else if (funcToAssign == FUNC_NAME_PASTE_OBJECT) {
    return &ControlFunctions::pasteObject;
  } else {
    return NULL;
  }
}

ControlFunctions::AnalogControlFunctionPtr readAnalogFunction(
    const char *funcName) {
  return NULL;
}

static inline void readThresholdAnalog(int analogNum, Mode &mode,
                                       vtkXMLDataElement *controlElt) {
  QSharedPointer<AnalogControlFunction> control(
      new AnalogThresholdControlFunction());
  AnalogThresholdControlFunction *controlT =
      dynamic_cast<AnalogThresholdControlFunction *>(control.data());
  int numThresholds = 0;
  int numSubElts = controlElt->GetNumberOfNestedElements();
  for (int k = 0; k < numSubElts; ++k) {
    vtkXMLDataElement *thresholdElt = controlElt->GetNestedElement(k);
    if (thresholdElt->GetName() !=
        QString(CONFIG_FILE_ANALOG_THRESHOLD_ELEMENT)) {
      continue;
    }
    const char *pW = thresholdElt->GetAttribute(
        CONFIG_FILE_ANALOG_THRESHOLD_PRESSED_WHEN_ATTRIBUTE);
    if (pW == NULL) {
      std::cout << "Analog assigment: Threshold has no pressed when attribute"
                << std::endl;
      continue;
    }
    QString pressedWhen(pW);
    AnalogThresholdControlFunction::ThresholdType t;
    if (pressedWhen == CONFIG_FILE_ANALOG_THRESHOLD_LESS_THAN) {
      t = AnalogThresholdControlFunction::LESS_THAN;
    } else if (pressedWhen == CONFIG_FILE_ANALOG_THRESHOLD_GREATER_THAN) {
      t = AnalogThresholdControlFunction::GREATER_THAN;
    } else {
      std::cout
          << "Analog assignment: Invalid condition in pressed when attribute"
          << std::endl;
      continue;
    }
    double value;
    if (!thresholdElt->GetScalarAttribute(
            CONFIG_FILE_ANALOG_THRESHOLD_VALUE_ATTRIBUTE, value)) {
      std::cout << "Analog assignment: Threshold has no value attribute"
                << std::endl;
      continue;
    }
    SketchBioHandId::Type side;
    if (!getHand(thresholdElt, side)) {
      std::cout << "Analog assignment: Threshold has no side attribute"
                << std::endl;
      continue;
    }
    const char *funcName =
        thresholdElt->GetAttribute(CONFIG_FILE_FUNCTION_ATTRIBUTE);
    if (funcName == NULL) {
      std::cout << "Analog assignment: Threshold has no function attribute"
                << std::endl;
      continue;
    }
    ControlFunctions::ButtonControlFunctionPtr bF =
        readButtonFunction(funcName);
    if (bF == NULL) {
      std::cout << "Analog assignment: Unknown function in threshold function "
                   "attribute" << std::endl;
      continue;
    }
    controlT->addThreshold(value, t, side, bF);
    ++numThresholds;
  }
  if (numThresholds > 2) {
    std::cout << "Analog assignment: Using three thresholds of the same analog "
                 "is unsupported" << std::endl;
    return;
  } else if (numThresholds == 0) {
    std::cout << "Analog assignment: Skipping analog with no valid thresholds"
              << std::endl;
    return;
  }
  mode.analogFunctions[analogNum] = control;
}

static inline void readNormalizeAnalog(int analogNum, Mode &mode,
                                       vtkXMLDataElement *controlElt) {
  vtkXMLDataElement *interval = controlElt->FindNestedElementWithName(
      CONFIG_FILE_ANALOG_INTERVAL_ELEMENT);
  if (interval == NULL) {
    std::cout << "Analog assignment: No interval for normalize type analog"
              << std::endl;
    return;
  }
  int min, max;
  if (!interval->GetScalarAttribute(CONFIG_FILE_ANALOG_INTERVAL_MIN_ATTRIBUTE,
                                    min)) {
    std::cout << "Analog assigment: Interval tag has no min attribute"
              << std::endl;
    return;
  }
  if (!interval->GetScalarAttribute(CONFIG_FILE_ANALOG_INTERVAL_MAX_ATTRIBUTE,
                                    max)) {
    std::cout << "Analog assignment: Interval tag has no max attribute"
              << std::endl;
    return;
  }
  SketchBioHandId::Type side;
  if (!getHand(interval, side)) {
    std::cout << "Analog assignment: Interval tag has no hand attribute"
              << std::endl;
    return;
  }
  const char *funcName = interval->GetAttribute(CONFIG_FILE_FUNCTION_ATTRIBUTE);
  if (funcName == NULL) {
    std::cout << "Analog assignment: Interval tag has no function attribute"
              << std::endl;
    return;
  }
  ControlFunctions::AnalogControlFunctionPtr func =
      readAnalogFunction(funcName);
  if (func == NULL) {
    std::cout << "Analog assignment: Unknown analog function: " << funcName
              << std::endl;
    return;
  }
  mode.analogFunctions[analogNum] = QSharedPointer<AnalogControlFunction>(
      new AnalogNormalizeControlFunction(min, max, side, func));
}

bool InputManager::InputManagerImpl::readModes(vtkXMLDataElement *root) {
  int maxButtons = buttonDevices.last().second->getOffset() +
                   buttonDevices.last().second->getMax();
  int maxAnalogs = analogDevices.last().second->getOffset() +
                   analogDevices.last().second->getMax();
  int rootNumNestedElements = root->GetNumberOfNestedElements();
  for (int i = 0; i < rootNumNestedElements; ++i) {
    vtkXMLDataElement *modeElt = root->GetNestedElement(i);
    if (modeElt->GetName() == QString(CONFIG_FILE_MODE_ELEMENT_NAME)) {
      const char *modeName =
          modeElt->GetAttribute(CONFIG_FILE_MODE_NAME_ATTRIBUTE);
      if (modeName == NULL) {
        std::cout << "Mode has no name" << std::endl;
        return false;
      }
      const char *defaultSelection =
          modeElt->GetAttribute(CONFIG_FILE_MODE_DEFAULT_SELECTION_ATTRIBUTE);
      SketchBio::OutlineType::Type outlineType;
      if (defaultSelection == NULL) {
        std::cout << "Mode has no default selection type" << std::endl;
        return false;
      } else if (defaultSelection ==
                 QString(CONFIG_FILE_SELECTION_TYPE_OBJECTS)) {
        outlineType = SketchBio::OutlineType::OBJECTS;
      } else if (defaultSelection ==
                 QString(CONFIG_FILE_SELECTION_TYPE_CONNECTOR)) {
        outlineType = SketchBio::OutlineType::CONNECTORS;
      } else {
        std::cout << "Mode has unknown selection type" << std::endl;
        return false;
      }
      modes.append(Mode(maxButtons, maxAnalogs, modeName, outlineType));
      Mode &mode = modes.last();
      int modeNumNested = modeElt->GetNumberOfNestedElements();
      for (int j = 0; j < modeNumNested; ++j) {
        vtkXMLDataElement *controlElt = modeElt->GetNestedElement(j);
        if (controlElt->GetName() ==
            QString(CONFIG_FILE_BUTTON_CTRL_ELEMENT_NAME)) {
          int buttonNum;
          SketchBioHandId::Type side;
          if (!getButtonNumber(controlElt, buttonDevices, buttonNum)) {
            std::cout << "Button assignment: Could not locate device or "
                         "buttonNumber attribute." << std::endl;
            continue;
          }
          if (!getHand(controlElt, side)) {
            std::cout << "Button assignment: Could not locate a hand attribute."
                      << std::endl;
            continue;
          }
          const char *func =
              controlElt->GetAttribute(CONFIG_FILE_FUNCTION_ATTRIBUTE);
          if (func == NULL) {
            std::cout
                << "Button assignment: Could not locate a function attribute."
                << std::endl;
            continue;
          }
          ControlFunctions::ButtonControlFunctionPtr bF =
              readButtonFunction(func);
          if (bF == NULL) {
            std::cout << "Button assignment: Unknown function = " << func
                      << std::endl;
            continue;
          }
          mode.buttonFunctions.data()[buttonNum].init(bF, side);
        } else if (controlElt->GetName() ==
                   QString(CONFIG_FILE_ANALOG_CTRL_ELEMENT_NAME)) {
          int analogNum;
          if (!getAnalogNumber(controlElt, analogDevices, analogNum)) {
            std::cout << "Analog assignment: Could not locate device or "
                         "buttonNumber attributes." << std::endl;
            continue;
          }
          if (controlElt->GetAttribute(CONFIG_FILE_ANALOG_TYPE_ATTRIBUTE) ==
              NULL) {
            std::cout << "Analog assignment: Could not locate type attribute."
                      << std::endl;
            continue;
          }
          QString type(
              controlElt->GetAttribute(CONFIG_FILE_ANALOG_TYPE_ATTRIBUTE));
          if (type == CONFIG_FILE_ANALOG_TYPE_THRESHOLD) {
            readThresholdAnalog(analogNum, mode, controlElt);
          } else if (type == CONFIG_FILE_ANALOG_TYPE_NORMALIZE) {
            readNormalizeAnalog(analogNum, mode, controlElt);
          } else {
            std::cout << "Analog assignment: Unknown type attribute value"
                      << std::endl;
            continue;
          }
        }
      }
    }
  }
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
