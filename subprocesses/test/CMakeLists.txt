SET(SUBPROCESS_TEST_LINK_LIBRARIES
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    PQP
    SketchSubprocess
    TestSubprocess
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIRECTORY}
    ${CMAKE_CURRENT_BINARY_DIRECTORY}
)

# TODO -- take this out when restructuring project so that sketchproject and
# it dependencies are in a library.  Replace with adding that library
SET(SketchObject_Cpp
            "${CMAKE_SOURCE_DIR}/sketchobject.cpp"
            "${CMAKE_SOURCE_DIR}/sketchmodel.cpp"
            "${CMAKE_SOURCE_DIR}/keyframe.cpp"
)
SET(SketchObject_MOC_Cpp
            "${CMAKE_SOURCE_DIR}/sketchmodel.h"
)
SET(WorldMgr_Cpp
             ${SketchObject_Cpp}
            "${CMAKE_SOURCE_DIR}/springconnection.cpp"
            "${CMAKE_SOURCE_DIR}/physicsstrategy.cpp"
            "${CMAKE_SOURCE_DIR}/worldmanager.cpp"
)
SET(WorldMgr_MOC_Cpp
            ${SketchObject_MOC_Cpp}
)
SET(SketchProject_Cpp
             ${WorldMgr_Cpp}
            "${CMAKE_SOURCE_DIR}/modelmanager.cpp"
            "${CMAKE_SOURCE_DIR}/transformmanager.cpp"
            "${CMAKE_SOURCE_DIR}/transformequals.cpp"
            "${CMAKE_SOURCE_DIR}/structurereplicator.cpp"
            "${CMAKE_SOURCE_DIR}/sketchproject.cpp"
            "${CMAKE_SOURCE_DIR}/projecttoblenderanimation.cpp"
)
SET(SketchProject_MOC_Cpp
            ${WorldMgr_MOC_Cpp}
)

QT4_WRAP_CPP(SketchProject_MOC_Out ${SketchProject_MOC_Cpp})

SOURCE_GROUP("Generated" FILES ${SketchProject_MOC_Out})

SET(SketchProject_Deps
    ${SketchProject_Cpp}
    ${SketchProject_MOC_Out}
)

# End replace this when adding library

QT4_WRAP_CPP(MocTestSrc testqt.h)
SOURCE_GROUP("Generated" FILES ${MocTestSrc})
MESSAGE( STATUS "MocSRC: " ${MocTestSrc} )
ADD_LIBRARY(TestSubprocess testqt.cpp ${MocTestSrc} ${SketchProject_Deps})

SET(TestChimeraSrc testChimera.cc)
ADD_EXECUTABLE(TestChimera ${TestChimeraSrc})
TARGET_LINK_LIBRARIES(TestChimera ${SUBPROCESS_TEST_LINK_LIBRARIES})

ADD_TEST(TestingChimeraImport           TestChimera)
