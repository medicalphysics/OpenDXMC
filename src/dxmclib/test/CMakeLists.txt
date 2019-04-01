
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/test)


message(STATUS "Testing is using xraylib library '${XRAYLIB_LIBRARY}'")
message(STATUS "Testing is using xraylib header files in '${XRAYLIB_INCLUDES}'")
message(STATUS )


add_executable(test_loadxraylib loadxraylib.cpp )
#include_directories(${XRAYLIB_INCLUDES})
target_include_directories(test_loadxraylib PRIVATE ${XRAYLIB_INCLUDES})
target_link_libraries(test_loadxraylib ${XRAYLIB_LIBRARY})
add_test(test_loadxraylib test_loadxraylib)
set_tests_properties(${test_loadxraylib}
    PROPERTIES
    PASS_REGULAR_EXPRESSION "Test passed")
set_tests_properties(${test_loadxraylib}
    PROPERTIES
    FAIL_REGULAR_EXPRESSION "(Exception|Test failed)")
set_tests_properties(${test_loadxraylib}
    PROPERTIES
    TIMEOUT 120)



add_executable(test_material test_material.cpp )
target_include_directories(test_material PRIVATE ${XRAYLIB_INCLUDES} ${OpenDXMClib_SOURCE_DIR}/include )
target_link_libraries(test_material ${XRAYLIB_LIBRARY} OpenDXMClib)
#include_directories(${XRAYLIB_INCLUDES} )


#
add_test(test_material test_material)
set_tests_properties(${test_material}
    PROPERTIES
    PASS_REGULAR_EXPRESSION "Test passed")
set_tests_properties(${test_material}
    PROPERTIES
    FAIL_REGULAR_EXPRESSION "(Exception|Test failed)")
set_tests_properties(${test_material}
    PROPERTIES
    TIMEOUT 120)
