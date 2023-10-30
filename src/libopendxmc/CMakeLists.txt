
qt_add_library(libopendxmc STATIC	
	basepipeline.cpp
	datacontainer.cpp
	ctimageimportpipeline.cpp
	slicerenderwidget.cpp
	custominteractorstyleimage.hpp
	)

# We expose headers for all targets consuming libopendxmc
target_include_directories(libopendxmc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Adding HDF5 headers internally for libopendxmc, maybe move to its own library?
target_include_directories(libopendxmc PRIVATE ${HDF5_INCLUDE_DIRS})
target_compile_definitions(libopendxmc PRIVATE ${HDF5_DEFINITIONS})

target_link_libraries(libopendxmc PUBLIC
	${VTK_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Charts	
	libdxmc
	PRIVATE
	${HDF5_LIBRARIES}		
	)

vtk_module_autoinit(
    TARGETS libopendxmc
    MODULES ${VTK_LIBRARIES}
    )
