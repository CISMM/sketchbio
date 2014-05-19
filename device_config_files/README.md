## XML Device Configuration

Date Modified: 01 May 2014  
Applies to: SketchBioDeviceConfig file version 0.0

- - -
### Description

SketchBio is designed to accept input from a wide variety of VRPN-supported 
devices. A _device configuration_ is used to tell SketchBio how to interpret 
the input from either (i) a single device or (ii) multiple devices in order to 
provide the user with the ability to interact with a scene. The _device 
configuration_ is written in XML so that it can be edited by an administrator 
in a text editor.

A functional _device configuration_ must include several buttons and analogs, 
as well as 1-2 trackers, across all of the devices included in the 
configuration.

- - -
### Modes

Most devices do not provide a button for every function in SketchBio. To 
account for this limitation, SketchBio allows functions to be grouped into 
"modes". An individual mode is a subset of functions that (together) perform a 
logically coherent task (e.g. Edit Objects).

A user cycles through modes using the designated "mode switch button". This is 
a dedicated button that performs no other function within SketchBio.

- - -
### Elements

1. __SketchBioDeviceConfig__
    * Description  
    This is the __root__ element for the XML document.
    * Attributes
        - fileVersion
            * Format  
            Integer.Integer (Major.Minor) version number
            * Description  
            The SketchBioDeviceConfig format will be updated, as 
            needed. With each new update, the current version of the 
            SketchBioDeviceConfig will be incremented accordingly. Thus, the 
            fileVersion indicates the SketchBioDeviceConfig version of the 
            current XML configuration.
    * Child Elements
        - vrpnFile (QTY: 1)
        - device (QTY: 1+)
        - trackerInputTransform (QTY: 1)
        - modeSwitchButton (QTY: 1)
        - mode (QTY: 1+)
2. __vrpnFile__
    * Description  
    This element must reference a single VRPN configuration file (.cfg). The 
    referenced .cfg file will dictate the names of each device used in this XML
     configuration file.
    * Content  
    The name of the .cfg file (include the .cfg extension).
3. __device__
    * Description  
    This element and its children tell SketchBio (i) which VRPN devices to 
    accept input from and (ii) what inputs are provided by each device.
    * Attributes
        - name
            * Format  
            Text
            * Description  
            This represents a device's "handle" on the VRPN server. It is a 
            unique ID that is specified by the .cfg file. It allows SketchBio 
            to distinguish between multiple devices.
    * Child Elements
        - buttons (QTY: 0-1)
        - analogs (QTY: 0-1)
        - tracker (QTY: 0-2)
4. __buttons__
    * Description  
    This element tells SketchBio that the device (i.e. parent element) will 
    provide button input.
    * Attributes
        - num
            * Format  
            Integer
            * Description  
            The "num" attribute indicates the number of buttons on the device.
5. __analogs__
    * Description  
    This element tells SketchBio that the device (i.e. parent element) will 
    provide analog input.
    * Attributes
        - num
            * Format  
            Integer
            * Description  
            The "num" attribute indicates the number of analog inputs on the 
            device.
6. __tracker__
    * Description  
    This element tells SketchBio that the device (i.e. parent element) will 
    provide tracker data. In VRPN, a "tracker" is a device that provides 
    _positional_ and _rotational_ data. It is recommended to use two trackers 
    with SketchBio, thereby gaining the ability to use both hands to manipulate
     a scene.
    * Attributes
        - channel
            * Format  
            Integer
            * Description  
            A VRPN device _may_ provide multiple trackers via a single device 
            (e.g. the Razer Hydra). Thus, VRPN distinguishes each tracker via 
            separate "channels". Tracker numbering begins at 0.
        - hand
            * Format  
            Text (either "left" or "right")
            * Description  
            SketchBio uses data from a tracker to guide an on-screen "hand" 
            (a single, grey sphere). This attribute tells SketchBio which hand 
            to associate with the given _channel_, on the given _device_.
            * Note  
            If you would like to use a single tracker with SketchBio, then 
            specify two tracker elements and assign channel 0 to both the left 
            and right hands.  For example:  
            `<tracker channel="0" hand="left"/>`  
            `<tracker channel="0" hand="right"/>`  
7. __trackerInputTransform__
    * Description  
    Tracker devices differ in their interpretation of "real world" physical 
    movement. Thus, trackerInputTransform is set _per device_ in order to turn 
    the data from a given tracker into "comfortable" movement within SketchBio.
    * Attributes  
        - matrix  
            * Format  
            4 x 4 matrix of floating-point decimal values (arranged as a 
            single, space-delimited row, in row-major format)
            * Description  
            This is an affine transform matrix.
8. __modeSwitchButton__
    * Description  
    This tells SketchBio which button (on which device) will be used to cycle 
    through modes.
    * Attributes  
        - deviceName  
            * Format  
            Text
            * Description  
            This indicates which device provides the mode switch button.
        - buttonNumber  
            * Format  
            Integer
            * Description  
            This indicates the button (on the indicated device) that will serve
             as the mode switch button.
9. __mode__
    * Description  
    As previously mentioned, a _mode_ is a subset of functions that perform a 
    logically coherent task. Modes are used to deal with the fact that few 
    devices provide enough buttons to cover all of SketchBio's control 
    functions. 
    * Attributes
        - name  
            * Format  
            Text
            * Description  
            A mode's _name_ will be displayed along the bottom of the SketchBio
             graphical user interface (GUI). The name should briefly describe 
            the "coherent task" being performed by the mode's subset of 
            functions.
        - defaultSelection  
            * Format  
            Text (either "Objects" or "Connectors")
            * Description  
            This attribute tells SketchBio whether to highlight objects or 
            connectors as a user interacts with a scene. This should match the 
            task being performed by the current mode (e.g. the 
            defaultSelection for the "Edit Springs" mode should be set to 
            "Connectors").
    * Child Elements
        - button (QTY: 0+)
        - analog (QTY: 0+)
