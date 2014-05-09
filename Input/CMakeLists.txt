INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Core
    ${CMAKE_SOURCE_DIR}/Export
)

SET(SketchBioInputQtHeaders
InputManager.h
signalemitter.h
transformeditoperationstate.h
TransformInputDialog.h
)
QT4_WRAP_CPP(SketchBioInputMoc ${SketchBioInputQtHeaders})
SOURCE_GROUP("Generated" FILES ${SketchBioInputMoc})

SET(SketchBioInputSrcs
TransformInputDialog.cpp  # h file used by Moc, no need to include here
InputManager.cpp
controlFunctions.cpp
controlFunctions.h
savedxmlundostate.cpp
savedxmlundostate.h
transformeditoperationstate.cpp
${SketchBioInputQtHeaders}
${SketchBioInputMoc}
)
ADD_LIBRARY(SketchBioInput ${SketchBioInputSrcs})

ADD_SUBDIRECTORY( test )
