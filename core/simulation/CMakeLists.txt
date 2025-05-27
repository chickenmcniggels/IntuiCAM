# IntuiCAM/core/simulation/CMakeLists.txt

set(CORE_SIMULATION_LIB_NAME intuicam_core_simulation)

file(GLOB_RECURSE CORE_SIMULATION_SOURCES
    "src/*.cpp"
    "src/*.h"
    "include/IntuiCAM/Simulation/*.h"
)

add_library(${CORE_SIMULATION_LIB_NAME} STATIC ${CORE_SIMULATION_SOURCES})

target_include_directories(${CORE_SIMULATION_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link against other core libraries from this project and OpenCASCADE
target_link_libraries(${CORE_SIMULATION_LIB_NAME} PRIVATE
    intuicam_core_common
    intuicam_core_geometry
    intuicam_core_toolpath
    ${OpenCASCADE_LIBRARIES}
)

# Link against necessary OpenCASCADE modules for simulation and visualization output
# Visualization group (if generating meshes for display):
set(OCCT_VISUALIZATION_LIBS
    TKMeshVS # For mesh visualization structures
    TKV3d    # Core 3D visualization
    TKService # Visualization services
    # Add other specific OCCT Visualization libraries if needed
)

target_link_libraries(${CORE_SIMULATION_LIB_NAME} PRIVATE
    ${OCCT_VISUALIZATION_LIBS}
    # It might also need TKMesh from ModelingAlgorithms if it performs meshing operations
    TKMesh
)

# Publicly link OCCT include dirs if its headers are used in our public headers
# target_include_directories(${CORE_SIMULATION_LIB_NAME} PUBLIC ${OpenCASCADE_INCLUDE_DIR}) 

# Export the target for use by other CMake projects/modules if necessary
# install(TARGETS ${CORE_SIMULATION_LIB_NAME} EXPORT ${CORE_SIMULATION_LIB_NAME}_Targets ...)
# export(TARGETS ${CORE_SIMULATION_LIB_NAME} FILE ${CMAKE_BINARY_DIR}/cmake/${CORE_SIMULATION_LIB_NAME}.cmake) 