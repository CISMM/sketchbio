# Set the libraries to link the tests to
set( vtk_test_link_libraries
    SketchBioCustomVtkFilters
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
)

# define a test-making macro
macro( make_vtk_test name source )
    add_executable(${name} ${source})
    target_link_libraries(${name} ${vtk_test_link_libraries})
    add_test( ${name} ${name} )
endmacro( make_vtk_test )

# make the tests
make_vtk_test( TestVTKProjectToPlane testProjectToPlane.cc )
