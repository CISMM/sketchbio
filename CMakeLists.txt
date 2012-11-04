cmake_minimum_required(VERSION 2.8)
 
PROJECT(SketchBio)
list(APPEND CMAKE_MODULE_PATH ${SketchBio_SOURCE_DIR}/cmake)
 
FIND_PACKAGE(VRPN REQUIRED)
FIND_PACKAGE(quatlib REQUIRED)

INCLUDE_DIRECTORIES(
  ${QUATLIB_INCLUDE_DIR}
  ${VRPN_INCLUDE_DIR}
)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})  # include UseQt4.cmake

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE}) # include UseVTK.cmake

# support for out-of-source build
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR} #this is where ui_SimpleView.h is generated
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# For PQP.
LINK_DIRECTORIES(C:/usr/local/lib)

# Set your files and resources here
SET(SimpleViewSrcs main.cpp SimpleView.cpp transformmanager.cpp structurereplicator.cpp modelmanager.cpp
sketchobject.cpp sketchmodel.cpp worldmanager.cpp springconnection.cpp)
SET(SimpleViewUI SimpleView.ui)
SET(SimpleViewHeaders SimpleView.h transformmanager.h structurereplicator.h modelmanager.h
sketchobject.h sketchmodel.h worldmanager.h springconnection.h)
 
QT4_WRAP_UI(UISrcs ${SimpleViewUI})
QT4_WRAP_CPP(MOCSrcs ${SimpleViewHeaders} )
 
SOURCE_GROUP("Resources" FILES
  ${SimpleViewUI}
)
 
SOURCE_GROUP("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
)

SET(CMAKE_CXX_FLAGS_DEBUG -Wall -g -ggdb)
 
ADD_EXECUTABLE( SketchBio ${SimpleViewSrcs} ${UISrcs} ${MOCSrcs})
TARGET_LINK_LIBRARIES( SketchBio
	QVTK ${VRPN_LIBRARIES} ${QUATLIB_LIBRARIES} PQP
)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)

