# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${OpenDXMC_SOURCE_DIR}/include/opendxmc/*.h")
#set(HEADER_LIST "${ModernCMakeExample_SOURCE_DIR}/include/modern/lib.hpp")

# Make an automatic library - will be static or dynamic based on user setting

add_library(opendxmclib
	mainwindow.cpp
    viewportwidget.cpp
    dicomimportwidget.cpp
	binaryimportwidget.cpp
	binaryimportpipeline.cpp
	imageimportpipeline.cpp
	progressindicator.cpp
	volumerenderwidget.cpp
	volumerenderwidgetsettings.cpp
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
	${HEADER_LIST}
	)

# We need this directory, and users of our library will need it too
target_include_directories(opendxmclib PUBLIC ../include)
target_include_directories(opendxmclib PUBLIC ${dxmclib_SOURCE_DIR}/include)
target_include_directories(opendxmclib PUBLIC ${HDF5_CXX_INCLUDE_DIRS})
# This depends on (header only) boost
target_link_libraries(opendxmclib
	PUBLIC libdxmc
	PUBLIC ${VTK_LIBRARIES}
    PUBLIC Qt5::Widgets
	PUBLIC Qt5::Core
	PUBLIC Qt5::Gui
	PUBLIC Qt5::Charts
	PUBLIC ${HDF5_CXX_LIBRARIES}
	PUBLIC ${HDF5_CXX_HL_LIBRARIES}
	)


# Adding vtk modules (replaces include(${VTK_USE_FILE}) since vtk 8.90) 
vtk_module_autoinit(
    TARGETS opendxmclib
    MODULES ${VTK_LIBRARIES}
    )

# All users of this library will need at least C++17
#target_compile_features(opendxmclib PUBLIC cxx_std_17)
