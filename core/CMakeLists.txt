cmake_minimum_required(VERSION 3.16)
project(IntuiCAMCore LANGUAGES CXX)

# Locate the installed OCCT 7.9.0 package
find_package(OpenCASCADE CONFIG REQUIRED)


add_library(IntuiCAMCore STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/io/StepLoader.cpp
        # … your other source files …
)

target_include_directories(IntuiCAMCore
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/IntuiCAM>
        ${OpenCASCADE_INCLUDE_DIR}               # legacy include dir for headers
)


target_link_libraries(IntuiCAMCore
        PUBLIC
        ${OpenCASCADE_LIBRARIES}                # all OCCT modules
)

# optional alias
add_library(IntuiCAM::IntuiCAMCore ALIAS IntuiCAMCore)
