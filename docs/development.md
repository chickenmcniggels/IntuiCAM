# Development Guide

Welcome to the IntuiCAM development guide. This document helps new contributors set up their environment, understand coding conventions, and follow best practices for contributing code, tests, and documentation.

---

## 1. Development Environment Setup

### 1.1 Quick Start (Windows - Tested Configuration)

The fastest way to get started with IntuiCAM development on Windows:

1. **Prerequisites (Windows)**:
   * Visual Studio 2022 with C++ workload
   * Qt 6.9.0 for MSVC 2022 (install to `C:/Qt/6.9.0/msvc2022_64`)
   * OpenCASCADE 7.6.0 with 3rdparty dependencies (install to `C:/OpenCASCADE/`)

2. **Clone and Build**:
   ```powershell
   git clone https://github.com/YourOrg/IntuiCAM.git
   cd IntuiCAM
   
   # Create build directory
   mkdir build
   cd build
   
   # Configure for development (Debug build)
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   
   # Build all targets
   cmake --build . --config Debug
   
   # Run the applications (Note: Qt DLLs are automatically deployed)
   .\Debug\IntuiCAMCli.exe --help
   .\Debug\IntuiCAMGui.exe
   ```

### 1.2 Cross-Platform Setup

1. **Clone the repository**:
   ```bash
   git clone https://github.com/YourOrg/IntuiCAM.git
   cd IntuiCAM
   ```

2. **Prerequisites**:
   * C++17-capable compiler (GCC ≥ 9, Clang ≥ 10, MSVC 2019+)
   * CMake ≥ 3.16
   * Qt 6.2+ with OpenGL and OpenGLWidgets components (tested with Qt 6.9.0)
   * OpenCASCADE 7.5.0+ (tested with 7.6.0) - Note: Recent versions have reorganized library names
   * VTK 9.4+ (included with OpenCASCADE)
   * Python 3.7+ (for optional scripting bindings)
   * `git`, `clang-format`, `clang-tidy` (optional but recommended)

3. **Install dependencies** (Ubuntu/Debian example):
   ```bash
   sudo apt update
   sudo apt install build-essential cmake qt6-base-dev python3-dev clang-format clang-tidy
   # Note: OpenCASCADE typically needs to be built from source on Linux
   ```

4. **Build** (Linux/macOS):
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build . --parallel
   ```

5. **Run tests**:
   ```bash
   ctest --output-on-failure
   ```

### 1.3 Common Build Issues

* **OpenCASCADE library not found errors**:
  * Recent OCCT versions have reorganized library names (e.g., `TKSTEP` → `TKDESTEP`)
  * Update CMakeLists.txt to use new library names
  * See `docs/ai_development.md` section 7 for complete mapping

* **Qt OpenGL components missing**:
  * Ensure Qt installation includes OpenGL and OpenGLWidgets components
  * Add these components to `find_package(Qt6 REQUIRED COMPONENTS ...)` in CMakeLists.txt

* **Runtime DLLs not found**:
  * All required DLLs are automatically copied during build using CMake post-build steps
  * Includes Qt6, OpenCASCADE 3rd party libraries, and VTK dependencies
  * If automatic copying fails, check build output for "Warning: DLL not found" messages
  * Manually rebuild the GUI target: `cmake --build . --config Debug --target IntuiCAMGui`

* **Executable path confusion**:
  * Built executables are located in `build/Debug/` or `build/Release/`
  * Run from the build directory: `.\Debug\IntuiCAMGui.exe`
  * Do not expect executables in root `Debug/` directory

For detailed solutions to these and other issues, see [docs/troubleshooting.md](troubleshooting.md).

---

## 2. Coding Standards

* **Language**: Modern C++17/20. Use RAII and smart pointers; avoid raw owning pointers.
* **Style**: Follow Google C++ Style Guide conventions:

  * Classes in `CamelCase`, methods and variables in `snake_case`.
  * Braces on the same line for functions and control statements.
  * 2‑spaces indent, max 100 characters per line.
* **Formatting**: Run `clang-format -i` on changed files. CI will enforce style.
* **Linting**: Use `clang-tidy` to catch common issues. Fix warnings before commit.
* **Documentation**: Public APIs must have Doxygen comments:

  ```cpp
  /// Computes turning toolpath for the given stock and parameters.
  /// \param stock  Stock geometry object.
  /// \param params  Operation parameters.
  /// \return Generated toolpath curve list.
  Toolpath compute_turning_path(const Stock& stock, const TurnParams& params);
  ```

---

## 3. Branching & Commits

* **Main branches**:

  * `main` (stable, always green CI)
  * `develop` (latest development, merge features here)
* **Feature branches**: `feature/<short-description>`
* **Bugfix branches**: `bugfix/<issue-number>-<short-desc>`
* **Commits**:

  * Write clear, imperative subject lines (e.g., "Add facing operation support").
  * Include issue reference (`#123`) when applicable.
  * Limit subject to 50 characters; use body to explain "what" and "why".

