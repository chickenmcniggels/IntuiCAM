# Troubleshooting Guide

This document helps resolve common issues encountered when building and running IntuiCAM.

---

## Build Issues

### OpenCASCADE Library Not Found

**Problem:** CMake fails with errors like `TKSTEP.lib` cannot be found.

**Cause:** Recent OpenCASCADE versions have reorganized library names.

**Solution:** Update your CMakeLists.txt to use the new library names:

```cmake
# Old names (will cause linker errors):
TKSTEP         # ❌ No longer exists
TKIGES         # ❌ No longer exists  
TKSTL          # ❌ No longer exists
TKSTEP209      # ❌ Consolidated into TKDESTEP
TKSTEPAttr     # ❌ Consolidated into TKDESTEP
TKSTEPBase     # ❌ Consolidated into TKDESTEP

# New names (correct):
TKDESTEP       # ✅ STEP file import/export
TKDEIGES       # ✅ IGES file import/export
TKDESTL        # ✅ STL file import/export
TKDEVRML       # ✅ VRML file import/export
```

**Complete mapping of changed library names:**
- `TKSTEP*` libraries → `TKDESTEP`
- `TKIGES` → `TKDEIGES`
- `TKSTL` → `TKDESTL`
- `TKVRML` → `TKDEVRML`

### Qt OpenGL Components Missing

**Problem:** Build fails with "QOpenGLWidget not found" or similar Qt OpenGL errors.

**Cause:** Qt installation missing OpenGL components or CMake not finding them.

**Solution:**

1. **Verify Qt Installation:** Ensure your Qt installation includes OpenGL components:
   ```bash
   # Check if OpenGL components are installed
   find /path/to/qt -name "*OpenGL*"
   ```

2. **Update CMakeLists.txt:** Add OpenGL components to find_package:
   ```cmake
   find_package(Qt6 REQUIRED COMPONENTS 
       Core 
       Gui 
       Widgets 
       OpenGL          # Add this
       OpenGLWidgets   # Add this
   )
   ```

3. **Link OpenGL libraries:**
   ```cmake
   target_link_libraries(${TARGET_NAME} PRIVATE
       Qt6::Core
       Qt6::Gui
       Qt6::Widgets
       Qt6::OpenGL
       Qt6::OpenGLWidgets
   )
   ```

### AIS_Shaded.hxx Not Found

**Problem:** Compilation fails with "AIS_Shaded.hxx: No such file or directory".

**Cause:** `AIS_Shaded` is not a header file, it's an enum value.

**Solution:** Replace the incorrect include:
```cpp
// ❌ Wrong:
#include <AIS_Shaded.hxx>

// ✅ Correct:
#include <AIS_DisplayMode.hxx>
```

### CMake Cannot Find OpenCASCADE

**Problem:** CMake fails to find OpenCASCADE installation.

**Solution:**

1. **Set OpenCASCADE_DIR:** Point to the CMake configuration directory:
   ```bash
   cmake -DOpenCASCADE_DIR=/path/to/opencascade/cmake ..
   ```

2. **Windows example:**
   ```powershell
   cmake -DOpenCASCADE_DIR="C:/OpenCASCADE/occt-vc144-64-with-debug/cmake" ..
   ```

3. **Add to CMAKE_PREFIX_PATH:**
   ```cmake
   list(APPEND CMAKE_PREFIX_PATH "C:/OpenCASCADE/occt-vc144-64-with-debug/cmake")
   ```

---

## Runtime Issues

### Executable Not Found

**Problem:** `./Debug/IntuiCAMGui.exe` command fails with "command not found".

**Cause:** Executables are built in the `build/Debug/` directory, not root `Debug/`.

**Solution:**
```powershell
# ❌ Wrong path:
./Debug/IntuiCAMGui.exe

# ✅ Correct paths:
# From project root:
./build/Debug/IntuiCAMGui.exe

# From build directory:
./Debug/IntuiCAMGui.exe
```

### Runtime DLL Not Found (Windows)

**Problem:** GUI application fails to start with DLL errors like:
- "Qt6Core.dll not found" 
- "libEGL.dll not found"
- "jemalloc.dll not found"
- "avcodec-57.dll not found"
- "openvr_api.dll not found"

