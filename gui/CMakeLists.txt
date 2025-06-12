# IntuiCAM/gui/CMakeLists.txt

set(GUI_EXECUTABLE_NAME IntuiCAMGui)

# Add source files for the GUI
set(GUI_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/opengl3dwidget.cpp
    src/steploader.cpp
    src/chuckmanager.cpp
    src/workpiecemanager.cpp
    src/rawmaterialmanager.cpp
    src/workspacecontroller.cpp
    src/partloadingpanel.cpp
    src/setupconfigurationpanel.cpp
    src/toolpathgenerationcontroller.cpp
    src/operationparameterdialog.cpp
    src/materialmanager.cpp
    src/toolmanager.cpp
    src/toolpathtimelinewidget.cpp
    src/toolpathmanager.cpp
)

set(GUI_HEADERS
    include/mainwindow.h
    include/opengl3dwidget.h
    include/steploader.h
    include/chuckmanager.h
    include/workpiecemanager.h
    include/rawmaterialmanager.h
    include/workspacecontroller.h
    include/partloadingpanel.h
    include/setupconfigurationpanel.h
    include/toolpathgenerationcontroller.h
    include/operationparameterdialog.h
    include/materialmanager.h
    include/toolmanager.h
    include/toolpathtimelinewidget.h
    include/toolpathmanager.h
)

set(GUI_UI_FILES
    resources/mainwindow.ui
)

set(GUI_RESOURCE_FILES
    resources/resources.qrc
)

# Ensure CMAKE_AUTOMOC, CMAKE_AUTOUIC, CMAKE_AUTORCC are ON (should be set in root CMakeLists.txt)

qt_add_executable(${GUI_EXECUTABLE_NAME}
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_UI_FILES}
    ${GUI_RESOURCE_FILES}
)

# Include directories for the GUI module itself
target_include_directories(${GUI_EXECUTABLE_NAME} PRIVATE 
    src 
    include
)

# Link against Qt6 modules
target_link_libraries(${GUI_EXECUTABLE_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Link against OpenCASCADE modules for visualization and STEP import
# Based on actual available libraries in OpenCASCADE installation
target_link_libraries(${GUI_EXECUTABLE_NAME} PRIVATE
    # Kernel and foundation
    TKernel
    TKMath
    
    # Data exchange - Updated library names
    TKXSBase
    TKDESTEP
    TKDEIGES
    
    # Application framework and visualization
    TKVCAF
    TKV3d
    TKOpenGl
    TKService
    
    # Basic topology and geometry
    TKG3d
    TKGeomBase
    TKBRep
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    
    # Mesh and shape healing
    TKMesh
    TKShHealing
    
    # Boolean operations and primitives
    TKBO
    TKBool
    
    # Hidden line removal
    TKHLR
)

# Optional: Set additional compiler flags for better optimization and warnings
if(MSVC)
    target_compile_options(${GUI_EXECUTABLE_NAME} PRIVATE /W3)
else()
    target_compile_options(${GUI_EXECUTABLE_NAME} PRIVATE -Wall -Wextra)
endif()

# Installation rules
install(TARGETS ${GUI_EXECUTABLE_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Link against our static core libraries
target_link_libraries(${GUI_EXECUTABLE_NAME} PRIVATE
    intuicam_core_common
    intuicam_core_geometry
    intuicam_core_toolpath
    intuicam_core_postprocessor
    intuicam_core_simulation
)

# OpenCASCADE flags are inherited through core libraries and direct OCCT linking
# No additional OpenCASCADE flags needed for GUI

# Set target properties for GUI application
set_target_properties(${GUI_EXECUTABLE_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# Add include directories from core modules that GUI needs direct header access to
# (target_link_libraries above handles include dirs for compilation of gui sources against core public headers)
# However, if gui/src itself needs to #include <IntuiCAM/CoreModule/Header.h>, this is handled by core modules exporting their include directories.

# Deploy Qt libraries on Windows
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${GUI_EXECUTABLE_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${GUI_EXECUTABLE_NAME}>"
            COMMENT "Deploying Qt runtime")
    endif()
endif()
