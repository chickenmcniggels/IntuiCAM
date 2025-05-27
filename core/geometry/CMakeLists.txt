# IntuiCAM/core/geometry/CMakeLists.txt

set(CORE_GEOMETRY_LIB_NAME intuicam_core_geometry)

file(GLOB_RECURSE CORE_GEOMETRY_SOURCES
    "src/*.cpp"
    "src/*.h"
    "include/IntuiCAM/Geometry/*.h"
)

add_library(${CORE_GEOMETRY_LIB_NAME} STATIC ${CORE_GEOMETRY_SOURCES})

target_include_directories(${CORE_GEOMETRY_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link against the common library from this project
target_link_libraries(${CORE_GEOMETRY_LIB_NAME} PRIVATE
    intuicam_core_common
    ${OpenCASCADE_LIBRARIES}
)

# Export the target for use by other CMake projects/modules if necessary
# install(TARGETS ${CORE_GEOMETRY_LIB_NAME} EXPORT ${CORE_GEOMETRY_LIB_NAME}_Targets ...)
# export(TARGETS ${CORE_GEOMETRY_LIB_NAME} FILE ${CMAKE_BINARY_DIR}/cmake/${CORE_GEOMETRY_LIB_NAME}.cmake) 