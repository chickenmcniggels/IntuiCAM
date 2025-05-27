# IntuiCAM/cli/CMakeLists.txt

set(CLI_EXECUTABLE_NAME IntuiCAMCli)

# Add source files for the CLI
set(CLI_SOURCES
    src/main.cpp
    # Add other .cpp files from cli/src if any
)

add_executable(${CLI_EXECUTABLE_NAME} ${CLI_SOURCES})

# Include directories for the CLI module itself
target_include_directories(${CLI_EXECUTABLE_NAME} PRIVATE src)

# Link against OpenCASCADE modules (similar to what core modules might need for processing)
# Adjust this list based on what the CLI directly interacts with from OCCT
set(OCCT_CLI_LIBS
    TKernel      # Core OCCT kernel
    TKMath       # Mathematical functions
    TKTopAlgo    # Topological algorithms
    TKGeomAlgo   # Geometric algorithms
    TKBRep       # Boundary representation
    TKGeomBase   # Basic geometric functions
    # Add other OCCT toolkits as needed
)
target_link_libraries(${CLI_EXECUTABLE_NAME} PRIVATE ${OCCT_CLI_LIBS})

# Link against our static core libraries
target_link_libraries(${CLI_EXECUTABLE_NAME} PRIVATE
    intuicam_core_common
    intuicam_core_geometry
    intuicam_core_toolpath
    intuicam_core_postprocessor
    # intuicam_core_simulation # If CLI uses simulation features
)

# OpenCASCADE flags are inherited through core libraries
# No direct OpenCASCADE flags needed for CLI 