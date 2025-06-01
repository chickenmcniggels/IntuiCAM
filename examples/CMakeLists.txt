# IntuiCAM/examples/CMakeLists.txt

# Example projects and usage demos

# Basic usage examples
add_subdirectory(basic_usage)
add_subdirectory(python_scripting)
add_subdirectory(freecad_integration)

# Advanced examples
add_subdirectory(custom_postprocessor)
add_subdirectory(simulation_demo)

# Sample data
install(DIRECTORY sample_data/
    DESTINATION examples/sample_data
    FILES_MATCHING 
    PATTERN "*.step"
    PATTERN "*.stp"
    PATTERN "*.iges"
    PATTERN "*.igs"
) 