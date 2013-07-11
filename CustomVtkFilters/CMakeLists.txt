# sources of custom vtk filters
set(vtkFilterSources
    vtkProjectToPlane.cxx
    vtkProjectToPlane.h
    vtkVRMLWriter.cxx
    vtkVRMLWriter.h
    )

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# make the library
add_library(SketchBioCustomVtkFilters ${vtkFilterSources})

# tests for this library
add_subdirectory(test)
