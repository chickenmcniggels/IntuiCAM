# IntuiCAM/core/common/CMakeLists.txt

set(CORE_COMMON_LIB_NAME intuicam_core_common)

file(GLOB_RECURSE CORE_COMMON_SOURCES
    "src/*.cpp"
    "src/*.h"
    "include/IntuiCAM/Common/*.h"
)

add_library(${CORE_COMMON_LIB_NAME} STATIC ${CORE_COMMON_SOURCES})

target_include_directories(${CORE_COMMON_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        src
)

# Link against OpenCASCADE libraries
target_link_libraries(${CORE_COMMON_LIB_NAME} PRIVATE
    ${OpenCASCADE_LIBRARIES}
)

# Test executable temporarily removed due to missing dependencies

# Export the target for use by other CMake projects/modules if necessary
# install(TARGETS ${CORE_COMMON_LIB_NAME} EXPORT ${CORE_COMMON_LIB_NAME}_Targets ...)
# export(TARGETS ${CORE_COMMON_LIB_NAME} FILE ${CMAKE_BINARY_DIR}/cmake/${CORE_COMMON_LIB_NAME}.cmake) 