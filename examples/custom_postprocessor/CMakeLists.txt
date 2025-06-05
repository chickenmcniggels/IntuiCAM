# IntuiCAM Custom Postprocessor Examples

cmake_minimum_required(VERSION 3.20)

# Create custom postprocessor example
add_executable(custom_postprocessor_example 
    custom_postprocessor.cpp
)

target_link_libraries(custom_postprocessor_example
    intuicam_core
)

# Set output directory
set_target_properties(custom_postprocessor_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/custom_postprocessor
)

# Install example
install(TARGETS custom_postprocessor_example
    RUNTIME DESTINATION examples/custom_postprocessor
)

# Install example source files for reference
install(FILES 
    custom_postprocessor.cpp
    custom_postprocessor.h
    DESTINATION examples/custom_postprocessor/src
    OPTIONAL
) 