cmake_minimum_required(VERSION 3.9)

#set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel CACHE TYPE INTERNAL FORCE)
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

project(OpenDXMC VERSION 0.0.1 DESCRIPTION "OpenDXMC description" LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)


# Build types 
set(CMAKE_CONFIGURATION_TYPES )



# Targets that we develop here
enable_testing()
add_subdirectory(dxmclib)
add_subdirectory(vtkextension)
add_subdirectory(opendxmc)



