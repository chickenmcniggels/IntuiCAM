# IntuiCAM/core/CMakeLists.txt

# Add each core module as a subdirectory
add_subdirectory(common)
add_subdirectory(geometry)
add_subdirectory(toolpath)
add_subdirectory(postprocessor)
add_subdirectory(simulation)

# Comment out Python module while Python bindings are disabled
# if(INTUICAM_BUILD_PYTHON)
#     add_subdirectory(python)
# endif()
