SET(SUBPROCESS_TEST_LINK_LIBRARIES
    TestSubprocess
    SketchBioSubprocess
    SketchBioExport
    SketchBioCore
    PQP
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIRECTORY}
    ${CMAKE_CURRENT_BINARY_DIRECTORY}
)

QT4_WRAP_CPP(MocTestSrc testqt.h)
SOURCE_GROUP("Generated" FILES ${MocTestSrc})
MESSAGE( STATUS "MocSRC: " ${MocTestSrc} )
ADD_LIBRARY(TestSubprocess testqt.cpp ${MocTestSrc})

macro (make_subprocess_test name source)
    ADD_EXECUTABLE(${name} ${source})
    TARGET_LINK_LIBRARIES(${name} ${SUBPROCESS_TEST_LINK_LIBRARIES})
    ADD_TEST(${name} ${name})
endmacro (make_subprocess_test)


make_subprocess_test(TestChimera testChimera.cc)
make_subprocess_test(TestBlenderDecimate testBlenderDecimate.cc)
make_subprocess_test(TestLoadFromPDB testModelFromPDB.cc)
