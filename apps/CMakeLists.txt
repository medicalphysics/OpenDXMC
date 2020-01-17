
add_executable(opendxmc WIN32 opendxmc.cpp ../data/resources.rc)

vtk_module_autoinit(
    TARGETS opendxmc
    MODULES ${VTK_LIBRARIES}
    )

target_compile_definitions(opendxmc PRIVATE APP_VERSION="${PROJECT_VERSION}")

if (ENABLE_OSPRAY)
	target_compile_definitions(opendxmc PRIVATE ENABLE_OSPRAY)
endif(ENABLE_OSPRAY)

target_link_libraries(opendxmc 
	PRIVATE libopendxmc
	)

# On windows install we copy all dependent shared libraries and data to binary folder 
if (WIN32)
    set(OPENDXMC_DEPLOY "${CMAKE_CURRENT_BINARY_DIR}/opendxmcdeploy/")
    # Data folder
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/resources")

    # QT deploy on windows
    get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
    set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
	add_custom_command(TARGET opendxmc POST_BUILD
        #COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} --dir "${CMAKE_CURRENT_BINARY_DIR}/winqtdeploy/" "$<TARGET_FILE_DIR:opendxmc>/$<TARGET_FILE_NAME:opendxmc>"
        COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} --dir "${OPENDXMC_DEPLOY}" "$<TARGET_FILE_DIR:opendxmc>/$<TARGET_FILE_NAME:opendxmc>"
        )
    
    #VTK deploy windows
	##BUGFIX vtkRenderingVolumeOpenGL2-8.X is not copied (BUG), a quick fix here
	## vtksys is not a component and must be copied manually
	set(VTK_LIBRARIES_DEPLOY ${VTK_LIBRARIES} "vtkRenderingVolumeOpenGL2" "vtksys")
    foreach(vtkdll ${VTK_LIBRARIES_DEPLOY})	
		set(libname "${vtkdll}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
		string(REPLACE "VTK::" "vtk" libname ${libname})
		if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
			string(REPLACE ".dll" "d.dll" libname ${libname})
		endif()
		set(libpath "${VTK_DIR}/bin/${CMAKE_BUILD_TYPE}/${libname}")
		set(outpath "${OPENDXMC_DEPLOY}/${libname}")
		
		if(NOT vtkdll STREQUAL "VTK::sqlite") ##Ignores vtksqlite-X.X.dll to prevent cmake error in case vtk was not compiled with SQLite
			add_custom_command(TARGET opendxmc POST_BUILD
				COMMENT "Copying vtk dll ${libpath} to ${outpath}"
				DEPENDS ${libpath}
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${libpath}
				${outpath}
				)
		endif()
	endforeach()

	##adding gdcm dependency for vtkDICOM if installed. To use GDCM vtkDicom must be compiled with it.
	##this simply adds required dlls to buildfolder for GDCM 
	FIND_PACKAGE(GDCM)
	IF(GDCM_FOUND)
		INCLUDE(${GDCM_USE_FILE})
		get_filename_component(GDCM_MAIN_DIR "${GDCM_LIBRARY_DIRS}/.." ABSOLUTE)
		message(STATUS "Using GDCM library ${GDCM_MAIN_DIR}")
		set(libnames "gdcmMSFF" "gdcmDSED" "gdcmCommon" 
		"gdcmjpeg8" "gdcmDICT" "gdcmjpeg12" "gdcmIOD" 
		"gdcmjpeg16" "gdcmopenjp2" "gdcmzlib" 
		"gdcmcharls" "gdcmexpat" "gdcmmd5")
		foreach(libname ${libnames})
			set(libpath "${GDCM_MAIN_DIR}/bin/${libname}.dll")
			set(outpath "${OPENDXMC_DEPLOY}/${libname}.dll")
			add_custom_command(TARGET opendxmc POST_BUILD
				COMMENT "Copying GDCM dll ${libpath} to ${outpath}"
				DEPENDS ${libpath}
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${libpath}
				${outpath}
				)
		endforeach()
	ENDIF(GDCM_FOUND)

	## Deploy HDF5 libraries
	set(HDF5_BINARY_DIR "${HDF5_ROOT}/bin")
	set(HDF5_LIBNAMES "hdf5" "hdf5_cpp")
	foreach(libname ${HDF5_LIBNAMES})
		set(libpath "${HDF5_BINARY_DIR}/${libname}.dll")
		set(outpath "${OPENDXMC_DEPLOY}/${libname}.dll")
		add_custom_command(TARGET opendxmc POST_BUILD
			COMMENT "Copying HDF5 dll ${libpath} to ${outpath}"
			DEPENDS ${libpath}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${libpath}
			${outpath}
			)
	endforeach()

	#Install windows requirered dlls
    #install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/winqtdeploy/" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    install(DIRECTORY "${OPENDXMC_DEPLOY}" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

endif(WIN32)


