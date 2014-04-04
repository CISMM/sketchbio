INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/export
)

SET(Subprocess_src "blenderanimationrunner.cpp" "chimeravtkexportrunner.cpp"
"subprocessutils.cpp" "blenderdecimationrunner.cpp" "pymolobjmaker.cpp"
"abstractsingleprocessrunner.cpp" "modelfrompdbrunner.cpp")
SET(Subprocess_qt_headers "blenderanimationrunner.h" "chimeravtkexportrunner.h"
"subprocessrunner.h" "blenderdecimationrunner.h" "pymolobjmaker.h"
"abstractsingleprocessrunner.h" "modelfrompdbrunner.h")
SET(Subprocess_non_qt_headers "subprocessutils.h" "subprocessrunner.h")

QT4_WRAP_CPP(Subprocess_MOC_Srcs ${Subprocess_qt_headers})

SOURCE_GROUP( "Generated" FILES
             ${Subprocess_MOC_Srcs}
)

ADD_LIBRARY(SketchBioSubprocess ${Subprocess_src} ${Subprocess_MOC_Srcs} ${Subprocess_non_qt_headers})

FILE(GLOB chimera_ext_src "${CMAKE_CURRENT_SOURCE_DIR}/chimeraExtension/ExportVTK/*")
add_custom_target(CopyChimeraExtension ALL COMMENT
    "Copying Chimera extension to scripts directory..." SOURCES
    ${chimera_ext_src})

# do a directory copy so that new files in the extension also will be copied
#
# could do copyFilesIfDifferent, but then wierd errors if files added and no
# reconfigure performed
add_custom_command(TARGET CopyChimeraExtension POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/chimeraExtension/ExportVTK
    ${CMAKE_BINARY_DIR}/scripts/ExportVTK)

ADD_SUBDIRECTORY(test)

TARGET_LINK_LIBRARIES( SketchBioSubprocess ${QUAZIP_LIBRARIES} )