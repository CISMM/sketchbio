# Set the libraries these tests can depend on
set ( core_test_link_libraries
    CoreTestHelpers
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

ADD_LIBRARY(CoreTestHelpers
TestCoreHelpers.cxx
TestCoreHelpers.h
)

# create the tests
make_core_test( TransformManager TestTransformManager.cxx )
make_core_test( SketchModel TestSketchModel.cxx )
make_core_test( ModelManager TestModelManager.cxx )
make_core_test( ModelInstance TestModelInstance.cxx )
make_core_test( ObjectGroup TestObjectGroup.cxx )
make_core_test( StructureReplicator TestStructureReplicator.cxx )
make_core_test( TransformEquals TestTransformEquals.cxx )
make_core_test( WorldManager TestWorldManager.cxx )
make_core_test( SketchProject TestSketchProject.cxx )
make_core_test( Hand TestHand.cxx )
