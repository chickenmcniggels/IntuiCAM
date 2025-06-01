# IntuiCAM Simulation Demo Examples

cmake_minimum_required(VERSION 3.20)

# Create simulation demo example
add_executable(simulation_demo 
    simulation_demo.cpp
)

target_link_libraries(simulation_demo
    intuicam_core
    intuicam_operations
    ${VTK_LIBRARIES}
)

# Set output directory
set_target_properties(simulation_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/simulation_demo
)

# Install example
install(TARGETS simulation_demo
    RUNTIME DESTINATION examples/simulation_demo
)

# Install example source files for reference
install(FILES 
    simulation_demo.cpp
    DESTINATION examples/simulation_demo/src
    OPTIONAL
)

# Install demo data files
install(FILES 
    demo_part.step
    demo_toolpath.json
    DESTINATION examples/simulation_demo/data
    OPTIONAL
) 