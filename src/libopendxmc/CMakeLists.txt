
qt_add_library(libopendxmc STATIC	
	basepipeline.cpp
	beamsettingsview.cpp
	beamsettingsmodel.cpp
	beamactorcontainer.cpp
	beamsettingsdelegate.cpp
	bowtiefilterreader.cpp
	datacontainer.cpp	
	ctimageimportpipeline.cpp
	ctorgansegmentatorpipeline.cpp
	ctsegmentationpipeline.cpp
	ctaecplot.cpp
	colormaps.cpp
	custominteractorstyleimage.cpp
	dosetablepipeline.cpp
	dosetablewidget.cpp
	dxmc_specialization.cpp
	hdf5wrapper.cpp
	h5io.cpp	
	icrpphantomimportpipeline.cpp
	renderwidgetscollection.cpp	
	slicerenderwidget.cpp		
	simulationpipeline.cpp
	simulationwidget.cpp
	volumerenderwidget.cpp
	volumerendersettingswidget.cpp
	volumelutwidget.cpp
	volumerendersettings.cpp
	)

# We expose headers for all targets consuming libopendxmc
target_include_directories(libopendxmc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Adding HDF5 headers internally for libopendxmc, maybe move to its own library?
target_include_directories(libopendxmc PRIVATE ${HDF5_INCLUDE_DIRS})
target_compile_definitions(libopendxmc PRIVATE ${HDF5_DEFINITIONS})

target_link_libraries(libopendxmc PUBLIC	
	Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Charts	
	libdxmc		
	VTK::CommonCore
	VTK::GUISupportQt
	VTK::DICOM
	VTK::RenderingVolumeOpenGL2
	VTK::ImagingStatistics
	VTK::IOExodus
	VTK::jsoncpp
	VTK::InteractionImage
	VTK::InteractionStyle
	VTK::ImagingGeneral
	VTK::CommonColor
	VTK::RenderingAnnotation
	PRIVATE	
	${HDF5_LIBRARIES}
	libctsegmentator
	)


# Rather than defining a single `VTK_DEFINITIONS` for use by all relevant
# targets, the definitions are made as needed with the exact set needed for the
# listed modules.
vtk_module_autoinit(
	TARGETS libopendxmc
	MODULES
	VTK::CommonCore
	VTK::GUISupportQt
	VTK::DICOM
	VTK::RenderingVolumeOpenGL2
	VTK::ImagingStatistics
	VTK::IOExodus	
	VTK::jsoncpp
	VTK::InteractionImage
	VTK::InteractionStyle
	VTK::ImagingGeneral
	VTK::CommonColor
	VTK::RenderingAnnotation
)