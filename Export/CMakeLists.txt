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

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/mybpyhelpers.py
        DESTINATION ${CMAKE_BINARY_DIR}/scripts )

SOURCE_GROUP( "Resources" FILES
${CMAKE_CURRENT_SOURCE_DIR}/scripts/mybpyhelpers.py
)

ADD_LIBRARY(SketchBioExport ${SketchBioExportSrcs})

ADD_SUBDIRECTORY( test )
