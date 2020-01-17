
add_executable(opendxmc WIN32 opendxmc.cpp)

vtk_module_autoinit(
    TARGETS opendxmc
    MODULES ${VTK_LIBRARIES}
    )
target_include_directories(opendxmc PUBLIC ../include)
target_include_directories(opendxmc PUBLIC ${HDF5_INCLUDE_DIRS})
target_compile_definitions(opendxmc PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_link_libraries(opendxmc 
	PRIVATE libopendxmc
	PRIVATE Qt5::Widgets
	PRIVATE Qt5::Core
	PRIVATE Qt5::Gui
	PRIVATE ${HDF5_LIBRARIES}
	)
