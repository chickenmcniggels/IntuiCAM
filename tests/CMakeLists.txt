cmake_minimum_required(VERSION 3.16)
project(IntuiCAMTests LANGUAGES CXX)

enable_testing()

# Core-logic test
# Commented out as requested - test_core.exe should not be built
# add_executable(test_core test_core.cpp)
# target_link_libraries(test_core PRIVATE IntuiCAMCore)
# add_test(NAME CoreTests COMMAND test_core)
