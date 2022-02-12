cmake_minimum_required(VERSION 3.14...3.16)

project(OpenDXMC VERSION 1.0.2 DESCRIPTION "OpenDXMC description" LANGUAGES CXX C)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" OFF)

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
      FiltersTexture
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
      RenderingUI
      ViewsQt
      glew
      zlib
      ImagingSources
      DICOM
      RenderingVolume
      RenderingAnnotation
      RenderingVolumeOpenGL2
      RenderingContext2D
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

option(ENABLE_OSPRAY "Build the OSPRay rendering backend. Requires OSPRay" OFF)
if(NOT ENABLE_OSPRAY)
	list(REMOVE_ITEM VTK_LIBRARIES "VTK::RenderingRayTracing" "VTK::RenderingSceneGraph" "VTK::jsoncpp")
endif()


## Include QT
## QT specifics for CMake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools Charts REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools Charts REQUIRED)

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
    GIT_TAG master
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
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${CMAKE_CONFIGURATION_TYPES}")

# Targets that we develop here
# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)

# Add helper executable to convert ICRU adult phantoms to binary
add_subdirectory(utilities)

# Testing only available if this is the main app
# Emergency override MODERN_CMAKE_BUILD_TESTING provided as well
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Install targets
include(InstallRequiredSystemLibraries)
install(TARGETS opendxmc
    EXPORT libopendxmc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
    PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin
    )

# Packaging
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Monte carlo dose application for diagnostic x-ray")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION_MAJOR}-${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VENDOR "Erlend Andersen")
include(CPack)

