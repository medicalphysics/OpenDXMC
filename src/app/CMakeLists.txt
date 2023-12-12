

qt_add_executable(opendxmc 
    beamsettingswidget.cpp
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

# Adding base bowtiefilters to binary resource
qt_add_resources(opendxmc bowtiefilters
    PREFIX "/bowtiefilters"
    BASE "${CMAKE_SOURCE_DIR}/data/bowtiefilters"
    FILES 
    "${CMAKE_SOURCE_DIR}/data/bowtiefilters/bowtiefilters.json"     
)

# adding phantoms to be available in binary folder
add_custom_command(
  TARGET opendxmc
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/data/phantoms" "$<TARGET_FILE_DIR:opendxmc>/data/phantoms"
  COMMENT "Copying ICRP phantoms to build folder"
)

# Install data folder
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "bin/resources")

