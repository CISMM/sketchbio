# Set the libraries these tests can depend on
set ( core_test_link_libraries
    SketchBioCore
    SketchBioCustomVtkFilters
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    ${PQP_LIBRARIES}
)

# a macro to create tests
macro( make_core_test name source )
    set(testname "CoreTest${name}")
    add_executable(${testname} ${source})
    target_link_libraries(${testname} ${core_test_link_libraries})
    add_test( ${testname} ${testname} )
endmacro( make_core_test )

# create the necessary model files for the tests to run
FILE(COPY ${CMAKE_SOURCE_DIR}/models/1m1j.obj
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/models
)

SOURCE_GROUP("Resources" FILES
    ${CMAKE_SOURCE_DIR}/models/1m1j.obj
)

# create the tests
make_core_test( TransformManager testTransformManager.cc )
make_core_test( SketchModel testSketchModel.cc )
make_core_test( SketchObject testSketchObject.cc )
make_core_test( StructureReplicator testStructureReplicator.cc )
make_core_test( TransformEquals testTransformEquals.cc )
