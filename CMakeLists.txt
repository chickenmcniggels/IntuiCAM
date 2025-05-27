cmake_minimum_required(VERSION 3.16)
project(IntuiCAM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Options to control build targets ---
option(INTUICAM_BUILD_GUI "Build the IntuiCAM Qt GUI application" ON)
option(INTUICAM_BUILD_CLI "Build the IntuiCAM command-line interface" ON)
option(INTUICAM_BUILD_PYTHON "Build Python bindings for Core libraries" OFF)
option(INTUICAM_BUILD_TESTS "Build unit and integration tests" ON)

# Set Qt, VTK, and OpenCASCADE paths
if(NOT DEFINED CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "")
endif()

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64")
list(APPEND CMAKE_PREFIX_PATH "C:/OpenCASCADE/3rdparty-vc14-64/vtk-9.4.1-x64/lib/cmake/vtk-9.4")
list(APPEND CMAKE_PREFIX_PATH "C:/OpenCASCADE/occt-vc144-64-with-debug/cmake")

# Add EGL and other 3rdparty library paths
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/angle-gles2-2.1.0-vc14-64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/draco-1.4.1-vc14-64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/jemalloc-vc14-64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/openvr-1.14.15-64/lib/win64")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/freeimage-3.18.0-x64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/freetype-2.13.3-x64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/tbb-2021.13.0-x64/lib")
link_directories("C:/OpenCASCADE/3rdparty-vc14-64/tcltk-8.6.15-x64/lib")

# Add OpenCASCADE library paths
link_directories("C:/OpenCASCADE/occt-vc144-64-with-debug/win64/vc14/lib")
link_directories("C:/OpenCASCADE/occt-vc144-64-with-debug/win64/vc14/libd")

# Find Qt6 with components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
qt_standard_project_setup()

# Find VTK first (required by OpenCASCADE)
set(VTK_DIR "C:/OpenCASCADE/3rdparty-vc14-64/vtk-9.4.1-x64/lib/cmake/vtk-9.4")
find_package(VTK REQUIRED)

# Set up OpenCASCADE
set(OpenCASCADE_DIR "C:/OpenCASCADE/occt-vc144-64-with-debug/cmake")
find_package(OpenCASCADE REQUIRED)

# Fix hardcoded jemalloc path issue in OpenCASCADE configuration
# Replace incorrect jemalloc path with correct one
if(TARGET TKernel)
    get_target_property(TKERNEL_LINK_LIBS TKernel INTERFACE_LINK_LIBRARIES)
    if(TKERNEL_LINK_LIBS)
        string(REPLACE "C:/work/occt/3rdparty-vc14-64/jemalloc-vc14-64/lib/jemalloc.lib" 
                       "C:/OpenCASCADE/3rdparty-vc14-64/jemalloc-vc14-64/lib/jemalloc.lib" 
                       TKERNEL_LINK_LIBS_FIXED "${TKERNEL_LINK_LIBS}")
        set_target_properties(TKernel PROPERTIES INTERFACE_LINK_LIBRARIES "${TKERNEL_LINK_LIBS_FIXED}")
    endif()
endif()

# Clean up OpenCASCADE compiler flags to avoid duplicates and malformed flags
if(OpenCASCADE_CXX_FLAGS)
    string(REGEX REPLACE "/fp:precise" "" OpenCASCADE_CXX_FLAGS_CLEANED "${OpenCASCADE_CXX_FLAGS}")
    string(REGEX REPLACE "[ ]+" " " OpenCASCADE_CXX_FLAGS_CLEANED "${OpenCASCADE_CXX_FLAGS_CLEANED}")
    string(STRIP "${OpenCASCADE_CXX_FLAGS_CLEANED}" OpenCASCADE_CXX_FLAGS_CLEANED)
    set(OpenCASCADE_CXX_FLAGS "${OpenCASCADE_CXX_FLAGS_CLEANED}")
endif()

# --- Include Directories ---
include_directories(
    ${OpenCASCADE_INCLUDE_DIR}
)

# --- Add Subdirectories ---
# Core libraries will be added here.
# Each core module (common, geometry, etc.) will be a subdirectory in 'core'.
add_subdirectory(core)

if(INTUICAM_BUILD_GUI)
    add_subdirectory(gui)
endif()

if(INTUICAM_BUILD_CLI)
    add_subdirectory(cli)
endif()


# The individual module CMakeLists.txt files will handle target linking
# This section is kept minimal as targets are properly configured in their respective subdirectories

# --- CPack Configuration for Professional Distribution ---
if(INTUICAM_BUILD_GUI)
    # Include CPack for creating installers
    include(InstallRequiredSystemLibraries)
    
    # Set package information
    set(CPACK_PACKAGE_NAME "IntuiCAM")
    set(CPACK_PACKAGE_VENDOR "IntuiCAM Development Team")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Advanced CAM Software for Precision Manufacturing")
    
    # Windows-specific packaging
    if(WIN32)
        set(CPACK_GENERATOR "NSIS;ZIP")
        set(CPACK_NSIS_DISPLAY_NAME "IntuiCAM")
        set(CPACK_NSIS_HELP_LINK "https://github.com/your-repo/IntuiCAM")
        set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/your-repo/IntuiCAM")
        set(CPACK_NSIS_CONTACT "support@intuicam.com")
        
        # Install the GUI executable
        install(TARGETS IntuiCAMGui 
            RUNTIME DESTINATION bin
            COMPONENT Applications)
            
        # Install the CLI executable
        if(INTUICAM_BUILD_CLI)
            install(TARGETS IntuiCAMCli 
                RUNTIME DESTINATION bin
                COMPONENT Applications)
        endif()
    endif()
    
    include(CPack)
endif()
