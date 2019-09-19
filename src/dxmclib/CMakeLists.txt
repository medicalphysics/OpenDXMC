cmake_minimum_required(VERSION 3.9)
project(dxmclib VERSION 0.0.1 DESCRIPTION "OpenDXMClib" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Thirdparty libs
IF(WIN32)
    set(XRAYLIB_INSTALL_PATH "C:/Program Files/xraylib 64-bit" CACHE PATH "Install path for xraylib (https://github.com/tschoonj/xraylib)")
    find_library(XRAYLIB_LIBRARY NAMES libxrl libxrl-7 xrl libxrl-7.dll HINTS "${XRAYLIB_INSTALL_PATH}/Lib" "C:/Windows/System32")
    set(XRAYLIB_INCLUDES "${XRAYLIB_INSTALL_PATH}/Include")
	set(XRAYLIB_LIBRARY_SHARED_NAME "libxrl-7.dll")
	set(XRAYLIB_LIBRARY_SHARED_PATH "C:/Windows/sysnative/${XRAYLIB_LIBRARY_SHARED_NAME}")
	#set(XRAYLIB_LIBRARY_SHARED_PATH "C:\%windir%\Sysnative\${XRAYLIB_LIBRARY_SHARED_NAME}")
    #add_library(xraylib SHARED IMPORTED)
    #set_target_properties(xraylib PROPERTIES IMPORTED_LOCATION ${XRAYLIB_LIBRARY})
ENDIF(WIN32)
IF(UNIX AND NOT APPLE)
    set(XRAYLIB_INSTALL_PATH "/usr/local/lib" CACHE PATH "Install path for xraylib (https://github.com/tschoonj/xraylib)")
    find_library(XRAYLIB_LIBRARY NAMES libxrl libxrl-7 xrl HINTS "${XRAYLIB_INSTALL_PATH}")
    set(XRAYLIB_INCLUDES "/usr/local/include/xraylib")
ENDIF()



message(STATUS "Using xraylib library '${XRAYLIB_LIBRARY}'")
message(STATUS "Using xraylib header files in '${XRAYLIB_INCLUDES}'")
IF(WIN32)
MESSAGE(STATUS "Using xraylib shared library ${XRAYLIB_LIBRARY_SHARED_PATH}")
ENDIF()

include_directories(${XRAYLIB_INCLUDES})
include_directories(include)



add_library(dxmclib STATIC
    src/world.cpp
    src/source.cpp
    src/exposure.cpp
    src/tube.cpp
    src/dxmcrandom.cpp
    src/beamfilters.cpp
    src/material.cpp
    src/transport.cpp
	src/attenuationlut.cpp
	)

find_package(Threads REQUIRED) # for threading support

# Depend on a library that we defined in the top-level file
target_link_libraries(dxmclib
    PRIVATE ${XRAYLIB_LIBRARY}
    PRIVATE Threads::Threads # For threading support
    )


target_include_directories(dxmclib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src ${XRAYLIB_INCLUDES}
    )


# If we have compiler requirements for this library, list them
# here
target_compile_features(dxmclib
    PUBLIC cxx_auto_type cxx_std_17 cxx_constexpr cxx_generic_lambdas
    PRIVATE cxx_variadic_templates)

# add some of my favorite flags
#target_compile_options(dxmclib PRIVATE -W4)  #-Wall -Wextra
#target_compile_options(dxmclib PRIVATE ${OPENDXMCLIB_CXX_FLAGS})


# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS dxmclib EXPORT OpenDXMCConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
install(EXPORT OpenDXMCConfig DESTINATION share/OpenDXMC/cmake)

# This makes the project importable from the build directory
export(TARGETS dxmclib FILE OpenDXMCConfig.cmake)


if (WIN32)
	add_custom_target(xraylibtestdeploy ALL)
	set(outpath "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${XRAYLIB_LIBRARY_SHARED_NAME}")
	set(inpath "${XRAYLIB_LIBRARY_SHARED_PATH}")
	add_custom_command(TARGET xraylibtestdeploy POST_BUILD
		COMMENT "Copying ${XRAYLIB_LIBRARY_SHARED_NAME} from ${inpath} to ${outpath}"
		DEPENDS ${inpath}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		${inpath}
		${outpath}
	)
	#configure_file(${inpath} ${outpath} COPYONLY)
endif(WIN32)


# Every library has unit tests, of course
add_executable(testloadxraylib
    test/loadxraylib.cpp)
target_include_directories(testloadxraylib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testloadxraylib
    dxmclib)
add_test(testloadxraylib testloadxraylib)

add_executable(testattenuationlut
    test/attenuationlut.cpp)
target_include_directories(testattenuationlut PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testattenuationlut
    dxmclib)
add_test(testattenuationlut testattenuationlut)


add_executable(testtube
    test/testtube.cpp)
target_include_directories(testtube PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testtube
    dxmclib)
add_test(testtube testtube)

add_executable(testrandom
    test/testrandom.cpp)
target_include_directories(testrandom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testrandom
    dxmclib)
add_test(testrandom testrandom)

add_executable(testsource
    test/testsource.cpp)
target_include_directories(testsource PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testsource
    dxmclib)
add_test(testsource testsource)

add_executable(testmaterial
    test/testmaterial.cpp)
target_include_directories(testmaterial PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testmaterial
    dxmclib)
add_test(testmaterial testmaterial)

add_executable(testworld
    test/testworld.cpp)
target_include_directories(testworld PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testworld
    dxmclib)
add_test(testworld testworld)

add_executable(testrunner
    test/testrunner.cpp)
target_include_directories(testrunner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testrunner
    dxmclib)
add_test(testrunner testrunner)

add_executable(testdxmclib
    test/testdxmclib.cpp)
target_include_directories(testdxmclib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(testdxmclib
    dxmclib)
add_test(testdxmclib testdxmclib)

add_executable(validatedxmclib
    test/validatedxmclib.cpp)
target_include_directories(validatedxmclib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(validatedxmclib
    dxmclib)
add_test(validatedxmclib validatedxmclib)


add_executable(benchmark
    test/benchmark.cpp)
target_include_directories(benchmark PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_link_libraries(benchmark
    dxmclib)
add_test(benchmark benchmark)
