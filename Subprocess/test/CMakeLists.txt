SET(SUBPROCESS_TEST_LINK_LIBRARIES
    TestSubprocess
    SketchBioSubprocess
    SketchBioExport
    SketchBioCore
    SketchBioCustomVtkFilters
    ${PQP_LIBRARIES}
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
    SET(testname "SubprocessTest${name}")
    ADD_EXECUTABLE(${testname} ${source})
    TARGET_LINK_LIBRARIES(${testname} ${SUBPROCESS_TEST_LINK_LIBRARIES})
    ADD_TEST(${testname} ${testname})
endmacro (make_subprocess_test)


make_subprocess_test( ChimeraObjMaker testChimeraObjMaker.cc)
make_subprocess_test( BlenderDecimationRunner testBlenderDecimationRunner.cc)
make_subprocess_test( ModelFromPdbRunner testModelFromPdbRunner.cc)
