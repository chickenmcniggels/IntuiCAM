# Mill Postprocessor

add_library(intuicam_postprocessor_mill 
    mill_postprocessor.cpp
)

target_include_directories(intuicam_postprocessor_mill PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(intuicam_postprocessor_mill
    intuicam_core
)

# Install rules
install(TARGETS intuicam_postprocessor_mill
    EXPORT IntuiCAMTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
) 