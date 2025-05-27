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
)

set(GUI_HEADERS
    src/mainwindow.h
    include/opengl3dwidget.h
    include/isteploader.h
    include/steploader.h
    include/chuckmanager.h
    include/workpiecemanager.h
    include/rawmaterialmanager.h
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
# Based on minimal requirements from OCCT samples
set(OCCT_GUI_LIBS
    TKernel        # OCCT kernel
    TKMath         # Mathematical functions
    TKG2d          # 2D geometry
    TKG3d          # 3D geometry
    TKGeomBase     # Geometry base
    TKGeomAlgo     # Geometry algorithms
    TKBRep         # Boundary representation
    TKTopAlgo      # Topology algorithms
    TKPrim         # Primitive objects
    TKService      # Visualization services
    TKV3d          # Core 3D visualization
    TKOpenGl       # OpenGL rendering backend
    TKOpenGles     # OpenGL ES rendering backend (for some platforms)
    TKCAF          # Common Application Framework
    TKShHealing    # Shape healing utilities
    TKDE           # Data Exchange utilities
    TKXSBase       # Exchange base
    TKDESTEP       # STEP file reader/writer
)
target_link_libraries(${GUI_EXECUTABLE_NAME} PRIVATE ${OCCT_GUI_LIBS})

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

# Copy required DLLs to output directory for runtime dependency resolution
if(WIN32)
    # Define essential DLLs that are commonly required by OpenCASCADE applications
    # This approach is more selective than copying entire bin directories
    set(REQUIRED_DLLS
        # TBB (Threading Building Blocks) - Essential for parallel processing
        "C:/OpenCASCADE/3rdparty-vc14-64/tbb-2021.13.0-x64/bin/tbb12.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/tbb-2021.13.0-x64/bin/tbb12_debug.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/tbb-2021.13.0-x64/bin/tbbmalloc.dll"
        
        # FreeImage - Image processing
        "C:/OpenCASCADE/3rdparty-vc14-64/freeimage-3.18.0-x64/bin/FreeImage.dll"
        
        # FreeType - Font rendering
        "C:/OpenCASCADE/3rdparty-vc14-64/freetype-2.13.3-x64/bin/freetype.dll"
        
        # TCL/TK - Scripting support
        "C:/OpenCASCADE/3rdparty-vc14-64/tcltk-8.6.15-x64/bin/tcl86.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/tcltk-8.6.15-x64/bin/tk86.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/tcltk-8.6.15-x64/bin/zlib1.dll"
        
        # Jemalloc - Memory allocator (critical for OpenCASCADE)
        "C:/OpenCASCADE/3rdparty-vc14-64/jemalloc-vc14-64/bin/jemalloc.dll"
        
        # OpenGL ES / EGL - Graphics support (ANGLE)
        "C:/OpenCASCADE/3rdparty-vc14-64/angle-gles2-2.1.0-vc14-64/bin/libEGL.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/angle-gles2-2.1.0-vc14-64/bin/libGLESv2.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/angle-gles2-2.1.0-vc14-64/bin/d3dcompiler_47.dll"
        
        # OpenVR - Virtual Reality support (if needed)
        "C:/OpenCASCADE/3rdparty-vc14-64/openvr-1.14.15-64/bin/win64/openvr_api.dll"
        
        # FFmpeg - Video/multimedia support
        "C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/bin/avcodec-57.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/bin/avdevice-57.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/bin/avfilter-6.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/bin/avformat-57.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/bin/avutil-55.dll"
        "C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/bin/swscale-4.dll"
    )
    
    # Copy only essential DLLs that exist
    foreach(DLL_FILE ${REQUIRED_DLLS})
        if(EXISTS ${DLL_FILE})
            add_custom_command(TARGET ${GUI_EXECUTABLE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_FILE}"
                "$<TARGET_FILE_DIR:${GUI_EXECUTABLE_NAME}>"
                COMMENT "Copying essential DLL: ${DLL_FILE}"
            )
        else()
            message(STATUS "Warning: DLL not found: ${DLL_FILE}")
        endif()
    endforeach()
    
    # Copy VTK DLLs (these are numerous, so we'll copy the essential ones)
    set(VTK_BIN_PATH "C:/OpenCASCADE/3rdparty-vc14-64/vtk-9.4.1-x64/bin")
    if(EXISTS ${VTK_BIN_PATH})
        # Copy core VTK DLLs that OpenCASCADE typically needs
        file(GLOB VTK_CORE_DLLS 
            "${VTK_BIN_PATH}/vtkCommon*.dll"
            "${VTK_BIN_PATH}/vtkRendering*.dll"
            "${VTK_BIN_PATH}/vtkFiltering*.dll"
            "${VTK_BIN_PATH}/vtkIO*.dll"
        )
        foreach(VTK_DLL ${VTK_CORE_DLLS})
            add_custom_command(TARGET ${GUI_EXECUTABLE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VTK_DLL}"
                "$<TARGET_FILE_DIR:${GUI_EXECUTABLE_NAME}>"
                COMMENT "Copying VTK DLL: ${VTK_DLL}"
            )
        endforeach()
    endif()
endif()
