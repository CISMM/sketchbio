# Set the libraries to link the tests to
set( export_test_link_libraries
    SketchBioExport
    SketchBioCore
    SketchBioCustomVtkFilters
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    ${PQP_LIBRARIES}
)

# define a test-making macro
macro( make_export_test name source )
    set(testname "ExportTest${name}")
    add_executable( ${testname} ${source})
    target_link_libraries(${testname} ${export_test_link_libraries})
    add_test( ${testname} ${testname} )
endmacro( make_export_test )

# copy the files needed for the tests into place
FILE(COPY ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/models
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/project.xml"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test/test2
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1m1j.obj.decimated.0.1.obj"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test/test2
)
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1fzc.obj.decimated.0.1.obj"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test/test2
)

SOURCE_GROUP("Resources" FILES
    "${CMAKE_SOURCE_DIR}/models/1m1j.obj"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1m1j.obj.decimated.0.1.obj"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/1fzc.obj.decimated.0.1.obj"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/saveTest2/project.xml"
)

# make the tests
make_export_test( ProjectToXML testProjectToXML.cc )
