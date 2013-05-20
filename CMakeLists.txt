cmake_minimum_required(VERSION 2.8)
 
PROJECT(SketchBio)

# Copied from Microscope Simulator... needed for bundling code
SET(SketchBio_MAJOR_NUMBER 0)
SET(SketchBio_MINOR_NUMBER 0)
SET(SketchBio_REVISION_NUMBER 1)
SET(SketchBio_VERSION_STRING "${SketchBio_MAJOR_NUMBER}.${SketchBio_MINOR_NUMBER}.${SketchBio_REVISION_NUMBER}")

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

###
# Libusb1
###
IF(NOT WIN32 AND NOT APPLE)
	find_package(Libusb1 REQUIRED)

    INCLUDE_DIRECTORIES(
        ${LIBUSB1_INCLUDE_DIRS}
    )
ENDIF(NOT WIN32 AND NOT APPLE)

# support for out-of-source build
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR} #this is where ui_SimpleView.h is generated
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# For PQP.
if (WIN32)
    LINK_DIRECTORIES(C:/usr/local/lib)
endif (WIN32)

# Set your files and resources here
SET(SimpleViewSrcs main.cpp SimpleView.cpp transformmanager.cpp structurereplicator.cpp modelmanager.cpp
sketchobject.cpp sketchmodel.cpp worldmanager.cpp springconnection.cpp sketchproject.cpp projecttoxml.cpp
physicsstrategy.cpp transformequals.cpp vrpnserver.cpp keyframe.cpp projecttoblenderanimation.cpp
hydrainputmanager.cpp hydrainputmode.cpp objecteditingmode.cpp springeditingmode.cpp)
SET(SimpleViewUI SimpleView.ui)
SET(Headers_Needing_QT_WRAP SimpleView.h vrpnserver.h hydrainputmanager.h hydrainputmode.h
objecteditingmode.h springeditingmode.h)
SET(Headers_Not_Needing_QT_WRAP transformmanager.h structurereplicator.h modelmanager.h
sketchobject.h sketchmodel.h worldmanager.h springconnection.h sketchproject.h sketchioconstants.h
projecttoxml.h sketchtests.h physicsstrategy.h transformequals.h groupidgenerator.h keyframe.h
projecttoblenderanimation.h )
 
QT4_WRAP_UI(UISrcs ${SimpleViewUI})
QT4_WRAP_CPP(MOCSrcs ${Headers_Needing_QT_WRAP} )
 
SOURCE_GROUP("Resources" FILES
  ${SimpleViewUI}
)
 
SOURCE_GROUP("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
)

SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -ggdb")
 
ADD_EXECUTABLE( SketchBio ${SimpleViewSrcs} ${Headers_Not_Needing_QT_WRAP} ${UISrcs} ${MOCSrcs})
TARGET_LINK_LIBRARIES( SketchBio
    QVTK
    ${VTK_LIBRARIES}
    ${VRPN_LIBRARIES}
    ${VRPN_SERVER_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    PQP
    SketchSubprocess
)
IF(UNIX AND NOT APPLE)
	TARGET_LINK_LIBRARIES( SketchBio ${LIBUSB1_LIBRARIES} )
ENDIF(UNIX AND NOT APPLE)

IF(APPLE)
    TARGET_LINK_LIBRARIES( SketchBio
        "-framework IOKit"
        "-framework CoreFoundation"
    )
ENDIF(APPLE)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(subprocesses)


# Copied from Microscope Simulator.  Used for creating the installer.
SET( CPACK_PACKAGE_NAME "SketchBio" )
SET( CPACK_PACKAGE_VENDOR "CISMM" )
SET( CPACK_PACKAGE_EXECUTABLES "SketchBio;SketchBio ${SketchBio_VERSION_STRING}" )
SET( CPACK_PACKAGE_DESCRIPTION_SUMMARY "SketchBio" )
SET( CPACK_PACKAGE_VERSION "${SketchBio_VERSION_STRING}" )
SET( CPACK_PACKAGE_VERSION_MAJOR "${SketchBio_MAJOR_NUMBER}" )
SET( CPACK_PACKAGE_VERSION_MINOR "${SketchBio_MINOR_NUMBER}" )
SET( CPACK_PACKAGE_VERSION_PATCH "${SketchBio_REVISION_NUMBER}" )
SET( CPACK_PACKAGE_INSTALL_DIRECTORY "SketchBio ${SketchBio_VERSION_STRING}" )
SET( CPACK_PACKAGE_INSTALL_REGISTRY_KEY "SketchBio ${SketchBio_VERSION_STRING}" )
SET( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/installer/licenses/SketchBio_license.txt" )
SET( CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/installer/licenses/README.txt" )

INCLUDE( installer/CMakeLists.txt )

INSTALL(TARGETS SketchBio
    RUNTIME DESTINATION bin
)

# This must always be last (according to Cory's code in MicroscopeSimulator)
INCLUDE( CPack )
