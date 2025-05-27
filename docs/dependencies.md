# Dependency Management Guide

This guide details the dependencies required by IntuiCAM, their version compatibility, and integration approaches for different development scenarios.

---

## 1. Required Dependencies

IntuiCAM depends on several key libraries that must be properly integrated:

| Dependency            | Purpose                            | Required Version | Recommended Version |
|-----------------------|------------------------------------|------------------|---------------------|
| Qt                    | GUI framework                      | 6.2.0+           | 6.6.0+              |
| OpenCASCADE (OCCT)    | Geometric modeling kernel          | 7.5.0+           | 7.6.0+              |
| pybind11              | Python bindings                    | 2.6.0+           | 2.10.0+             |
| Eigen                 | Mathematical operations            | 3.3+             | 3.4+                |
| GoogleTest            | Testing framework (dev only)       | 1.10.0+          | 1.12.0+             |
| CMake                 | Build system                       | 3.16+            | 3.24+               |

### 1.1 Qt Framework

Qt provides the GUI toolkit for IntuiCAM's user interface.

* **Minimum version**: 6.2.0 (Long Term Support)
* **Recommended version**: 6.6.0 or newer
* **Required components**:
  * QtCore, QtGui, QtWidgets (essential)
  * Qt3D (for 3D visualization)
  * QtConcurrent (for parallel processing)
  * QtTest (for GUI tests)

**Notes**: 
* Qt 5.15 is not officially supported but may work with minimal changes
* Qt modules should be consistent versions within a build

### 1.2 OpenCASCADE Technology (OCCT)

OCCT provides the core geometry processing for CAD operations.

* **Minimum version**: 7.5.0
* **Recommended version**: 7.6.0 or newer
* **Required modules**:
  * Foundation Classes (for basic types)
  * Modeling Algorithms (for geometry operations)
  * Data Exchange (for STEP import/export)
  * Visualization (for 3D display)

**Version compatibility notes**:
* OCCT 7.5.0 introduced key improvements in STEP handling
* OCCT 7.6.0 added performance improvements for boolean operations
* OCCT API changes between minor versions are typically minimal

### 1.3 pybind11

pybind11 enables exposing C++ functionality to Python.

* **Minimum version**: 2.6.0
* **Recommended version**: 2.10.0 or newer
* **Header-only library**: No binary compatibility concerns

**Notes**:
* Newer versions improve compilation times and Python 3.10+ support

### 1.4 Eigen

Eigen provides vector and matrix math for geometrical operations.

* **Minimum version**: 3.3
* **Recommended version**: 3.4 or newer
* **Header-only library**: No binary compatibility concerns

---

## 2. Platform-Specific Considerations

### 2.1 Windows

* **Compiler compatibility**:
  * Visual Studio 2019 (MSVC 16.8+) or newer
  * MinGW-w64 with GCC 9+ (alternative option)

* **OpenCASCADE specifics**:
  * Use pre-built binaries from official site or vcpkg
  * Ensure debug/release variants match your build configuration
  * Add OCCT's `bin` directory to PATH when running

* **Qt integration**:
  * Use the official Qt installer or vcpkg
  * Set `CMAKE_PREFIX_PATH` to point to Qt installation directory
  * For MSVC, ensure matching runtime library settings (MT/MD)

### 2.2 Linux

* **Compiler compatibility**:
  * GCC 9+ (recommended)
  * Clang 10+ (alternative)

* **Distribution-specific notes**:
  * Ubuntu/Debian: Consider PPA repositories for newer Qt versions
  * Fedora/RHEL: Use COPR repositories for OCCT if needed
  * Arch Linux: Latest versions available in main repositories

* **OpenCASCADE installation**:
  * Official repositories often have outdated versions
  * Building from source recommended (see [installation.md](installation.md))
  * Set `LD_LIBRARY_PATH` if installing to non-standard locations

### 2.3 macOS

