# IntuiCAM Postprocessors
# Collection of G-code postprocessors for different machine types

cmake_minimum_required(VERSION 3.20)

# Add postprocessor libraries
add_subdirectory(lathe)
add_subdirectory(mill)
add_subdirectory(swiss)

# Create postprocessors interface library
add_library(intuicam_postprocessors INTERFACE)

target_include_directories(intuicam_postprocessors INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(intuicam_postprocessors INTERFACE
    intuicam_core
)

# Install rules
install(TARGETS intuicam_postprocessors
    EXPORT IntuiCAMTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/intuicam/plugins/postprocessors
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
) 