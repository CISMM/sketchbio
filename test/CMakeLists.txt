SET(Test1Srcs testTransformManager.cc "${CMAKE_SOURCE_DIR}/transformmanager.cpp")
ADD_EXECUTABLE( TransformTest ${Test1Srcs})
TARGET_LINK_LIBRARIES( TransformTest QVTK ${QUATLIB_LIBRARIES} )

SET(Test2Srcs testPQPDeform.cc "${CMAKE_SOURCE_DIR}/modelmanager.cpp"
            "${CMAKE_SOURCE_DIR}/sketchmodel.cpp")

ADD_EXECUTABLE( PQPTest ${Test2Srcs})
TARGET_LINK_LIBRARIES( PQPTest QVTK ${QUATLIB_LIBRARIES} PQP )


SET(Test3Srcs testReplicatedObjects.cc "${CMAKE_SOURCE_DIR}/structurereplicator.cpp"
            "${CMAKE_SOURCE_DIR}/worldmanager.cpp" "${CMAKE_SOURCE_DIR}/sketchobject.cpp"
            "${CMAKE_SOURCE_DIR}/sketchmodel.cpp" "${CMAKE_SOURCE_DIR}/modelmanager.cpp"
            "${CMAKE_SOURCE_DIR}/springconnection.cpp")
ADD_EXECUTABLE( ReplicatorTest ${Test3Srcs})
TARGET_LINK_LIBRARIES( ReplicatorTest QVTK ${QUATLIB_LIBRARIES} PQP )

ADD_TEST(RotateAboutTrackerTransformTest    TransformTest "4")
ADD_TEST(LeftTrackerTransformTest           TransformTest "6")
ADD_TEST(RightTrackerTransformTest          TransformTest "7")