---

## 4. Testing & Continuous Integration

### 4.1 Testing Philosophy

Following Qt's best practices for testing, IntuiCAM adopts a multi-level testing approach:

* **Unit tests**: Verify individual functions and classes in isolation
* **Component tests**: Test interactions between related components
* **Integration tests**: Validate end-to-end workflows
* **GUI tests**: Test user interface components and interactions

### 4.2 Test Structure

* **Test location**:
  * Core tests: Located under `core/<module>/tests/`
  * GUI tests: Located under `gui/tests/`
  * Integration tests: Located under `tests/`

* **Test frameworks**:
  * Core tests: GoogleTest
  * GUI tests: Qt Test framework

### 4.3 Writing Testable Code

Following Qt's recommendations for testable code:

* **Break dependencies** using dependency injection where possible
* **Compile classes into libraries** to make them easier to test
* **Separate business logic from UI** to enable testing without a GUI
* **Use interfaces and mocks** for external dependencies

### 4.4 Test Guidelines

* **Test naming**: `test_<component>_<functionality>.cpp`
* **Coverage targets**: 
  * Core algorithm code: Aim for 90%+ test coverage
  * Utility code: Aim for 70%+ test coverage
  * GUI code: Focus on critical paths and workflows

* **Test organization**:
  ```cpp
  // Unit test structure
  TEST(ClassName, FunctionName_Scenario_ExpectedResult) {
    // Arrange: Set up the test environment
    
    // Act: Call the function under test
    
    // Assert: Verify the results
  }
  ```

### 4.5 Testing GUI Components

* **Test in isolation**: Use QTest's `QTEST_MAIN` macro to create a minimal application
* **Use mock data**: Create mock models and controllers to test views
* **Test signals and slots**: Verify that signals trigger the correct slots
* **Avoid screen savers and system dialogs**: Follow Qt's recommendations for test machines setup
* **Consider using headless testing**: Use the offscreen platform plugin when possible

### 4.6 Continuous Integration

* **GitHub Actions workflows** in `.github/workflows/ci.yml`
* **Test matrix**:
  * Operating Systems: Windows, Linux, macOS
  * Build configurations: Debug, Release
  * Qt versions: 5.15, 6.2, 6.6+
* **Scheduled tests**: Daily tests against latest dependencies
* **Pull Request validation**: Every PR requires passing tests

---

## 5. Pull Request Process

1. **Fork & branch** off `develop`.
2. **Implement** your feature/fix with tests.
3. **Update documentation** if needed (README, docs/ files).
4. **Run CI** locally: build, tests, format, lint.
5. **Push** your branch and open a PR against `develop`.
6. **Review**: maintainers will review code, request changes if necessary.
7. **Merge**: after approval and passing CI, PR will be merged.

---

## 6. Reporting Issues

