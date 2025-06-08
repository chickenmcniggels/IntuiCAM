# IntuiCAM/core/geometry/tests/CMakeLists.txt

find_package(GTest REQUIRED)

# Geometry module tests
add_executable(geometry_tests
    test_types.cpp
)

target_link_libraries(geometry_tests
    PRIVATE
        intuicam_core_geometry
        intuicam_core_common
        GTest::gtest
        GTest::gtest_main
        ${OpenCASCADE_LIBRARIES}
)

target_include_directories(geometry_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core/geometry/include
        ${CMAKE_SOURCE_DIR}/core/common/include
)

# Add test data directory
target_compile_definitions(geometry_tests
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(geometry_tests) 