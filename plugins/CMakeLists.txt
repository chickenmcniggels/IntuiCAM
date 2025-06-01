# IntuiCAM/plugins/CMakeLists.txt

# Plugin system for IntuiCAM extensions

# Plugin interface library
add_library(intuicam_plugin_interface INTERFACE)

target_include_directories(intuicam_plugin_interface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Built-in plugins
add_subdirectory(postprocessors)
add_subdirectory(operations)

# Optional: External plugin loading support
option(INTUICAM_ENABLE_PLUGIN_LOADING "Enable dynamic plugin loading" ON)

if(INTUICAM_ENABLE_PLUGIN_LOADING)
    target_compile_definitions(intuicam_plugin_interface INTERFACE
        INTUICAM_PLUGIN_LOADING_ENABLED
    )
endif()

# Installation
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
) 