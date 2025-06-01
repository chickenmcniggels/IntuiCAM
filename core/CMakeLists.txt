# IntuiCAM/core/CMakeLists.txt

# Add each core module as a subdirectory
add_subdirectory(common)
add_subdirectory(geometry)
add_subdirectory(toolpath)
add_subdirectory(postprocessor)
add_subdirectory(simulation)

# Create unified core library with a dummy source file
add_library(intuicam_core STATIC)

# Create a dummy source file for the unified library
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/intuicam_core_dummy.cpp 
    "// Dummy source file for unified IntuiCAM core library\n"
    "namespace IntuiCAM { namespace Core { int dummy_function() { return 0; } } }\n"
)

target_sources(intuicam_core PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/intuicam_core_dummy.cpp)

# Link all core modules to the unified library
target_link_libraries(intuicam_core PUBLIC
    intuicam_core_common
    intuicam_core_geometry
    intuicam_core_toolpath
    intuicam_core_postprocessor
    intuicam_core_simulation
)

# Add include directories
target_include_directories(intuicam_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/geometry/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/toolpath/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/postprocessor/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/simulation/include>
    $<INSTALL_INTERFACE:include>
)

# Add alias for consistent naming (support both intuicam_core and IntuiCAMCore)
add_library(IntuiCAMCore ALIAS intuicam_core)

# Add Python bindings if enabled
if(INTUICAM_BUILD_PYTHON)
    add_subdirectory(python)
endif()
