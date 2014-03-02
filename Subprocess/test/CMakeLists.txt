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

# create the necessary model files for the tests to run
FILE(COPY ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/models
)

FILE(GLOB test_project "${CMAKE_CURRENT_SOURCE_DIR}/resources/testproj/*")

add_custom_target(CopySubprocessTestProject ALL COMMENT
    "Copying subprocess test projects..." SOURCES ${test_project})


add_custom_command(TARGET CopySubprocessTestProject POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/testproj
    ${CMAKE_CURRENT_BINARY_DIR}/projects/animation)

FILE(GLOB mybpyhelpers "${CMAKE_SOURCE_DIR}/Export/scripts/mybpyhelpers.py")

add_custom_target(CopyBlenderPythonScriptForSubprocessTest ALL COMMENT
    "Copying helper scripts for Subprocess test")

add_custom_command(TARGET CopyBlenderPythonScriptForSubprocessTest POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/Export/scripts/mybpyhelpers.py
    ${CMAKE_CURRENT_BINARY_DIR}/scripts/mybpyhelpers.py)
add_custom_target(CopyChimeraExtensionForTests ALL COMMENT
"Copying chimera extension for tests...")
add_custom_command(TARGET CopyChimeraExtensionForTests POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension
    ${CMAKE_CURRENT_BINARY_DIR}/scripts)

make_subprocess_test( ChimeraVTKExportRunner testChimeraVTKExportRunner.cc)
make_subprocess_test( BlenderDecimationRunner testBlenderDecimationRunner.cc)
make_subprocess_test( ModelFromPdbRunner testModelFromPdbRunner.cc)
make_subprocess_test( BlenderAnimationRunner testBlenderAnimationRunner.cc)