* **Compiler compatibility**:
  * Apple Clang 12+ (from Xcode)
  * GCC via Homebrew (alternative)

* **Architecture considerations**:
  * Support both x86_64 and ARM64 (Apple Silicon)
  * Use universal builds when possible

* **Homebrew support**:
  * `brew install opencascade qt@6 eigen`
  * For development: `brew install googletest`

* **Framework integration**:
  * Qt should be used as framework on macOS
  * Handle @rpath integration for OCCT libraries

---

## 3. Dependency Management Approaches

### 3.1 Using vcpkg (Recommended for Windows)

vcpkg provides consistent dependency management across platforms.

1. **Setup vcpkg**:
   ```bash
   git clone https://github.com/microsoft/vcpkg
   ./vcpkg/bootstrap-vcpkg.sh   # or bootstrap-vcpkg.bat on Windows
   ```

2. **Install dependencies**:
   ```bash
   ./vcpkg/vcpkg install qt6 opencascade eigen3 pybind11 gtest
   ```

3. **Integrate with CMake**:
   ```bash
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

**vcpkg.json configuration**:
```json
{
  "name": "intuicam",
  "version-string": "1.0.0",
  "dependencies": [
    {
      "name": "qt",
      "version>=": "6.2.0",
      "default-features": false,
      "features": ["widgets", "gui", "3d"]
    },
    {
      "name": "opencascade",
      "version>=": "7.6.0"
    },
    "eigen3",
    "pybind11",
    {
      "name": "gtest",
      "host": true
    }
  ]
}
```

### 3.2 Using System Packages (Linux)

For Linux distributions, a combination of system packages and manual builds often works well.

1. **Install system packages**:
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install qtbase6-dev qttools6-dev libqt6opengl6-dev \
                    libqt63dcore6 libqt63drender6 libqt63dextras6 \
                    libeigen3-dev python3-dev
   
   # Fedora
   sudo dnf install qt6-qtbase-devel qt6-qt3d-devel \
                    eigen3-devel python3-devel
   ```

2. **Build and install OpenCASCADE**:
   ```bash
   git clone https://git.dev.opencascade.org/repos/occt.git
   cd occt
   git checkout V7_6_0
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
   make -j$(nproc)
   sudo make install
   ```

3. **Configure CMake**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_PREFIX_PATH="/usr/local;/usr"
   ```

### 3.3 Using Conan

Conan is an alternative package manager that can be used for dependency management.

1. **Setup Conan**:
   ```bash
   pip install conan
   ```

2. **Create conanfile.txt**:
   ```
   [requires]
   qt/6.6.0
   opencascade/7.6.0
   eigen/3.4.0
   pybind11/2.10.0
   gtest/1.12.1
   
   [generators]
   CMakeDeps
   CMakeToolchain
   ```

3. **Install dependencies**:
   ```bash
   conan install . --build=missing
   ```

4. **Configure CMake**:
   ```bash
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
   ```

---

## 4. Dependency Tracking and Updates

### 4.1 Versioning Strategy

* **Explicit requirements**: Always specify minimum required versions in documentation
* **Compatibility testing**: Regularly test with newest versions of dependencies
* **Version bumping**: Update requirements when utilizing new features
* **Long-term support**: Prefer LTS versions for critical dependencies

### 4.2 Continuous Integration

* **Matrix testing**: Test with multiple dependency versions in CI
* **Compatibility warnings**: Flag issues with specific dependency versions
* **Bundle verification**: Verify correct dependency bundling in packaged builds

Sample GitHub Actions CI configuration:

```yaml
# Dependency compatibility testing
dependency-matrix:
  runs-on: ${{ matrix.os }}
  strategy:
    matrix:
      os: [ubuntu-latest, windows-latest, macos-latest]
      qt-version: ['6.2.4', '6.6.0']
      occt-version: ['7.5.3', '7.6.0']
  
  steps:
    # Setup and test with specific versions
    - name: Install Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: ${{ matrix.qt-version }}
    
    # Similar steps for other dependencies
