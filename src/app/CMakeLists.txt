

set(OPENDXMC_SOURCE_FILES 
    beamsettingswidget.cpp
	opendxmc.cpp
	basesettingswidget.cpp
	mainwindow.cpp
	ctdicomimportwidget.cpp
    icrpphantomimportwidget.cpp
    otherphantomimportwidget.cpp
    statusbar.cpp
)

#Adding icon resource for windows
if(WIN32)
    set(OPENDXMC_SOURCE_FILES ${OPENDXMC_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/data/icons/icon.rc)
endif()

qt_add_executable(opendxmc ${OPENDXMC_SOURCE_FILES})

# We expose headers for all targets consuming opendxmc
target_include_directories(opendxmc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(opendxmc PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_link_libraries(opendxmc 
	PRIVATE 
    libopendxmc
    Qt6::Widgets   
	)

set_target_properties(opendxmc PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# adding dxmc physicslist to binary folder
dxmclib_add_physics_list(opendxmc)

if(${OPENDXMC_USECTSEGMENTATOR})
    #adding models to binary folder
    copy_ctsegmentator_model_to_binary_dir(opendxmc)
    #adding torch binaries
    copy_torch_binaries(opendxmc)

    target_compile_definitions(opendxmc PRIVATE USECTSEGMENTATOR=1)
endif()

# Adding icons to binary resource
qt_add_resources(opendxmc icons
    PREFIX "/icons"
    BASE "${CMAKE_SOURCE_DIR}/data/icons"
    FILES 
    "${CMAKE_SOURCE_DIR}/data/icons/icon.png" 
    "${CMAKE_SOURCE_DIR}/data/icons/icon_fill.png"
    "${CMAKE_SOURCE_DIR}/data/icons/settings.png"
)

# adding phantoms to be available in binary folder
add_custom_command(
  TARGET opendxmc
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/data/phantoms" "$<TARGET_FILE_DIR:opendxmc>/data/phantoms"
  COMMENT "Copying ICRP phantoms to build folder"
)
# adding bowtiefilters to be available in binary folder
add_custom_command(
  TARGET opendxmc
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/data/bowtiefilters" "$<TARGET_FILE_DIR:opendxmc>/data/bowtiefilters"
  COMMENT "Copying bowtie filters to build folder"
)


# Install targets
include(InstallRequiredSystemLibraries)
install(TARGETS opendxmc
    EXPORT libopendxmc    
)

# Generate QT deployment script for the target opendxmc.
qt6_generate_deploy_app_script(
    TARGET opendxmc    
    OUTPUT_SCRIPT qt_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
# Call the deployment script on "cmake --install".
install(SCRIPT ${qt_deploy_script})

# Install data folder
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/phantoms" DESTINATION "${CMAKE_INSTALL_BINDIR}/data")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/bowtiefilters" DESTINATION "${CMAKE_INSTALL_BINDIR}/data")

if(WIN32)
    # Install dependent runtime
    install(FILES $<TARGET_RUNTIME_DLLS:opendxmc> TYPE BIN)
endif()


# Packaging
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Monte carlo dose application for diagnostic x-ray")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Erlend Andersen")
set(CPACK_PACKAGE_EXECUTABLES "opendxmc;OpenDXMC")
set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/data/icons/icon_fill.bmp")
set(CPACK_THREADS 0)
#set(CPACK_SOURCE_GENERATOR "TGZ")
if(WIN32)
    set(CPACK_NSIS_MODIFY_PATH "OFF")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\OpenDXMC.lnk' '$INSTDIR\\\\bin\\\\OpenDXMC.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\OpenDXMC.lnk'"
    )
    #Allow non UAC installs
    set(CPACK_NSIS_DEFINES "RequestExecutionLevel user") 
    
    set(CPACK_WIX_UPGRADE_GUID "aee28b68-bbb9-0f61-a782-3ab32ebdccae")
    set(CPACK_WIX_INSTALL_SCOPE "perUser")
    set(CPACK_WIX_VERSION "4")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "OpenDXMC")

    set(CPACK_INNOSETUP_ICON_FILE "${PROJECT_SOURCE_DIR}/data/icons/icon.ico")
    set(CPACK_INNOSETUP_SETUP_PrivilegesRequired "lowest")
    set(CPACK_INNOSETUP_SETUP_PrivilegesRequiredOverridesAllowed "commandline dialog")    
    set(CPACK_INNOSETUP_USE_MODERN_WIZARD ON)
    set(CPACK_INNOSETUP_CREATE_UNINSTALL_LINK ON)    
    set(CPACK_INNOSETUP_RUN_EXECUTABLES "{app}/bin/opendxmc")
endif()
include(CPack)


