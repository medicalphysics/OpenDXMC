cmake_minimum_required(VERSION 3.26)

project(OpenDXMC VERSION 1.2.4 DESCRIPTION "OpenDXMC description" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Let's nicely support folders in IDE's
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Testing only available if this is the main app
# Note this needs to be done in the main CMakeLists
# since it calls enable_testing, which must be in the
# main CMakeLists.
include(CTest)

# Docs only available if this is the main app
find_package(Doxygen)
if(Doxygen_FOUND)
	add_subdirectory(docs)
else()
	message(STATUS "Doxygen not found, not building docs")
endif()

#Settings to use DL organ segmentator
set(OPENDXMC_USECTSEGMENTATOR Off CACHE BOOL "Use DL organ segmentation for CT")

## Include QT
## QT specifics for CMake (https://doc.qt.io/qt-6/cmake-get-started.html)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)
qt_standard_project_setup()

# Adding VTK package
find_package(VTK REQUIRED COMPONENTS          
     CommonColor
     CommonCore
     DICOM
     GUISupportQt
     InteractionImage
     InteractionStyle
     ImagingGeneral
     ImagingStatistics
     IOExodus    
     jsoncpp
     RenderingAnnotation
     RenderingContextOpenGL2
     RenderingCore
     RenderingFreeType
     RenderingGL2PSOpenGL2
     RenderingImage
     RenderingOpenGL2
     RenderingVolume
     RenderingVolumeOpenGL2            
  )

#include HDF5
set(HDF5_USE_STATIC_LIBRARIES OFF)
find_package(HDF5 REQUIRED COMPONENTS CXX)
if(NOT HDF5_ROOT AND HDF5_FOUND)
    get_filename_component(HDF5_ROOT "${HDF5_INCLUDE_DIRS}/.." ABSOLUTE)
endif()

## Fetch packages
include(FetchContent)
## Adding DXMClib package
FetchContent_Declare(
    libdxmc
    GIT_REPOSITORY https://github.com/medicalphysics/DXMClib.git
    GIT_TAG develop
    )
FetchContent_MakeAvailable(libdxmc)

if(${OPENDXMC_USECTSEGMENTATOR})
    ## Adding CTSegmentator package
    FetchContent_Declare(
        libctsegmentator
        GIT_REPOSITORY https://github.com/medicalphysics/ctsegmentation.git
        GIT_TAG main
        )
    FetchContent_MakeAvailable(libctsegmentator)
endif()

# Build types 
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel") 
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "List of supported configurations.") 
mark_as_advanced(CMAKE_CONFIGURATION_TYPES) 
if(NOT CMAKE_BUILD_TYPE) 
	message("Defaulting to release build.") 
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}." FORCE) 
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${CMAKE_CONFIGURATION_TYPES}")

# Targets that we develop here
# The compiled library code is here
add_subdirectory(src/libopendxmc)

# The executable code is here
add_subdirectory(src/app)

# Add helper executable to convert ICRU adult phantoms to binary
add_subdirectory(utilities)

# Testing only available if this is the main app
# Emergency override MODERN_CMAKE_BUILD_TESTING provided as well
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()



