cmake_minimum_required(VERSION 3.16)
project(IntuiCAMCore LANGUAGES CXX)

# Determine library type (STATIC by default or based on BUILD_SHARED_LIBS)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
    add_definitions(-DINTUICAM_EXPORTS)
    if(WIN32)
        # Add export macros for Windows DLL
        add_definitions(-DINTUICAM_DLL)
    endif()
else()
    set(LIBRARY_TYPE STATIC)
endif()

# Find required OpenCASCADE packages
find_package(OpenCASCADE REQUIRED COMPONENTS 
    TKernel 
    TKMath 
    TKBRep 
    TKG3d 
    TKBO 
    TKTopAlgo 
    TKGeomBase 
    TKGeomAlgo
    TKSTEP
    TKSTEPBase
    TKSTEPAttr
    TKSTEP209
    TKShHealing
)

# Find source files
file(GLOB_RECURSE CORE_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# Create Core library target
add_library(IntuiCAMCore ${LIBRARY_TYPE}
    ${CORE_SRC}
    ${CORE_HEADERS}
)

# Configure include directories
target_include_directories(IntuiCAMCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/IntuiCAM>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# Link with OpenCASCADE
target_include_directories(IntuiCAMCore PUBLIC ${OpenCASCADE_INCLUDE_DIR})
target_link_directories(IntuiCAMCore PUBLIC ${OpenCASCADE_LIBRARY_DIR})
target_link_libraries(IntuiCAMCore
    PUBLIC
        ${OpenCASCADE_LIBRARIES}
)

# Export symbols with visibility controls
set_target_properties(IntuiCAMCore PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Create an alias target for use in other parts of the build
add_library(IntuiCAM::IntuiCAMCore ALIAS IntuiCAMCore)

# Install interface
if(NOT SKBUILD)
    # TODO: Install headers and export the target from the top-level CMakeLists.txt
endif()
