
SET(Test1Srcs testTransformManager.cc)
ADD_EXECUTABLE( TransformTest ${Test1Srcs})
TARGET_LINK_LIBRARIES( TransformTest
    QVTK
    ${QUATLIB_LIBRARIES}
    SketchBioCore
)

SET(Test2Srcs testPQPDeform.cc )
ADD_EXECUTABLE( PQPTest ${Test2Srcs})
TARGET_LINK_LIBRARIES( PQPTest
    QVTK
    ${QUATLIB_LIBRARIES}
    PQP
    SketchBioCore
)


SET(Test3Srcs testReplicatedObjects.cc )
ADD_EXECUTABLE( ReplicatorTest ${Test3Srcs})
TARGET_LINK_LIBRARIES( ReplicatorTest
    QVTK
    ${QUATLIB_LIBRARIES}
    PQP
    SketchBioCore
)

# move files used by several tests to location where tests can find them
FILE(COPY ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    DESTINATION ${CMAKE_BINARY_DIR}/test/models
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/project.xml"
    DESTINATION ${CMAKE_BINARY_DIR}/test/test/test2
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1m1j.obj.decimated.0.1.obj"
    DESTINATION ${CMAKE_BINARY_DIR}/test/test/test2
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1fzc.obj.decimated.0.1.obj"
    DESTINATION ${CMAKE_BINARY_DIR}/test/test/test2
)

# this file is assumed to be present by several tests' code
SOURCE_GROUP("Resources" FILES
    ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1m1j.obj.decimated.0.1.obj
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1fzc.obj.decimated.0.1.obj
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/project.xml
)

SET(Test4Srcs testSaveToXML.cc )
ADD_EXECUTABLE( SaveTest ${Test4Srcs})
TARGET_LINK_LIBRARIES( SaveTest
    QVTK
    ${QUATLIB_LIBRARIES}
    PQP
    SketchBioCore
    SketchBioExport
)

SET(Test5Srcs testObjectGroups.cc)
ADD_EXECUTABLE( ObjectTest ${Test5Srcs})
TARGET_LINK_LIBRARIES( ObjectTest
    QVTK
    ${QUATLIB_LIBRARIES}
    PQP
    SketchBioCore
)

SET(Test6Srcs testTransformEquals.cc )
ADD_EXECUTABLE( TransformEqualsTest ${Test6Srcs})
TARGET_LINK_LIBRARIES( TransformEqualsTest
    QVTK
    ${QUATLIB_LIBRARIES}
    PQP
    SketchBioCore
)

SET(Test7Srcs testSketchModel.cc )
ADD_EXECUTABLE( ModelTest ${Test7Srcs})
TARGET_LINK_LIBRARIES( ModelTest
    QVTK
    ${QUATLIB_LIBRARIES}
    PQP
    SketchBioCore
)

ADD_TEST(RotateAboutTrackerTransformTest    TransformTest "4")
ADD_TEST(LeftTrackerTransformTest           TransformTest "6")
ADD_TEST(RightTrackerTransformTest          TransformTest "7")
ADD_TEST(ModelInstanceObjectGroupTest       ObjectTest)
ADD_TEST(TransformEqualsTest                TransformEqualsTest)
ADD_TEST(SaveLoadTest                       SaveTest)
ADD_TEST(SketchModelTest                    ModelTest)
