cmake_minimum_required(VERSION 3.16)
project(IntuiCAMTests LANGUAGES CXX)

# Enable testing
enable_testing()

# Find catch2 or gtest if available (optional, can be excluded if not using)
find_package(Catch2 QUIET)
if(Catch2_FOUND)
    include(Catch)
    set(TEST_FRAMEWORK_FOUND TRUE)
    message(STATUS "Using Catch2 for testing")
endif()

if(NOT TEST_FRAMEWORK_FOUND)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        include(GoogleTest)
        set(TEST_FRAMEWORK_FOUND TRUE)
        message(STATUS "Using Google Test for testing")
    endif()
endif()

# High-level test for core functionality
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE IntuiCAM::IntuiCAMCore)
target_include_directories(test_core PRIVATE ${CMAKE_SOURCE_DIR}/core/include)

# If test framework found, use it for advanced testing 
if(TEST_FRAMEWORK_FOUND)
    if(Catch2_FOUND)
        target_link_libraries(test_core PRIVATE Catch2::Catch2WithMain)
    elseif(GTest_FOUND)
        target_link_libraries(test_core PRIVATE GTest::gtest GTest::gtest_main)
    endif()
endif()

# Add test to CMake's testing system
add_test(NAME CoreTests COMMAND test_core)

# Additional low-level unit tests
file(GLOB_RECURSE UNIT_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp")
if(UNIT_TEST_SOURCES)
    add_executable(unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(unit_tests PRIVATE IntuiCAM::IntuiCAMCore)
    
    if(TEST_FRAMEWORK_FOUND)
        if(Catch2_FOUND)
            target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
            catch_discover_tests(unit_tests)
        elseif(GTest_FOUND)
            target_link_libraries(unit_tests PRIVATE GTest::gtest GTest::gtest_main)
            gtest_discover_tests(unit_tests)
        endif()
    endif()
    
    add_test(NAME UnitTests COMMAND unit_tests)
endif()
