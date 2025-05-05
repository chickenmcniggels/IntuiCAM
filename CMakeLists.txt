cmake_minimum_required(VERSION 3.16)
project(IntuiCAM LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add modules
add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(tests)