```

---

## 5. Best Practices

### 5.1 Dependency Isolation

* **Isolation approaches**:
  * Prefer shallow dependencies (few layers of transitive dependencies)
  * Wrap third-party APIs in adapter classes
  * Create abstraction layers for swappable implementations
  * Use feature detection rather than version checks

* **Example adapter pattern**:
  ```cpp
  // Instead of direct OCCT dependency:
  class GeometryEngine {
  public:
    virtual ~GeometryEngine() = default;
    virtual Solid importStep(const std::string& filename) = 0;
    // Other operations...
  };
  
  // OCCT implementation:
  class OCCTGeometryEngine : public GeometryEngine {
  public:
    Solid importStep(const std::string& filename) override {
      // OCCT-specific implementation
    }
  };
  
  // Mock implementation for testing:
  class MockGeometryEngine : public GeometryEngine {
  public:
    Solid importStep(const std::string& filename) override {
      // Test implementation
    }
  };
  ```

### 5.2 Version Compatibility

* **Compatibility checks**:
  ```cpp
  // Qt version check
  #if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    // Use Qt 6.2+ features
  #else
    // Fallback implementation
  #endif
  
  // OCCT version check
  #ifdef OCCT_VERSION_MAJOR
  #if (OCCT_VERSION_MAJOR > 7 || (OCCT_VERSION_MAJOR == 7 && OCCT_VERSION_MINOR >= 6))
    // Use OCCT 7.6+ features
  #else
    // Fallback implementation
  #endif
  #endif
  ```

* **Feature detection**:
  ```cpp
  template <typename T>
  using has_volume_t = decltype(std::declval<T>().Volume());
  
  template <typename T>
  constexpr bool has_volume_v = is_detected_v<has_volume_t, T>;
  
  template <typename ShapeType>
  double get_volume(const ShapeType& shape) {
    if constexpr (has_volume_v<ShapeType>) {
      return shape.Volume();
    } else {
      return fallback_volume_calculation(shape);
    }
  }
  ```

### 5.3 Dependency Bundling

For distribution, consider the following bundling approaches:

* **Windows**: 
  * Bundle dependencies using `windeployqt` for Qt
  * Include all necessary OCCT DLLs
  * Consider using NSIS installer with dependency checks

* **Linux**:
  * Create AppImage, Flatpak, or Snap package
  * Specify dependency requirements in package metadata
  * Use RPATH with `$ORIGIN` for local library resolution

* **macOS**:
  * Create a proper .app bundle with `macdeployqt`
  * Use @rpath for framework and library resolution
  * Consider code signing and notarization requirements

---

## 6. Troubleshooting

### 6.1 Common Issues

* **Library not found errors**:
  * Ensure library search paths are correctly set
  * Windows: Add DLL directories to PATH
  * Linux: Set LD_LIBRARY_PATH or use rpath
  * macOS: Check @rpath and framework paths

* **Version mismatch errors**:
  * Check for multiple versions of same library in path
  * Verify consistent debug/release configurations
  * Check for incompatible ABI changes

* **Build errors**:
  * Ensure compiler meets minimum requirements
  * Check for missing development packages
  * Verify compatible build flags between dependencies

### 6.2 Diagnostic Tools

* **Windows**:
  * Dependency Walker or Process Explorer to check loaded DLLs
  * CMake's `file(GET_RUNTIME_DEPENDENCIES)` command

* **Linux**:
  * `ldd` to check shared library dependencies
  * `ldconfig -p` to see library cache
  * `LD_DEBUG=libs` environment variable for detailed loading info

* **macOS**:
  * `otool -L` to check dynamic library dependencies
  * `install_name_tool` to fix library paths

---

By following these guidelines and best practices, IntuiCAM can maintain a robust dependency management strategy that ensures compatibility across platforms and simplifies development. 