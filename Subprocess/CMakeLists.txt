INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/export
)

SET(Subprocess_src "blenderanimationrunner.cpp" "chimeraobjmaker.cpp" "subprocessutils.cpp"
"blenderdecimationrunner.cpp" "pymolobjmaker.cpp" "abstractsingleprocessrunner.cpp"
"modelfrompdbrunner.cpp")
SET(Subprocess_qt_headers "blenderanimationrunner.h" "chimeraobjmaker.h" "subprocessrunner.h"
"blenderdecimationrunner.h" "pymolobjmaker.h" "abstractsingleprocessrunner.h"
"modelfrompdbrunner.h")
SET(Subprocess_non_qt_headers "subprocessutils.h" "subprocessrunner.h")

QT4_WRAP_CPP(Subprocess_MOC_Srcs ${Subprocess_qt_headers})

SOURCE_GROUP( "Generated" FILES
             ${Subprocess_MOC_Srcs}
)

ADD_LIBRARY(SketchBioSubprocess ${Subprocess_src} ${Subprocess_MOC_Srcs} ${Subprocess_non_qt_headers})

ADD_SUBDIRECTORY(test)
