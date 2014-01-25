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

FILE(GLOB test_project "${CMAKE_CURRENT_SOURCE_DIR}/resources/testproj/*")
FILE(GLOB model_files "${CMAKE_SOURCE_DIR}/models/*")

add_custom_target(CopySubprocessTestModels ALL COMMENT
    "Copying subprocess test models..." SOURCES ${model_files})
add_custom_target(CopySubprocessTestProject ALL COMMENT
    "Copying subprocess test projects..." SOURCES ${test_project})


copyFilesIfDifferent("CopySubprocessTestModels"
    "${model_files}"
    "${CMAKE_CURRENT_BINARY_DIR}/models")
copyFilesIfDifferent("CopySubprocessTestProject"
    "${test_project}"
    "${CMAKE_CURRENT_BINARY_DIR}/projects/animation")

FILE(GLOB chimera_ext_files 
    "${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension/ExportVTK/*")
add_custom_target(CopyChimeraExtensionForTest ALL COMMENT
    "Copying Chimera extension for tests...")
add_custom_command(TARGET CopyChimeraExtensionForTest POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension/ExportVTK
    ${CMAKE_CURRENT_BINARY_DIR}/scripts/ExportVTK)


FILE(GLOB mybpyhelpers "${CMAKE_SOURCE_DIR}/Export/scripts/mybpyhelpers.py")

add_custom_target(CopyBlenderPythonScriptForSubprocessTest COMMENT
    "Copying bpy helper script for Subprocess test")

add_custom_command(TARGET CopyBlenderPythonScriptForSubprocessTest POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Export/scripts
    ${CMAKE_CURRENT_BINARY_DIR}/scripts)

make_subprocess_test( ChimeraVTKExportRunner testChimeraVTKExportRunner.cc)
make_subprocess_test( BlenderDecimationRunner testBlenderDecimationRunner.cc)
make_subprocess_test( ModelFromPdbRunner testModelFromPdbRunner.cc)
make_subprocess_test( BlenderAnimationRunner testBlenderAnimationRunner.cc)
