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

SOURCE_GROUP("Resources" FILES
    ${CMAKE_SOURCE_DIR}/models/1m1j.obj
)

FILE(COPY ${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension/ExportVTK/__init__.py
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/scripts/ExportVTK )
FILE(COPY ${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension/ExportVTK/ChimeraExtension.py
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/scripts/ExportVTK )

SOURCE_GROUP("Resources" FILES
    ${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension/ExportVTK/__init__.py
    ${CMAKE_SOURCE_DIR}/Subprocess/chimeraExtension/ExportVTK/ChimeraExtension.py
)

make_subprocess_test( ChimeraVTKExportRunner testChimeraVTKExportRunner.cc)
make_subprocess_test( BlenderDecimationRunner testBlenderDecimationRunner.cc)
make_subprocess_test( ModelFromPdbRunner testModelFromPdbRunner.cc)
