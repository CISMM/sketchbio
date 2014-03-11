# Set the libraries to link the tests to
set( export_test_link_libraries
    ExportTestHelpers
    CoreTestHelpers
    SketchBioExport
    SketchBioCore
    SketchBioCustomVtkFilters
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    ${PQP_LIBRARIES}
)

ADD_LIBRARY(ExportTestHelpers
CompareBeforeAndAfter.h
CompareBeforeAndAfter.cxx
MakeTestProject.h
MakeTestProject.cxx
)

# define a test-making macro
macro( make_export_test name source )
    set(testname "ExportTest${name}")
    add_executable( ${testname} ${source})
    target_link_libraries(${testname} ${export_test_link_libraries})
    add_test( ${testname} ${testname} )
endmacro( make_export_test )

# copy the files needed for the tests into place
FILE(GLOB test_project "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/*")

add_custom_target(CopyProjectForExportTests ALL COMMENT
    "Copying projects for save tests..."
    SOURCES ${test_project} ${model_files})

add_custom_command(TARGET CopyProjectForExportTests POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2
    ${CMAKE_CURRENT_BINARY_DIR}/test/test2)
# create the necessary model files for the tests to run
FILE(COPY ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/models
)

# make the tests
make_export_test( ProjectToXMLSave      TestProjectToXMLSave.cxx      )
make_export_test( ProjectToXMLCopyPaste TestProjectToXMLCopyPaste.cxx )
