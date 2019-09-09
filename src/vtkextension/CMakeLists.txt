 project(vtkextension VERSION 0.0.1 DESCRIPTION "OpenDXMC extensions to VTK " LANGUAGES CXX)
 
 #Some other library that we import that was also built using CMake
# and has an exported target.
#set(VTK_DIR "C:/Users/ander/development/VTK/bin/VTK-8.1.1" CACHE FILEPATH "Path to VTK binary folder" )
set(VTK_DIR "C:/VTK/bin/VTK-8.1.2/" CACHE FILEPATH "Path to VTK binary folder" )
#set(VTK_DIR "/home/erlend/Development/VTK/bin/VTK-8.1.1/" CACHE FILEPATH "Path to VTK binary folder" )

find_package(VTK COMPONENTS
  vtkCommonColor
  vtkCommonComputationalGeometry
  vtkCommonCore
  vtkCommonDataModel
  vtkCommonExecutionModel
  vtkCommonMath
  vtkCommonMisc
  vtkCommonSystem
  vtkCommonTransforms
  vtkFiltersCore
  vtkFiltersGeneral
  vtkFiltersSources
  vtkGUISupportQt
  vtkImagingCore
  vtkImagingMath
  vtkInteractionStyle
  vtkInteractionImage
  vtkRenderingCore
  vtkRenderingImage
  vtkRenderingOpenGL2
  vtkRenderingQt
  vtkViewsQt
  vtkkwiml
  #vtkDICOM
  vtkRenderingVolume
  vtkRenderingVolumeOpenGL2
  )
include(${VTK_USE_FILE})
include_directories(${VTK_INCLUDE_DIRS})
include_directories(include)

add_library(vtkextension STATIC
    src/imagethresholdmultiple.cpp
	)

target_link_libraries(vtkextension
    ${VTK_LIBRARIES}
    )

target_include_directories(vtkextension PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    )
