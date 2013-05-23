SET(Subprocess_src "blenderanimationrunner.cpp" "chimeraobjmaker.cpp" "subprocessutils.cpp"
"blenderdecimationrunner.cpp" "pymolobjmaker.cpp")
SET(Subprocess_qt_headers "blenderanimationrunner.h" "chimeraobjmaker.h" "subprocessrunner.h"
"blenderdecimationrunner.h" "pymolobjmaker.h")
SET(Subprocess_non_qt_headers "subprocessutils.h" "subprocessrunner.h")

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

QT4_WRAP_CPP(Subprocess_MOC_Srcs ${Subprocess_qt_headers})

SOURCE_GROUP( "Generated" FILES
             ${Subprocess_MOC_Srcs}
)

ADD_LIBRARY(SketchSubprocess ${Subprocess_src} ${Subprocess_MOC_Srcs} ${Subprocess_non_qt_headers})