**Cause:** This should normally not happen with recent builds as all required DLLs are automatically copied.

**Automatic Solution:** IntuiCAM now automatically copies all required DLLs during the build process, including:
- Qt6 DLLs (Core, Gui, Widgets, OpenGL, etc.)
- OpenCASCADE 3rd party DLLs (TBB, FreeImage, FFmpeg, OpenVR, Jemalloc, etc.)
- VTK libraries

**Solutions (if automatic deployment fails):**

1. **Option 1 - Rebuild the GUI target:**
   ```powershell
   cmake --build . --config Debug --target IntuiCAMGui
   # This should automatically copy all required DLLs
   ```

2. **Option 2 - Check for warnings during build:**
   Look for messages like "Warning: DLL not found: ..." during build and verify the 3rd party library paths are correct.

3. **Option 3 - Manual Qt deployment (if needed):**
   ```powershell
   & "C:\Qt\6.9.0\msvc2022_64\bin\windeployqt.exe" .\Debug\IntuiCAMGui.exe
   ```

4. **Option 4 - Add paths to system PATH (temporary):**
   ```powershell
   $env:PATH = "C:\Qt\6.9.0\msvc2022_64\bin;C:\OpenCASCADE\occt-vc144-64-with-debug\win64\vc14\bin;C:\OpenCASCADE\3rdparty-vc14-64\tbb-2021.13.0-x64\bin;" + $env:PATH
   .\Debug\IntuiCAMGui.exe
   ```

### OpenCASCADE DLL Not Found (Windows)

**Problem:** Application fails with OpenCASCADE DLL errors.

**Solution:** Add OpenCASCADE bin directory to PATH:
```powershell
$env:PATH = "C:\OpenCASCADE\occt-vc144-64-with-debug\win64\vc14\bin;" + $env:PATH
```

### Application Crashes on Startup

**Problem:** GUI application starts but crashes immediately.

**Debugging steps:**

1. **Run in Debug mode:**
   ```bash
   gdb ./Debug/IntuiCAMGui
   (gdb) run
   (gdb) bt  # Get backtrace when it crashes
   ```

2. **Check dependency versions:**
   Ensure Qt, OpenCASCADE, and VTK versions are compatible.

3. **Verify OpenCASCADE installation:**
   Test with a simple OpenCASCADE program first.

---

## Development Issues

### Include Directories Not Found

**Problem:** Compiler cannot find OpenCASCADE or Qt headers.

**Solution:** Verify include directories are properly set:
```cmake
target_include_directories(${TARGET_NAME} PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
    # Add other include paths as needed
)
```

### Linking Errors

**Problem:** Undefined references to OpenCASCADE functions.

**Solution:**

1. **Check library names** (see OpenCASCADE library mapping above)
2. **Verify library paths:**
   ```cmake
   link_directories(${OpenCASCADE_LIBRARY_DIR})
   ```
3. **Use minimal library set** to avoid conflicts:
   ```cmake
   set(MINIMAL_OCCT_LIBS
       TKernel TKMath TKG2d TKG3d TKGeomBase TKGeomAlgo
       TKBRep TKTopAlgo TKPrim TKService TKV3d TKOpenGl
       TKXSBase TKDESTEP
   )
   ```

---

## Getting Help

If you're still experiencing issues:

1. **Check the logs:** Look for error messages in the build output or application logs
2. **Verify versions:** Ensure you're using supported versions of dependencies
3. **Search existing issues:** Check the GitHub issues for similar problems
4. **Create an issue:** If the problem persists, create a detailed issue report with:
   - Operating system and version
   - Compiler version
   - CMake version
   - Qt version
   - OpenCASCADE version
   - Complete build output or error messages
   - Steps to reproduce the problem

---

## Additional Resources

- [Development Guide](development.md) - Complete development setup
- [AI Development Guidelines](ai_development.md) - AI-specific considerations
- [Architecture Overview](architecture.md) - Project structure and design
- [OpenCASCADE Documentation](https://dev.opencascade.org/doc/) - Official OCCT docs
- [Qt Documentation](https://doc.qt.io/qt-6/) - Official Qt documentation 