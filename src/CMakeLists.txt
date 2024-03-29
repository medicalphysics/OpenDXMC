# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${OpenDXMC_SOURCE_DIR}/include/opendxmc/*.h")

add_library(libopendxmc STATIC
	mainwindow.cpp
    viewportwidget.cpp
    dicomimportwidget.cpp
	binaryimportwidget.cpp
	binaryimportpipeline.cpp
	imageimportpipeline.cpp
	progressindicator.cpp
	volumerenderwidget.cpp
	volumerenderwidgetsettings.cpp
	slicerenderinteractor.cpp
	slicerenderwidget.cpp
	simulationpipeline.cpp
	colormap.cpp
	materialselectionwidget.cpp
	volumeactorcontainer.cpp
	sourceeditorwidget.cpp
	phantomselectionwidget.cpp
	dosereportcontainer.cpp
	dosereportwidget.cpp
	sourceeditormodel.cpp
	exportwidget.cpp
	saveload.cpp
	stringmanipulation.cpp
	h5wrapper.cpp
	progresswidget.cpp
	phantomimportpipeline.cpp
	qpathmanipulation.cpp
	${HEADER_LIST}
	)

# We need this directory, and users of our library will need it too
target_include_directories(libopendxmc PUBLIC ../include)
target_include_directories(libopendxmc PUBLIC ${libdxmc_SOURCE_DIR}/include)
target_include_directories(libopendxmc PRIVATE ${HDF5_INCLUDE_DIRS})

target_compile_definitions(libopendxmc PRIVATE ${HDF5_DEFINITIONS})
if (ENABLE_OSPRAY)
	target_compile_definitions(libopendxmc PRIVATE ENABLE_OSPRAY)
endif()


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
