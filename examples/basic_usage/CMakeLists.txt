# IntuiCAM Basic Usage Examples

cmake_minimum_required(VERSION 3.20)

# Create simple turning example
add_executable(simple_turning simple_turning.cpp)

target_link_libraries(simple_turning
    intuicam_core
)

# Set output directory
set_target_properties(simple_turning PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/basic_usage
)

# Install example
install(TARGETS simple_turning
    RUNTIME DESTINATION examples/basic_usage
)

# Install example source files for reference
install(FILES simple_turning.cpp
    DESTINATION examples/basic_usage/src
) 