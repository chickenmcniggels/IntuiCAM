cmake_minimum_required(VERSION 3.16)
project(IntuiCAMCore LANGUAGES CXX)

# Find OpenCASCADE (OCCT) 7.9.0 via vcpkg
find_package(OpenCASCADE 7.9.0 REQUIRED CONFIG)        # Provided by vcpkg’s opencascade port
message(STATUS "Found OpenCASCADE ${OpenCASCADE_VERSION}")

# Define the core library target
add_library(IntuiCAMCore STATIC  # or SHARED, depending on your needs
        src/Core.cpp
        # ... list your core .cpp sources
        # (Headers can be listed for clarity or installation if needed)
)

# Link OpenCASCADE to the core library
target_link_libraries(IntuiCAMCore PUBLIC ${OpenCASCADE_LIBRARIES})
# If using imported targets (OCCT defines targets like TKernel, TKGeomBase, etc.), you could do for example:
# target_link_libraries(IntuiCAMCore PUBLIC TKernel TKGeomBase TKMath TKG2d TKG3d ...)
# The PUBLIC scope makes OCCT transitively available to consumers of IntuiCAMCore (e.g., the GUI module).

# Include directories for OCCT (if not auto-added by imported targets)
target_include_directories(IntuiCAMCore PUBLIC ${OpenCASCADE_INCLUDE_DIR})

# Ensure the library’s public headers are known (if any)
target_include_directories(IntuiCAMCore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>    # headers for use during build
        $<INSTALL_INTERFACE:include/IntuiCAM>                     # headers for use after install
)

# Set any necessary compile definitions or options for OCCT (if required)
# e.g., OCCT may need certain definitions like -DOCC_CONVERT_SIGNALS (only if needed by OCCT usage)

# (Optional) Create an alias target for easier referencing (modern CMake practice)
add_library(IntuiCAM::IntuiCAMCore ALIAS IntuiCAMCore)

# Installation for IntuiCAMCore (handled in top-level for simplicity or can be done here)
# install(TARGETS IntuiCAMCore EXPORT IntuiCAMTargets ...)
# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/IntuiCAM)
