INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

SET(SketchBioExportSrcs
projecttoxml.cpp
projecttoxml.h
projecttoblenderanimation.cpp
projecttoblenderanimation.h
)

ADD_LIBRARY(SketchBioExport ${SketchBioExportSrcs})

ADD_SUBDIRECTORY( test )
