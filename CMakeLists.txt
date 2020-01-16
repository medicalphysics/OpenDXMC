cmake_minimum_required(VERSION 3.14...3.16)

project(OpenDXMC VERSION 0.1.10 DESCRIPTION "OpenDXMC description" LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

set(CMAKE_CXX_STANDARD 17)
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


# Adding VTK package
##adding VTK dependency
find_package(VTK COMPONENTS
      CommonColor
      CommonComputationalGeometry
      CommonCore
      CommonDataModel
      CommonExecutionModel
      CommonMath
      CommonMisc
      CommonSystem
      CommonTransforms
      FiltersCore
      FiltersGeneral
      FiltersSources
      FiltersGeometry
      FiltersStatistics
      FiltersModeling
      FiltersHybrid
      ParallelDIY
      ParallelCore
      GUISupportQt
      ImagingCore
      ImagingMath
      ImagingGeneral
      ImagingHybrid
      ImagingFourier
      InteractionStyle
      InteractionImage
      RenderingCore
      RenderingImage
      RenderingOpenGL2
      RenderingQt
      ViewsQt
      glew
      zlib
      ImagingSources
      DICOM
      RenderingVolume
      RenderingAnnotation
      RenderingVolumeOpenGL2
      IOCore
      IOMovie
      IOXML
      IOXMLParser
      IOImage
      IOLegacy
      FiltersExtraction
      ImagingColor
      InteractionImage
      InteractionWidgets
      RenderingFreeType
      DICOMParser
      jpeg
      pugixml
      freetype
      metaio
      png
      tiff
      loguru
      doubleconversion
      lz4
      lzma
      expat
      OPTIONAL_COMPONENTS
      RenderingRayTracing
      RenderingSceneGraph
      jsoncpp
  )

## Include QT
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

#include HDF5
find_package(HDF5 REQUIRED COMPONENTS CXX)
include_directories(${HDF5_CXX_INCLUDE_DIRS})

## Fetch packages
include(FetchContent)
## Adding DXMClib package
FetchContent_Declare(
    libdxmc
    GIT_REPOSITORY https://github.com/medicalphysics/DXMClib.git
    )
FetchContent_MakeAvailable(libdxmc)

# Build types 
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel") 
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "List of supported configurations.") 
mark_as_advanced(CMAKE_CONFIGURATION_TYPES) 
if(NOT CMAKE_BUILD_TYPE) 
	message("Defaulting to release build.") 
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}." FORCE) 
endif()


# Targets that we develop here
# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)

# Testing only available if this is the main app
# Emergency override MODERN_CMAKE_BUILD_TESTING provided as well
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

