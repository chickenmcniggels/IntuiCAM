cmake_minimum_required(VERSION 3.16)
project(IntuiCAM VERSION 1.0 LANGUAGES CXX)

# Enable C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler options for MSVC 2022
if(MSVC)
    add_compile_options(/W4 /MP)
    # Use multi-process compilation
    # Add runtime library flags if needed: /MD, /MDd, etc.
endif()

add_definitions(-D_USE_MATH_DEFINES)  # Define _USE_MATH_DEFINES for math constants

# Find required packages - Qt and OpenCASCADE
set(CMAKE_PREFIX_PATH 
    "C:/Qt/6.9.0/msvc2022_64/lib/cmake/Qt6"
    "C:/OpenCASCADE/occt-vc144-64-with-debug/cmake"
    ${CMAKE_PREFIX_PATH}
)

# Configure OpenCASCADE with 3rd party dependencies
set(OpenCASCADE_ROOT_DIR "C:/OpenCASCADE/occt-vc144-64-with-debug")
set(OpenCASCADE_THIRD_PARTY_DIR "C:/OpenCASCADE/3rdparty-vc14-64")

# Find OpenCASCADE components
find_package(OpenCASCADE REQUIRED COMPONENTS 
    TKernel
    TKMath
    TKBRep
    TKG3d
    TKTopAlgo
    TKGeomBase
    TKGeomAlgo
    TKBO
    TKPrim
    TKSTEP
    TKSTEPBase
    TKSTEPAttr
    TKSTEP209
    TKXSBase
    TKShHealing
    TKHLR
    TKMesh
    TKService
    TKV3d
)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets)
qt_standard_project_setup()

# Output some useful information
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "OpenCASCADE version: ${OpenCASCADE_VERSION}")
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# Build options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_GUI "Build GUI application" ON)
option(BUILD_CLI "Build command-line interface" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_PYTHON_WRAPPER "Build Python wrapper" OFF)
option(BUILD_FREECAD_PLUGIN "Build as FreeCAD plugin" OFF)

# Add subdirectories for modules
add_subdirectory(core)

if(BUILD_GUI)
    add_subdirectory(gui)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_CLI)
    add_subdirectory(cli)
endif()

if(BUILD_PYTHON_WRAPPER)
    add_subdirectory(python)
endif()

if(BUILD_FREECAD_PLUGIN)
    add_subdirectory(freecad)
endif()

# Installation of targets (library and executable)
include(GNUInstallDirs)  # Ensures standard install directories (bin, lib, etc.)

# Install IntuiCAMCore library
install(TARGETS IntuiCAMCore
        EXPORT IntuiCAMTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install public headers for IntuiCAMCore
install(DIRECTORY ${CMAKE_SOURCE_DIR}/core/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/IntuiCAM
)

# Export the targets to a CMake package configuration
install(EXPORT IntuiCAMTargets
        FILE IntuiCAMTargets.cmake
        NAMESPACE IntuiCAM::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IntuiCAM
)

# Generate and install package configuration files
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/IntuiCAMConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/IntuiCAMConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IntuiCAM
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/IntuiCAMConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/IntuiCAMConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/IntuiCAMConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IntuiCAM
)
