

qt_add_executable(opendxmc 
	opendxmc.cpp
	basesettingswidget.cpp
	mainwindow.cpp
	ctdicomimportwidget.cpp
)

# We expose headers for all targets consuming opendxmc
target_include_directories(opendxmc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(opendxmc PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_link_libraries(opendxmc 
	PRIVATE libopendxmc
	)

set_target_properties(opendxmc PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

vtk_module_autoinit(
    TARGETS opendxmc
    MODULES ${VTK_LIBRARIES}
    )


# adding dxmc physicslist to binary folder
dxmclib_add_physics_list(opendxmc)

# Adding icons to binary resource
qt_add_resources(opendxmc icons
    PREFIX "/icons"
    BASE "${CMAKE_SOURCE_DIR}/data/icons"
    FILES 
    "${CMAKE_SOURCE_DIR}/data/icons/icon.png" 
    "${CMAKE_SOURCE_DIR}/data/icons/icon_fill.png"
    "${CMAKE_SOURCE_DIR}/data/icons/settings.png"
)

# Install data folder
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "bin/resources")

