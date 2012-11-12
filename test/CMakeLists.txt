SET(Test1Srcs testTransformManager.cc "${CMAKE_SOURCE_DIR}/transformmanager.cpp")
ADD_EXECUTABLE( TransformTest ${Test1Srcs})
TARGET_LINK_LIBRARIES( TransformTest QVTK quat )

SET(Test2Srcs testPQPDeform.cc "${CMAKE_SOURCE_DIR}/modelmanager.cpp"
            "${CMAKE_SOURCE_DIR}/sketchmodel.cpp")
ADD_EXECUTABLE( PQPTest ${Test2Srcs})
TARGET_LINK_LIBRARIES( PQPTest QVTK quat PQP )
ADD_TEST(RotateAboutTrackerTransformTest    TransformTest "4")
ADD_TEST(LeftTrackerTransformTest           TransformTest "6")
ADD_TEST(RightTrackerTransformTest          TransformTest "7")
