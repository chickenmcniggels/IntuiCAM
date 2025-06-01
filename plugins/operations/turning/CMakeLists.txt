# Turning Operations

add_library(intuicam_operation_turning 
    turning_operation.cpp
)

target_include_directories(intuicam_operation_turning PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(intuicam_operation_turning
    intuicam_core
)

# Install rules
install(TARGETS intuicam_operation_turning
    EXPORT IntuiCAMTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
) 