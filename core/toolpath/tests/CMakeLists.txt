# IntuiCAM/core/toolpath/tests/CMakeLists.txt

find_package(GTest REQUIRED)

add_executable(toolpath_core_tests
    test_toolpath.cpp
    test_operation_factory.cpp
    test_operation_generation.cpp
)

target_link_libraries(toolpath_core_tests
    PRIVATE
        intuicam_core_toolpath
        intuicam_core_geometry
        intuicam_core_common
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(toolpath_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core/toolpath/include
        ${CMAKE_SOURCE_DIR}/core/geometry/include
        ${CMAKE_SOURCE_DIR}/core/common/include
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(toolpath_core_tests) 