10. __button__
    * Description  
    A button element tells SketchBio what function is performed when a given 
    button is either pressed or released. This assignment is only valid for 
    the current mode (i.e. parent element).
    * Attributes  
        - deviceName
            * Format  
            Text
            * Description  
            This indicates which device provides the specified button.
        - buttonNumber
            * Format  
            Integer
            * Description  
            This indicates the button (on the indicated device) that will 
            perform the specified function.
        - hand
            * Format  
            Text (either "left" or "right")
            * Description  
            This tells SketchBio which hand to associate with the indicated 
            button.
        - function
            * Format  
            Text (see the section "Control Functions" for a list of function
            names)
            * Description  
            This indicates the function that is performed when the specificed 
            button is either pressed or released.
11. __analog__
    * Description  
    In SketchBio, analog inputs can function as either (i) "standard" analog 
    inputs, in which case the full range of motion is used as data, or (ii) as
     "pseudo-buttons", in which case specified sub-ranges are treated as being 
    pressed/released. A "standard" button is designated as _normalize_, and a 
    "pseudo-button" is designated as _threshold_. This designation is only 
    valid for the current mode (i.e. parent element).
    * Attributes
        - deviceName
            * Format  
            Text
            * Description  
            This indicates which device provides the specified analog.
        - analogNumber
            * Format  
            Integer
            * Description  
            This indicates the analog (on the indicated device) that will 
            perform the specified function(s).
        - type
            * Format  
            Text (either "normalize" or "threshold")
            * Description  
            This tells SketchBio how to treat the indicated analog. If the 
            _type_ is set to "normalize", then SketchBio will attempt to 
            process _interval_ child elements. Conversely, if the _type_ is 
            set to "threshold", then SketchBio will attempt to process 
            _threshold_ child elements.
    * Child Elements
        - interval (QTY: 1) _Note: Only applies to analog type "normalize"_
        - threshold (QTY: 1-2) _Note: Only applies to analog type "threshold"_
            * Note  
            A "pseudo-button" (i.e. _threshold_) may be assigned up to two 
            functions via the designation of two non-overlapping sub-ranges.
12. __interval__
    * Description  
    Analog inputs have different ranges (minimum and maximum values). To deal 
    with this, SketchBio normalizes all "standard" analog buttons to the range 
    [0, 1]. In order to perform this normalization, SketchBio must be provided 
    the full range of the specified analog.
    * Attributes
        - min
            * Format  
            Floating-point decimal
            * Description  
            The minimum value of the analog's range.
        - max
            * Format  
            Floating-point decimal
            * Description  
            The maximum value of the analog's range.
        - hand
            * Format  
            Text (either "left" or "right")
            * Description  
            This tells SketchBio which hand to associate with the indicated 
            analog.
        - function
            * Format  
            Text (see the section "Control Functions" for a list of function
            names)
            * Description  
            This indicates the function that is performed when the specificed 
            analog is manipulated.
13. __threshold__
    * Description  
    _Threshold_ analogs are treated as if they are buttons (i.e. 
    pseudo-buttons). This is achieved by designating sub-ranges as "pressed" 
    regions (i.e. consider the pseudo-button to be "pressed" when the analog
    reports a value within a specified range).
    * Attributes
        - pressedWhen
            * Format  
            Text (either "greater than" or "less than")
            * Description  
            This attribute, when combined with the _value_ attribute, defines 
            the sub-range in which the pseudo-button is considered to be 
            "pressed". Specifically, this attribute designates which "side" of 
            the _value_ is considered to "pressed".
        - value
            * Format  
            Floating-point decimal
            * Description  
            This attribute serves as the boundary between the "pressed" and 
            "released" sub-ranges of a _threshold_ analog.
        - hand
            * Format  
            Text (either "left" or "right")
            * Description  
            This tells SketchBio which hand to associate with the indicated 
            analog.
        - function
            * Format  
            Text (see the section "Control Functions" for a list of function
            names)
            * Description  
            This indicates the function that is performed when the specificed 
            analog is manipulated.

- - -
### Control Functions

The lists below indicate valid text values for _function_ attributes.

* Control Functions for Buttons and _Threshold_ Analogs (i.e. Pseudo-Buttons)
    - redo
    - toggleCollisionChecks
    - toggleSpringsEnabled
    - zoom
    - grabObjectOrWorld
    - resetCameraAngle
    - undo
    - createCrystalByExample
    - lockTransforms
    - setTransform
    - deleteObject
    - selectChildOfCurrent
    - selectParentOfCurrent
    - grabConnectorOrWorld
    - deleteConnector
    - snapConnectorToTerminus
    - setTerminusToSnapSpring
    - createSpring
    - createTransparentConnector
    - createMeasuringTape
    - keyframeAll
    - toggleKeyframeObject
    - addCamera
    - showAnimationPreview
    - changeObjectColor
    - changeObjectColorVariable
    - toggleObjectVisibility
    - toggleShowInvisibleObjects
    - toggleGroupMembership
    - copyObject
    - pasteObject
* Control Functions for _Normalize_ Analogs
    - rotateCameraYaw
    - rotateCameraPitch
    - setCrystalByExampleCopies
    - moveAlongTimeline

- - -
_This document is written in Markdown GFM ([GitHub Flavored Markdown](https://help.github.com/articles/github-flavored-markdown))._
