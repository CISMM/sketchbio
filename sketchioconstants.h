#ifndef SKETCHIOCONSTANTS_H
#define SKETCHIOCONSTANTS_H


// parameters that define how trackers and objects are connected
#define DISTANCE_THRESHOLD 0
#define SPRING_DISTANCE_THRESHOLD 40
#define OBJECT_SIDE_LEN 200
#define TRACKER_SIDE_LEN 200
#define OBJECT_GRAB_SPRING_CONST 2

// constants for grabbing the world code
#define WORLD_NOT_GRABBED 0
#define LEFT_GRABBED_WORLD 1
#define RIGHT_GRABBED_WORLD 2


#define NUM_HYDRA_BUTTONS 16
#define NUM_HYDRA_ANALOGS 6

// input constants
#define HYDRA_SCALE_FACTOR 8.0f

#define BUTTON_LEFT(x) (x+0)
#define BUTTON_RIGHT(x) (x+8)

#define OBLONG_BUTTON_IDX   (0)
#define ONE_BUTTON_IDX      (1)
#define TWO_BUTTON_IDX      (2)
#define THREE_BUTTON_IDX    (3)
#define FOUR_BUTTON_IDX     (4)
#define BUMPER_BUTTON_IDX   (5)

#define LEFT_RIGHT_ANALOG_IDX   (0)
#define UP_DOWN_ANALOG_IDX      (1)
#define TRIGGER_ANALOG_IDX      (2)

#define ANALOG_LEFT(x) (x+0)
#define ANALOG_RIGHT(x) (x+3)

namespace HydraButtonMapping {

inline int scale_button_idx() {
    return BUTTON_LEFT(FOUR_BUTTON_IDX);
}
inline int spring_disable_button_idx() {
    return BUTTON_LEFT(THREE_BUTTON_IDX);
}
inline int spring_add_button_idx() {
    return BUTTON_RIGHT(THREE_BUTTON_IDX);
}
inline int transform_equals_add_button_idx() {
    return BUTTON_RIGHT(TWO_BUTTON_IDX);
}
inline int replicate_object_button() {
    return BUTTON_RIGHT(ONE_BUTTON_IDX);
}
inline int duplicate_object_button() {
    return BUTTON_RIGHT(FOUR_BUTTON_IDX);
}
inline int collision_disable_button_idx() {
    return BUTTON_LEFT(TWO_BUTTON_IDX);
}
inline int change_modes_button_idx() {
    return BUTTON_LEFT(ONE_BUTTON_IDX);
}

}

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
