# IntuiCAM/core/postprocessor/CMakeLists.txt

set(CORE_POSTPROCESSOR_LIB_NAME intuicam_core_postprocessor)

file(GLOB_RECURSE CORE_POSTPROCESSOR_SOURCES
    "src/*.cpp"
    "src/*.h"
    "include/IntuiCAM/PostProcessor/*.h"
)

add_library(${CORE_POSTPROCESSOR_LIB_NAME} STATIC ${CORE_POSTPROCESSOR_SOURCES})

target_include_directories(${CORE_POSTPROCESSOR_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link against other core libraries from this project and OpenCASCADE
target_link_libraries(${CORE_POSTPROCESSOR_LIB_NAME} 
    PRIVATE
        intuicam_core_common
        ${OpenCASCADE_LIBRARIES}
    PUBLIC
        intuicam_core_toolpath
)

# Export the target for use by other CMake projects/modules if necessary
# install(TARGETS ${CORE_POSTPROCESSOR_LIB_NAME} EXPORT ${CORE_POSTPROCESSOR_LIB_NAME}_Targets ...)
# export(TARGETS ${CORE_POSTPROCESSOR_LIB_NAME} FILE ${CMAKE_BINARY_DIR}/cmake/${CORE_POSTPROCESSOR_LIB_NAME}.cmake) 