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
)
SET(SketchProject_MOC_Cpp
            ${WorldMgr_MOC_Cpp}
)

QT4_WRAP_CPP(SketchObject_MOC_Out ${SketchObject_MOC_Cpp})
QT4_WRAP_CPP(WorldMgr_MOC_Out ${WorldMgr_MOC_Cpp})
QT4_WRAP_CPP(SketchProject_MOC_Out ${SketchProject_MOC_Cpp})

SOURCE_GROUP("Generated" FILES ${SketchProject_MOC_Out})

SET(SketchObject_Deps
    ${SketchObject_Cpp}
    ${SketchObject_MOC_Out}
)

SET(WorldMgr_Deps
    ${WorldMgr_Cpp}
    ${WorldMgr_MOC_Out}
)

SET(SketchProject_Deps
    ${SketchProject_Cpp}
    ${SketchProject_MOC_Out}
)

SET(Test1Srcs testTransformManager.cc "${CMAKE_SOURCE_DIR}/transformmanager.cpp")
ADD_EXECUTABLE( TransformTest ${Test1Srcs})
TARGET_LINK_LIBRARIES( TransformTest QVTK ${QUATLIB_LIBRARIES} )

SET(Test2Srcs testPQPDeform.cc "${CMAKE_SOURCE_DIR}/modelmanager.cpp"
            ${SketchObject_Deps})

ADD_EXECUTABLE( PQPTest ${Test2Srcs})
TARGET_LINK_LIBRARIES( PQPTest QVTK ${QUATLIB_LIBRARIES} PQP )


SET(Test3Srcs testReplicatedObjects.cc ${SketchProject_Deps} )
ADD_EXECUTABLE( ReplicatorTest ${Test3Srcs})
TARGET_LINK_LIBRARIES( ReplicatorTest QVTK ${QUATLIB_LIBRARIES} PQP )

# move files used by several tests to location where tests can find them
FILE(COPY ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    DESTINATION ${CMAKE_BINARY_DIR}/test/models
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/project.xml"
    DESTINATION ${CMAKE_BINARY_DIR}/test/test/test2
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/actin.obj.decimated.0.1.obj"
    DESTINATION ${CMAKE_BINARY_DIR}/test/test/test2
)

# this file is assumed to be present by several tests' code
SOURCE_GROUP("Resources" FILES
    ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/actin.obj.decimated.0.1.obj
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/project.xml
)

SET(Test4Srcs testSaveToXML.cc "${CMAKE_SOURCE_DIR}/projecttoxml.cpp" ${SketchProject_Deps} )
ADD_EXECUTABLE( SaveTest ${Test4Srcs})
TARGET_LINK_LIBRARIES( SaveTest QVTK ${QUATLIB_LIBRARIES} PQP )

SET(Test5Srcs testObjectGroups.cc ${SketchObject_Deps})
ADD_EXECUTABLE( ObjectTest ${Test5Srcs})
TARGET_LINK_LIBRARIES( ObjectTest QVTK ${QUATLIB_LIBRARIES} PQP )

SET(Test6Srcs testTransformEquals.cc "${CMAKE_SOURCE_DIR}/transformequals.cpp" ${SketchObject_Deps} )
ADD_EXECUTABLE( TransformEqualsTest ${Test6Srcs})
TARGET_LINK_LIBRARIES( TransformEqualsTest QVTK ${QUATLIB_LIBRARIES} PQP )

ADD_TEST(RotateAboutTrackerTransformTest    TransformTest "4")
ADD_TEST(LeftTrackerTransformTest           TransformTest "6")
ADD_TEST(RightTrackerTransformTest          TransformTest "7")
ADD_TEST(ModelInstanceObjectGroupTest       ObjectTest)
ADD_TEST(TransformEqualsTest                TransformEqualsTest)
ADD_TEST(SaveLoadTest                       SaveTest)  # breaks when run by CTest... maybe because it writes a file?
                                                    # this works in GDB and Valgrind doesn't find anything...
