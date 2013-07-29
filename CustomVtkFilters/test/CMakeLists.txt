# Set the libraries to link the tests to
set( vtk_test_link_libraries
    SketchBioCustomVtkFilters
    ${VTK_LIBRARIES}
    ${QUATLIB_LIBRARIES}
)

# define a test-making macro
macro( make_vtk_test name source )
    set(testname "CustomVtkFiltersTest${name}")
    add_executable(${testname} ${source})
    target_link_libraries(${testname} ${vtk_test_link_libraries})
    add_test( ${testname} ${testname} )
endmacro( make_vtk_test )

# make the tests
make_vtk_test( ProjectToPlane testProjectToPlane.cc )
make_vtk_test( VRMLWriter     testVRMLWriter.cc     )
make_vtk_test( GiftRibbonSource testGiftRibbonSource.cc )
