
add_executable(opendxmc WIN32 opendxmc.cpp ../data/resources.rc)

vtk_module_autoinit(
    TARGETS opendxmc
    MODULES ${VTK_LIBRARIES}
    )
#target_include_directories(opendxmc PRIVATE ../include)
#target_include_directories(opendxmc PRIVATE ${HDF5_INCLUDE_DIRS})
target_compile_definitions(opendxmc PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_link_libraries(opendxmc 
	PRIVATE libopendxmc
	)
