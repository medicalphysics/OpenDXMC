
qt_add_library(libopendxmc STATIC	
    viewportwidget.cpp    
	)

# We expose headers for all targets consuming libopendxmc
target_include_directories(libopendxmc INTERFACE ${CMAKE_SOURCE_DIR}/include/libopendxmc)
# We expose headers internally for libopendxmc, maybe we should just use PUBLIC?
target_include_directories(libopendxmc PRIVATE ${CMAKE_SOURCE_DIR}/include/libopendxmc)
# Adding HDF5 headers internally for libopendxmc, maybe move to its own library?
target_include_directories(libopendxmc PRIVATE ${HDF5_INCLUDE_DIRS})
target_compile_definitions(libopendxmc PRIVATE ${HDF5_DEFINITIONS})

target_link_libraries(libopendxmc PUBLIC
	${VTK_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Charts
	PRIVATE
	${HDF5_LIBRARIES}
	libdxmc	
	)


# Adding vtk modules (replaces include(${VTK_USE_FILE}) since vtk 8.90) 
vtk_module_autoinit(
    TARGETS libopendxmc
    MODULES ${VTK_LIBRARIES}
    )
