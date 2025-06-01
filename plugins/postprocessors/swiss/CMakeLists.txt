# Swiss Postprocessor

add_library(intuicam_postprocessor_swiss 
    swiss_postprocessor.cpp
)

target_include_directories(intuicam_postprocessor_swiss PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(intuicam_postprocessor_swiss
    intuicam_core
)

# Install rules
install(TARGETS intuicam_postprocessor_swiss
    EXPORT IntuiCAMTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
) 