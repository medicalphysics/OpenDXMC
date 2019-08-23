


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
  vtkDICOM
  vtkRenderingVolume
  vtkRenderingVolumeOpenGL2
  )
include(${VTK_USE_FILE})
include_directories(${VTK_INCLUDE_DIRS})


## QT specifics for CMake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Charts )
find_package(Qt5Charts)
IF(UNIX AND NOT APPLE)
    find_package(Qt5 REQUIRED COMPONENTS X11Extras )
ENDIF()
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Charts_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

## spdlog Logging header only
include_directories("thirdparty/spdlog/*")


# Define an executable (WIN32 argument to prevent console window on windows)
add_executable(OpenDXMC WIN32
	resources/resources.rc
    src/app.cpp
	src/mainwindow.h
	src/mainwindow.cpp
    src/viewportwidget.cpp
    src/viewportwidget.h
    src/dicomimportwidget.h
    src/dicomimportwidget.cpp
	src/imageimportpipeline.h
	src/imageimportpipeline.cpp
	src/progressindicator.h
	src/progressindicator.cpp
	src/volumerenderwidget.cpp
	src/volumerenderwidget.h
	src/volumerenderwidgetsettings.cpp
	src/volumerenderwidgetsettings.h
	src/slicerenderwidget.h
	src/slicerenderwidget.cpp
	src/simulationpipeline.h
	src/simulationpipeline.cpp
	src/imagecontainer.h
	src/colormap.h
	src/colormap.cpp
	src/materialselectionwidget.cpp
	src/materialselectionwidget.h
	src/volumeactorcontainer.cpp
	src/volumeactorcontainer.h
	src/sourceeditorwidget.cpp
	src/sourceeditorwidget.h
	src/phantomselectionwidget.cpp
	src/phantomselectionwidget.h
	src/dosereportcontainer.h
	src/dosereportcontainer.cpp
	src/dosereportwidget.h
	src/dosereportwidget.cpp
	src/sourceeditormodel.h
	src/sourceeditormodel.cpp
	src/exportwidget.h
	src/exportwidget.cpp
    )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/opendxmc")

# Define the libraries this project depends upon
target_link_libraries(OpenDXMC
        PRIVATE dxmclib
		PRIVATE vtkextension
		${VTK_LIBRARIES}
        Qt5::Widgets
		Qt5::Core
		Qt5::Gui
		Qt5::Charts
        )

target_include_directories(OpenDXMC PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty>
    )

#making version available in code

target_compile_definitions(OpenDXMC PRIVATE APP_VERSION="${PROJECT_VERSION}")




##adding REQUIRED dlls to the binary folder of executable
#adding target to copy qt dlls 
if (WIN32)
    get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
    set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
	add_custom_target(qtdeploy ALL
       COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} --qmldir ${CMAKE_BINARY_DIR} $<TARGET_FILE_DIR:OpenDXMC>)
endif(WIN32)

#adding target to copy vtk dlls 
if (WIN32)
	add_custom_target(vtkdeploy ALL)
	foreach(vtkdll ${VTK_LIBRARIES})
		set(libname "${vtkdll}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
		set(libpath "${VTK_RUNTIME_LIBRARY_DIRS}/${CMAKE_BUILD_TYPE}/${libname}")
		set(outpath "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${libname}")
		if(NOT vtkdll STREQUAL "vtksqlite") ##Ignores vtksqlite-X.X.dll to prevent cmake error in case vtk was not compiled with SQLite
			add_custom_command(TARGET vtkdeploy POST_BUILD
				COMMENT "Copying vtk dll ${libpath} to ${outpath}"
				DEPENDS ${libpath}
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${libpath}
				${outpath}
				)
		endif()
	endforeach()
	
	## vtkRenderingVolumeOpenGL2-8.1 is not copied (BUG) a quick fix here
	add_custom_target(vtkdeployBUGFIX ALL)
	set(vtkdll "vtkRenderingVolumeOpenGL2")
	set(libname "${vtkdll}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
	set(libpath "${VTK_RUNTIME_LIBRARY_DIRS}/${CMAKE_BUILD_TYPE}/${libname}")
	set(outpath "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${libname}")
	add_custom_command(TARGET vtkdeployBUGFIX POST_BUILD
		COMMENT "Copying vtk dll ${libpath} to ${outpath}"
		DEPENDS ${libpath}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${libpath}
		${outpath}
		)
endif(WIN32)


#adding target to copy xraylib dlls 
if (WIN32)
	add_custom_target(xraylibdeploy ALL)
	set(XRAYLIB_LIBRARY_SHARED_NAME "libxrl-7.dll")
	set(XRAYLIB_LIBRARY_SHARED_PATH "C:/Windows/sysnative/${XRAYLIB_LIBRARY_SHARED_NAME}")
	set(outpath "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${XRAYLIB_LIBRARY_SHARED_NAME}")
	set(inpath "${XRAYLIB_LIBRARY_SHARED_PATH}")
	add_custom_command(TARGET xraylibdeploy POST_BUILD
		COMMENT "Copying ${XRAYLIB_LIBRARY_SHARED_NAME} from ${inpath} to ${outpath}"
		DEPENDS ${inpath}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		${inpath}
		${outpath}
	)
	#configure_file(${inpath} ${outpath} COPYONLY)
endif(WIN32)




#Copying application resources
add_custom_target(addresources ALL)
set (fileresources 
	"Human.vtp" 
	"phantoms/icrp/AF/AF.dat" "phantoms/icrp/AF/AF_organs.dat" "phantoms/icrp/AF/AF_media.dat"
	"phantoms/icrp/AM/AM.dat" "phantoms/icrp/AM/AM_organs.dat" "phantoms/icrp/AM/AM_media.dat"
	"phantoms/icrp/README.txt"
	"bowtiefilters.json"
	"icons/settings.png"
	"icons/settings.svg"
	"icons/icon.png"
	)
foreach(fileresource ${fileresources})
	set(outpath "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/resources/${fileresource}")
	set(inpath "${CMAKE_CURRENT_SOURCE_DIR}/resources/${fileresource}")
	add_custom_command(TARGET addresources POST_BUILD
		COMMENT "Copying file ${inpath} to ${outpath}."
		DEPENDS ${inpath}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		${inpath}
		${outpath}
	)
endforeach()

