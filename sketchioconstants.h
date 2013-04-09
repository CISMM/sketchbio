#ifndef SKETCHIOCONSTANTS_H
#define SKETCHIOCONSTANTS_H


// parameters that define how trackers and objects are connected
#define DISTANCE_THRESHOLD 0
#define OBJECT_SIDE_LEN 200
#define TRACKER_SIDE_LEN 200

// constants for grabbing the world code
#define WORLD_NOT_GRABBED 0
#define LEFT_GRABBED_WORLD 1
#define RIGHT_GRABBED_WORLD 2


#define NUM_HYDRA_BUTTONS 16
#define NUM_HYDRA_ANALOGS 6

// input constants
#define SCALE_BUTTON 5
#define ROTATE_BUTTON 13
#define PAUSE_PHYSICS_BUTTON 2
#define HYDRA_SCALE_FACTOR 8.0f
#define HYDRA_LEFT_TRIGGER 2
#define HYDRA_RIGHT_TRIGGER 5

// debugging flag -- if false does not start vrpn clients
#define VRPN_ON true
// change this to false to use an external vrpn server
#define VRPN_USE_INTERNAL_SERVER true

// the device name used on the server -- must change the line below when this is changed
#define VRPN_RAZER_HYDRA_DEVICE_NAME "razer"
// the device name and location used by the client -- must update when the device name above updates
#define VRPN_RAZER_HYDRA_DEVICE_STRING "razer@localhost"

// transform manager's tracker to workspace scale factor
#define TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE (.0625 )

// scale between world and camera (also used to determine tracker size)
#define SCALE_DOWN_FACTOR (.03125)
#define STARTING_CAMERA_POSITION (50.0)

// the name of the project's save file in the project directory
#define PROJECT_XML_FILENAME "project.xml"

#endif // SKETCHIOCONSTANTS_H
