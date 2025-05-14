cmake_minimum_required(VERSION 3.16)
project(IntuiCAM_GUI LANGUAGES CXX)

# Find Qt components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets)
qt_standard_project_setup()

# Find OpenCASCADE visualization components
find_package(OpenCASCADE REQUIRED COMPONENTS 
    TKService
    TKV3d
    TKHLR
)

# Find all source files
file(GLOB_RECURSE GUI_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE GUI_HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

file(GLOB_RECURSE GUI_UI_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
)

# Resources
set(RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc
)

# Define the GUI executable
qt_add_executable(IntuiCAM_GUI
    MANUAL_FINALIZATION
    WIN32
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_UI_FILES}
    ${RESOURCE_FILES}
)

# Configure include directories
target_include_directories(IntuiCAM_GUI
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OpenCASCADE_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(IntuiCAM_GUI
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        IntuiCAM::IntuiCAMCore
        ${OpenCASCADE_LIBRARIES}
)

# Finalize Qt executable (handles deployment)
qt_finalize_executable(IntuiCAM_GUI)

# Installation
install(TARGETS IntuiCAM_GUI
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install dependent DLLs on Windows if shared libraries
if(WIN32 AND BUILD_SHARED_LIBS)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    
    # Install Qt DLLs
    install(
        DIRECTORY "${_qt_bin_dir}/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILES_MATCHING PATTERN "*.dll"
    )

    # Install OpenCASCADE DLLs if needed
    if(OpenCASCADE_BINARY_DIR)
        install(
            DIRECTORY "${OpenCASCADE_BINARY_DIR}/"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            FILES_MATCHING PATTERN "*.dll"
        )
    endif()
endif()
