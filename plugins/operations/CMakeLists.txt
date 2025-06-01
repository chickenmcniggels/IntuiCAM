# IntuiCAM Operations
# Collection of machining operations for CAM processing

cmake_minimum_required(VERSION 3.20)

# Add operation libraries
add_subdirectory(turning)
add_subdirectory(milling)
add_subdirectory(drilling)
add_subdirectory(threading)

# Create operations interface library
add_library(intuicam_operations INTERFACE)

target_include_directories(intuicam_operations INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(intuicam_operations INTERFACE
    intuicam_core
)

# Install rules
install(TARGETS intuicam_operations
    EXPORT IntuiCAMTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/intuicam/plugins/operations
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
) 