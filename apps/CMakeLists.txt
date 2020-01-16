add_executable(opendxmc opendxmc.cpp)

vtk_module_autoinit(
    TARGETS opendxmc
    MODULES ${VTK_LIBRARIES}
    )
#target_include_directories(opendxmc PRIVATE ../include)
target_link_libraries(opendxmc 
	PRIVATE opendxmclib
	PRIVATE Qt5::Widgets
	PRIVATE Qt5::Core
	PRIVATE Qt5::Gui
	PRIVATE ${VTK_LIBRARIES}
	)
