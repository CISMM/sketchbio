# sources of custom vtk filters
set(vtkFilterSources
    vtkProjectToPlane.cxx
    vtkProjectToPlane.h
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# make the library
add_library(SketchBioCustomVtkFilters ${vtkFilterSources})

# tests for this library
add_subdirectory(test)
