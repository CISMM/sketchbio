INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

SET(SketchBioExportSrcs
projecttoxml.cpp
projecttoxml.h
projecttoblenderanimation.cpp
projecttoblenderanimation.h
ProjectToFlorosim.cpp
ProjectToFlorosim.h
FlorosimExportTopOfFile.h
FlorosimExportBottomOfFile.h
)

FILE(GLOB mybpyhelpers "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mybpyhelpers.py")

add_custom_target(CopyScriptsToBuild ALL COMMENT
    "Copying blender python helper script to scripts directory..." ${mybpyhelpers})

add_custom_command(TARGET CopyScriptsToBuild POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts
    ${CMAKE_BINARY_DIR}/scripts)

ADD_LIBRARY(SketchBioExport ${SketchBioExportSrcs})

ADD_SUBDIRECTORY( test )
