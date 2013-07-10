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

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/chimeraExtension/ExportVTK/__init__.py
        DESTINATION ${CMAKE_BINARY_DIR}/scripts/ExportVTK )
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/chimeraExtension/ExportVTK/ChimeraExtension.py
        DESTINATION ${CMAKE_BINARY_DIR}/scripts/ExportVTK )

SOURCE_GROUP("Resources" FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/chimeraExtension/ExportVTK/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/chimeraExtension/ExportVTK/ChimeraExtension.py
)

ADD_SUBDIRECTORY(test)
