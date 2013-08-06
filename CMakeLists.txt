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

FIND_PACKAGE(VTK 6.1 COMPONENTS vtkGUISupportQt vtkIOGeometry vtkIOLegacy NO_MODULE)
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

FIND_PACKAGE(PQP REQUIRED)

INCLUDE_DIRECTORIES(
    ${PQP_INCLUDE_DIRS}
)

# This is to add modules, note that the name of each module is the
# name of its directory and that the name of the library created by
# each module is SketchBioX where X is the module name.
macro(addmodule name)
    ADD_SUBDIRECTORY(${name})
    INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/${name}" )
    SET(SketchBioLibraries "SketchBio${name}" ${SketchBioLibraries} )
endmacro()

ENABLE_TESTING()

addmodule( CustomVtkFilters )
addmodule( Core )
addmodule( Input )
addmodule( Export )
addmodule( Subprocess )

# Set your files and resources here
SET(SketchBioSrcs main.cpp SimpleView.cpp
)
SET(SimpleViewUI SimpleView.ui)
SET(Headers_Needing_QT_WRAP SimpleView.h
)
 
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
 
ADD_EXECUTABLE( SketchBio ${SketchBioSrcs} ${UISrcs} ${MOCSrcs})

TARGET_LINK_LIBRARIES( SketchBio
    ${SketchBioLibraries}
    ${VTK_LIBRARIES}
    ${QT_LIBRARIES}
    ${VRPN_LIBRARIES}
    ${VRPN_SERVER_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    ${PQP_LIBRARIES}
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

# make a program to clear the settings to test what will happen on a clean install
ADD_EXECUTABLE( ClearSettings clearsettings.cpp )
TARGET_LINK_LIBRARIES( ClearSettings ${QT_LIBRARIES} )
