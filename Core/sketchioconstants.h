#ifndef SKETCHIOCONSTANTS_H
#define SKETCHIOCONSTANTS_H

// parameters that define how trackers and objects are connected
static const double DISTANCE_THRESHOLD = 0;
static const double SPRING_DISTANCE_THRESHOLD = 40;

namespace SketchBioHandId
{
enum Type { LEFT = 0, RIGHT = 1 };
}

namespace SketchBio
{
namespace OutlineType
{
enum Type { CONNECTORS, OBJECTS };
}
}

// scale between world and camera (also used to determine tracker size)
static const double SCALE_DOWN_FACTOR = .03125;
static const double STARTING_CAMERA_POSITION = 50.0;

// the name of the project's save file in the project directory
static const char PROJECT_XML_FILENAME[] = "project.xml";

// the name of a structure's save file
static const char STRUCTURE_XML_FILENAME[] = "structure.xml";

// default mass and moment of inertia
static const double DEFAULT_INVERSE_MASS = 1.0;
static const double DEFAULT_INVERSE_MOMENT = 1.0 / 25000;

// shadow plane constants

// the y height of the plane in room units
static const double PLANE_ROOM_Y = -6;
// the offset in room space of the lines above the plane to prevent OpenGL
// depth problems
static const double LINE_FROM_PLANE_OFFSET = 0.3;

// end shadow plane constants

#endif  // SKETCHIOCONSTANTS_H
