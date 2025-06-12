cmake_minimum_required(VERSION 3.16)
project(IntuiCAM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Options to control build targets ---
option(INTUICAM_BUILD_GUI "Build the IntuiCAM Qt GUI application" ON)
option(INTUICAM_BUILD_PYTHON "Build Python bindings for Core libraries" OFF)
option(INTUICAM_BUILD_TESTS "Build unit and integration tests" ON)


# Find Qt6 with components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
qt_standard_project_setup()

# Find VTK first (required by OpenCASCADE)
find_package(VTK REQUIRED)

# Set up OpenCASCADE
find_package(OpenCASCADE REQUIRED)

# --- Add Subdirectories ---
# Core libraries will be added here.
# Each core module (common, geometry, etc.) will be a subdirectory in 'core'.
add_subdirectory(core)

if(INTUICAM_BUILD_GUI)
    add_subdirectory(gui)
endif()



# Add tests if enabled
if(INTUICAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Add Python bindings if enabled
if(INTUICAM_BUILD_PYTHON)
    add_subdirectory(core/python)
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
            
    endif()
    
    include(CPack)
endif()