* Use GitHub **Issues** for bugs and feature requests.
* Provide clear descriptions, steps to reproduce, and relevant logs or screenshots.
* Tag issues with appropriate labels (`bug`, `enhancement`, `question`).

---

## 7. Documentation Contributions

* Documentation lives under `docs/`. Follow the **Markdown** style:

  * Use fenced code blocks and relative links.
  * Add new topics under appropriate headings (installation, core, gui, etc.).
* For code snippets or API docs, ensure Doxygen comments in source are up to date.
* After editing docs, build the repo to verify no broken links.
* **Always update documentation** when making changes to code functionality or APIs.
* Follow the principle: "Code isn't complete until it's documented."

---

## 8. Runtime Dependency Management (Windows)

### 8.1 Automatic DLL Deployment

IntuiCAM implements an automated DLL deployment system for Windows builds to ensure the GUI application can run without manual PATH configuration:

**Automatically copied DLLs:**
- **Qt6 libraries**: Core, Gui, Widgets, OpenGL, OpenGLWidgets and their dependencies
- **OpenCASCADE 3rd party libraries**:
  - TBB (Threading Building Blocks): `tbb12.dll`, `tbbmalloc.dll`
  - FreeImage: `FreeImage.dll`
  - FreeType: `freetype.dll`
  - TCL/TK: `tcl86.dll`, `tk86.dll`, `zlib1.dll`
  - Jemalloc: `jemalloc.dll` (critical memory allocator)
  - OpenGL ES/EGL: `libEGL.dll`, `libGLESv2.dll`
  - OpenVR: `openvr_api.dll`
  - FFmpeg: `avcodec-57.dll`, `avutil-55.dll`, `swscale-4.dll`, etc.
- **VTK libraries**: Core VTK rendering and common libraries

### 8.2 Implementation Details

The DLL copying is implemented in `gui/CMakeLists.txt` using CMake post-build commands:

```cmake
# Essential DLLs copied automatically
set(REQUIRED_DLLS
    "C:/OpenCASCADE/3rdparty-vc14-64/tbb-2021.13.0-x64/bin/tbb12.dll"
    "C:/OpenCASCADE/3rdparty-vc14-64/jemalloc-vc14-64/bin/jemalloc.dll"
    # ... other essential DLLs
)

foreach(DLL_FILE ${REQUIRED_DLLS})
    if(EXISTS ${DLL_FILE})
        add_custom_command(TARGET ${GUI_EXECUTABLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}" "$<TARGET_FILE_DIR:${GUI_EXECUTABLE_NAME}>"
        )
    endif()
endforeach()
```

### 8.3 Benefits

- **Self-contained deployment**: No system PATH modifications required
- **Version isolation**: Avoids conflicts with system-wide library versions
- **Simplified distribution**: Application directory contains all dependencies
- **Development convenience**: Build and run immediately without setup

### 8.4 Troubleshooting DLL Issues

If you encounter DLL-related issues:

1. **Check build output** for "Warning: DLL not found" messages
2. **Verify 3rd party paths** match your OpenCASCADE installation
3. **Rebuild GUI target** to trigger DLL copying: `cmake --build . --target IntuiCAMGui`
4. **Check target directory** to see which DLLs were actually copied

### 8.5 Adding New DLL Dependencies

When adding new libraries that require runtime DLLs:

1. Add the DLL path to the `REQUIRED_DLLS` list in `gui/CMakeLists.txt`
2. Test that the DLL exists and is copied during build
3. Update this documentation to reflect the new dependency

---

## 9. Community & Communication

* **Discussions**: Use GitHub Discussions for general questions and proposals.
* **Code of Conduct**: Be respectful and inclusive. See `CODE_OF_CONDUCT.md`.
* **Chat**: (Optional) Slack/Discord invite link can be added here.

Thank you for contributing to IntuiCAM! Your work helps make desktop CNC turning accessible to everyone.
