# Windows Setup Guide

This guide provides detailed instructions for setting up IntuiCAM on Windows, including all configuration fixes and optimizations that have been applied.

---

## Tested Configuration

**Successfully verified on:**
- Windows 10/11 (Build 22631)
- Visual Studio 2022 Community Edition
- Qt 6.9.0 for MSVC 2022 64-bit
- OpenCASCADE 7.6.0 with complete 3rdparty dependencies
- VTK 9.4.1 (included with OpenCASCADE)

---

## Prerequisites Installation

### 1. Visual Studio 2022

1. Download from [Microsoft Visual Studio](https://visualstudio.microsoft.com/downloads/)
2. Install with **Desktop development with C++** workload
3. Ensure MSVC v143 compiler toolset is included
4. CMake tools for Visual Studio (included by default)

### 2. Qt 6.9.0

1. Download the [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. Create a Qt account (free for open source development)
3. Install to the standard path: `C:\Qt\6.9.0\msvc2022_64`
4. Select the following components:
   - Qt 6.9.0 → MSVC 2022 64-bit
   - Qt 6.9.0 → Qt Creator (optional)
   - Qt 6.9.0 → Sources (optional)

### 3. OpenCASCADE 7.6.0

1. Download from [OpenCASCADE Release Page](https://dev.opencascade.org/release)
2. Download the **complete package** with 3rdparty dependencies:
   - `opencascade-7.6.0-vc14-64.exe` (main package)
   - `3rdparty-vc14-64.exe` (3rdparty dependencies)
3. Install both packages to `C:\OpenCASCADE\`
4. Verify the following directories exist:
   ```
   C:\OpenCASCADE\
   ├── occt-vc144-64-with-debug\
   │   ├── win64\vc14\lib\          # OpenCASCADE libraries
   │   └── cmake\                   # CMake configuration files
   └── 3rdparty-vc14-64\
       ├── vtk-9.4.1-x64\           # VTK libraries
       ├── jemalloc-vc14-64\        # Memory allocator
       ├── freeimage-3.18.0-x64\    # Image processing
       ├── openvr-1.14.15-64\       # VR support
       ├── ffmpeg-3.3.4-64\         # Video codecs
       ├── tbb-2021.13.0-x64\       # Threading Building Blocks
       └── tcltk-8.6.15-x64\        # Tcl/Tk scripting
   ```

---

## Building IntuiCAM

### Quick Build Process

1. **Clone Repository**:
   ```powershell
   git clone https://github.com/your-org/IntuiCAM.git
   cd IntuiCAM
   ```

2. **Configure Build**:
   ```powershell
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build Project**:
   ```powershell
   cmake --build . --config Release
   ```

4. **Test Build**:
   ```powershell
   
   # Launch GUI
   .\Release\IntuiCAMGui.exe
   ```

### Build Output

Successful build produces:
- `build\Release\IntuiCAMGui.exe` - Qt GUI application (with auto-deployed DLLs)
- `build\core\*\Release\*.lib` - Static core libraries

**Important:** The GUI build automatically copies all required runtime DLLs to the output directory, including:
- Qt6 DLLs (Core, Gui, Widgets, OpenGL, etc.)
- OpenCASCADE 3rd party DLLs (TBB, FreeImage, FFmpeg, OpenVR, etc.)
- No additional PATH configuration is required

---

## Configuration Details

### CMake Configuration

The project's `CMakeLists.txt` has been configured with:

1. **Automatic Path Detection**:
   ```cmake
   list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64")
   list(APPEND CMAKE_PREFIX_PATH "C:/OpenCASCADE/3rdparty-vc14-64/vtk-9.4.1-x64/lib/cmake/vtk-9.4")
   list(APPEND CMAKE_PREFIX_PATH "C:/OpenCASCADE/occt-vc144-64-with-debug/cmake")
   ```

2. **3rdParty Library Paths**:
   ```cmake
   link_directories("C:/OpenCASCADE/3rdparty-vc14-64/openvr-1.14.15-64/lib/win64")
   link_directories("C:/OpenCASCADE/3rdparty-vc14-64/freeimage-3.18.0-x64/lib")
   link_directories("C:/OpenCASCADE/3rdparty-vc14-64/ffmpeg-3.3.4-64/lib")
   link_directories("C:/OpenCASCADE/3rdparty-vc14-64/freetype-2.13.3-x64/lib")
   link_directories("C:/OpenCASCADE/3rdparty-vc14-64/tbb-2021.13.0-x64/lib")
   link_directories("C:/OpenCASCADE/3rdparty-vc14-64/tcltk-8.6.15-x64/lib")
   ```

3. **OpenCASCADE jemalloc Path Fix**:
   ```cmake
   # Fix hardcoded jemalloc path issue in OpenCASCADE configuration
   if(TARGET TKernel)
       get_target_property(TKERNEL_LINK_LIBS TKernel INTERFACE_LINK_LIBRARIES)
       if(TKERNEL_LINK_LIBS)
           string(REPLACE "C:/work/occt/3rdparty-vc14-64/jemalloc-vc14-64/lib/jemalloc.lib" 
                          "C:/OpenCASCADE/3rdparty-vc14-64/jemalloc-vc14-64/lib/jemalloc.lib" 
                          TKERNEL_LINK_LIBS_FIXED "${TKERNEL_LINK_LIBS}")
           set_target_properties(TKernel PROPERTIES INTERFACE_LINK_LIBRARIES "${TKERNEL_LINK_LIBS_FIXED}")
       endif()
   endif()
   ```

### OpenCASCADE Library Configuration

**GUI Libraries (gui/CMakeLists.txt)**:
```cmake
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
    TKOpenGles     # OpenGL ES rendering backend
    TKCAF          # Common Application Framework
    TKShHealing    # Shape healing utilities
    TKDE           # Data Exchange utilities
    TKXSBase       # Exchange base
    TKDESTEP       # STEP file reader/writer
)
```

**Automatic DLL Deployment:**
The GUI target automatically copies essential 3rd party DLLs to the output directory:
```cmake
# Essential DLLs copied automatically during build:
- TBB libraries (tbb12.dll, tbbmalloc.dll)
- FreeImage (FreeImage.dll)
- FreeType (freetype.dll)
- TCL/TK (tcl86.dll, tk86.dll)
- Jemalloc (jemalloc.dll)
- OpenGL ES/EGL (libEGL.dll, libGLESv2.dll)
- OpenVR (openvr_api.dll)
- FFmpeg libraries (avcodec-57.dll, avutil-55.dll, etc.)
- VTK core libraries (vtkCommon*.dll, vtkRendering*.dll)
```

**CLI Libraries (cli/CMakeLists.txt)**:
```cmake
set(OCCT_CLI_LIBS
    TKernel      # Core OCCT kernel
    TKMath       # Mathematical functions
    TKTopAlgo    # Topological algorithms
    TKGeomAlgo   # Geometric algorithms
    TKBRep       # Boundary representation
    TKGeomBase   # Basic geometric functions
)
```

---

## Common Issues & Solutions

### ✅ Resolved Issues

1. **jemalloc.lib path errors**:
   - **Problem**: OpenCASCADE CMake config had hardcoded path `C:\work\occt\...`
   - **Solution**: Automatic path correction in CMakeLists.txt

2. **Missing TKAIS.lib**:
   - **Problem**: Referenced non-existent library
   - **Solution**: Removed from GUI configuration

3. **Missing 3rdparty libraries**:
   - **Problem**: openvr_api.lib, FreeImage.lib, etc. not found
   - **Solution**: Added all 3rdparty library paths to CMakeLists.txt

4. **Runtime DLL not found errors**:
   - **Problem**: Application fails to start due to missing DLLs (libEGL.dll, avcodec.dll, etc.)
   - **Solution**: Implemented automatic DLL copying during build process

### Current Known Issues

1. **Vulkan headers warning**:
   - **Status**: Non-fatal Qt warning
   - **Impact**: None - project builds and runs successfully
   - **Message**: `Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)`

### Troubleshooting

If you encounter build errors:

1. **Verify paths**: Ensure Qt and OpenCASCADE are installed to the expected paths
2. **Check dependencies**: Verify all 3rdparty libraries are present
3. **Clean build**: Delete `build` directory and reconfigure
4. **Check compiler**: Ensure Visual Studio 2022 with C++ workload is installed
5. **Check CMake version**: Ensure CMake 3.16+ is available

---

## Development Workflow

### IDE Setup

**Visual Studio 2022**:
1. Open `IntuiCAM.sln` from the build directory
3. Build configuration: Release or Debug

**Qt Creator** (Optional):
1. Open `CMakeLists.txt` as project
2. Configure with detected CMake and compiler
3. Use built-in Qt tools for UI design

### Debugging

**Debug Build**:
```powershell
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

**Debug executables**:
- `build\Debug\IntuiCAMGui.exe`

### Testing

Currently no automated tests are configured. Manual testing:
```powershell

# Test GUI launch
.\Release\IntuiCAMGui.exe
```

---

## Performance Notes

- **Build time**: ~2-5 minutes on modern hardware
- **Executable sizes**: 
  - CLI: ~13KB
  - GUI: ~28KB
- **Runtime dependencies**: Qt6 DLLs, OpenCASCADE DLLs (ensure in PATH or copy to output)

---

## Support

For Windows-specific issues:
1. Check this guide first
2. Verify your installation matches the tested configuration
3. Report issues with system information (Windows version, VS version, etc.) 