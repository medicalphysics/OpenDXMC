

#set(XRAYLIB_INSTALL_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Install path for xraylib (https://github.com/tschoonj/xraylib)")

set(XRAYLIB_INSTALL_PATH "C:/Program Files/xraylib 64-bit" CACHE PATH "Install path for xraylib (https://github.com/tschoonj/xraylib)")
find_library(XRAYLIB_LIBRARY libxrl-7 "${XRAYLIB_INSTALL_PATH}/Lib")

set(XRAYLIB_INCLUDES "${XRAYLIB_INSTALL_PATH}/Include")


add_library(xraylib SHARED IMPORTED)
set_target_properties(xraylib PROPERTIES IMPORTED_LOCATION ${XRAYLIB_LIBRARY})

message(STATUS "Using xraylib library '${XRAYLIB_LIBRARY}'")
message(STATUS "Using xraylib header files in '${XRAYLIB_INCLUDES}'")

