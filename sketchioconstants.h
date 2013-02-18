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

// magic constants to take out later:
// debuggin flag
#define VRPN_ON true

// transform manager's tracker to workspace scale factor
#define TRANSFORM_MANAGER_TRACKER_COORDINATE_SCALE .0625

// scale between world and camera (also used to determine tracker size)
#define SCALE_DOWN_FACTOR (.03125)

#endif // SKETCHIOCONSTANTS_H
