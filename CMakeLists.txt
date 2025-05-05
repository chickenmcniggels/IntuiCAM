cmake_minimum_required(VERSION 3.16)
project(IntuiCAM LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Add subdirectories
add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(tests)
