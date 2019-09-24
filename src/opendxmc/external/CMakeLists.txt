

include(ExternalProject)
  ExternalProject_Add(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    TIMEOUT 5
    CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
    CONFIGURE_COMMAND "" # Disable configure step
    BUILD_COMMAND "" # Disable build step
    INSTALL_COMMAND "" # Disable install step
    UPDATE_COMMAND "" # Disable update step: clones the project only once
)
#ExternalProject_Get_Property(spdlog_external source_dir)
#set(spdlog_INCLUDE_DIR ${source_dir}/include)
## add the imported library which is 
##   an INTERFACE (ie header only)
##   IMPORTED (ie third party - no compilation necessary)
##   GLOBAL (you can reference it from everywhere 
##           even if the target is not in the same scope
##           which is the case for project1 and project2 because 
##           the spdlog  target is not in their parent folder)


ExternalProject_Get_Property(spdlog source_dir)
SET(spdlog_DIR ${source_dir})
add_subdirectory(${source_dir})
find_package(spdlog)


#include_directories(${source_dir}/include )
#find_package(spdlog PATHS ${source_dir})

#add_library(spdlog INTERFACE IMPORTED GLOBAL)
#add_dependencies(spdlog spdlog_external)
#ExternalProject_Get_Property(spdlog_external source_dir)
message(STATUS "spdlog source is located at: ${source_dir}")
## set the include directory which is to be associated with this target
## and which will be inherited by all targets which use 
## target_link_libraries(<target> spdlog)
#target_include_directories(spdlog INTERFACE ${source_dir})