cmake_minimum_required(VERSION 3.16)
project(IntuiCAM VERSION 1.0 LANGUAGES CXX)

# Enable C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt6 Core to get common CMake APIs (e.g. qt_standard_project_setup)
find_package(Qt6 REQUIRED COMPONENTS Core)         # Qt6.9.0 or later is assumed
qt_standard_project_setup()                        # Enable Qt auto-moc/uic, etc.:contentReference[oaicite:0]{index=0}

# Optionally, specify Qt version requirements (uncomment if needed)
# qt_standard_project_setup(REQUIRES 6.5)          # Ensure at least Qt 6.5 for new behaviors

# Add subdirectories for modules
add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(tests)

# Installation of targets (library and executable)
include(GNUInstallDirs)  # Ensures standard install directories (bin, lib, etc.)

# Install IntuiCAMCore library (if built as shared or for distribution)
install(TARGETS IntuiCAMCore
        EXPORT IntuiCAMTargets                       # export for package config
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  # .dll/.so
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # .lib/.a
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # .dll on Windows
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install IntuiCAM_GUI executable
install(TARGETS IntuiCAM_GUI
        EXPORT IntuiCAMTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # .exe or binary
)

# (Optional) Install public headers for IntuiCAMCore, if any
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/core/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/IntuiCAM)

# Export the targets to a CMake package configuration
install(EXPORT IntuiCAMTargets
        FILE IntuiCAMTargets.cmake
        NAMESPACE IntuiCAM::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IntuiCAM
)
