# Set the dependencies for the tests
set( input_test_link_libraries
#    InputTestHelpers
    SketchBioInput
    ExportTestHelpers
    SketchBioExport
    CoreTestHelpers
    SketchBioCore
    SketchBioCustomVtkFilters
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
    ${VRPN_LIBRARIES}
    ${QUATLIB_LIBRARIES}
    ${PQP_LIBRARIES}
)

#ADD_LIBRARY( InputTestHelpers
#)

macro( make_input_test name source )
    set( testname "InputTest${name}" )
    add_executable( ${testname} ${source} )
    target_link_libraries( ${testname} ${input_test_link_libraries} )
    add_test( ${testname} ${testname} )
endmacro( make_input_test )

make_input_test( GlobalControls TestGlobalControls.cxx )
make_input_test( ColorFunctions TestColorFunctions.cxx )
make_input_test( AnimationFunctions TestAnimationFunctions.cxx )
make_input_test( TransformFunctions TestTransformFunctions.cxx )