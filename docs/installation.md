# Installation

This guide shows how to get IntuiCAM up and running on different platforms with specific attention to compatibility of dependencies. For development setup, see the [Development Guide](development.md).

---

## 1. Prerequisites

IntuiCAM requires the following dependencies with their recommended minimum versions:

| Dependency          | Minimum Version | Recommended Version | Purpose                          |
|---------------------|-----------------|---------------------|----------------------------------|
| C++ Compiler        | C++17 support   | GCC 10+, MSVC 2019+ | Core build                       |
| CMake               | 3.16            | 3.24+               | Build system                     |
| Qt                  | 6.2             | 6.6+                | GUI framework                    |
| OpenCASCADE (OCCT)  | 7.5.0           | 7.6.0+              | 3D modeling kernel               |
| pybind11            | 2.6.0           | 2.10.0+             | Python bindings                  |
| Eigen               | 3.3             | 3.4+                | Math operations                  |

### 1.1 Version Compatibility Notes

* **Qt Compatibility**:
  * Qt 6.2 is the minimum supported version with Long-Term Support (LTS)
  * Qt 6.6+ is recommended for the best experience and latest features
  * Qt 5.15 may work but is not officially supported

* **OpenCASCADE Compatibility**:
  * OCCT 7.5.0 is the minimum version with the necessary B-Rep functionality
  * OCCT 7.6.0+ is recommended for improved performance and stability
  * OCCT must be built with the Standard and Modeling modules

* **Compiler Requirements**:
  * GCC 9+ on Linux/macOS
  * Clang 10+ on Linux/macOS
  * MSVC 2019+ (v16.8+) on Windows
  * Apple Clang 12+ on macOS

---

## 2. Windows Installation

### 2.1 Using vcpkg (Recommended)

vcpkg is the recommended way to manage dependencies on Windows as it ensures consistent versions.

1. **Install Prerequisites**:
   * [Visual Studio 2019/2022](https://visualstudio.microsoft.com/downloads/) with C++ workload
   * [Git](https://git-scm.com/download/win)
   * [CMake](https://cmake.org/download/) (3.16+)

2. **Install vcpkg**:
   ```powershell
   git clone https://github.com/microsoft/vcpkg
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```

3. **Clone IntuiCAM**:
   ```powershell
   git clone https://github.com/your-org/IntuiCAM.git
   cd IntuiCAM
   ```

4. **Build with vcpkg integration**:
   ```powershell
   cmake --preset ninja-release
   cmake --build --preset ninja-release
   ```

### 2.2 Manual Installation (Tested Configuration)

**Verified Working Setup on Windows 10/11:**

1. **Install Visual Studio 2022**: 
   * Download from [Microsoft](https://visualstudio.microsoft.com/downloads/)
   * Include "Desktop development with C++" workload

2. **Install Qt 6.9.0**:
   * Download from [Qt Online Installer](https://www.qt.io/download-qt-installer)
   * Install to `C:\Qt\6.9.0\msvc2022_64`
   * Select the MSVC 2022 64-bit component

3. **Install OpenCASCADE 7.6.0**:
   * Download from [OpenCASCADE website](https://dev.opencascade.org/release)
   * Install the complete package with 3rdparty dependencies to `C:\OpenCASCADE\`
   * Ensure the following directories exist:
     - `C:\OpenCASCADE\occt-vc144-64-with-debug\`
     - `C:\OpenCASCADE\3rdparty-vc14-64\`

4. **Build IntuiCAM**:
   ```powershell
   git clone https://github.com/your-org/IntuiCAM.git
   cd IntuiCAM
   mkdir build
   cd build
   
   # Configure (paths are automatically detected)
   cmake .. -DCMAKE_BUILD_TYPE=Release
   
   # Build all components
   cmake --build . --config Release
   
   # Test the build
   .\Release\IntuiCAMCli.exe --help
   .\Release\IntuiCAMGui.exe
   ```

**Important Notes**:
- vcpkg installs all dependencies automatically and `windeployqt` runs during the build.
- No manual copying of DLLs or PATH configuration is required.

---

## 3. Linux Installation

### 3.1 Ubuntu/Debian

1. **Install System Dependencies**:
   ```bash
   sudo apt update
   sudo apt install build-essential git cmake python3-dev libgl1-mesa-dev
   ```

2. **Install Qt**:
   ```bash
   sudo apt install qt6-base-dev qt6-declarative-dev
   # If Qt6 is not available in your repositories:
   # sudo add-apt-repository ppa:beineri/opt-qt-6.6.0-jammy
   # sudo apt update
   # sudo apt install qt6-base-dev
   ```

3. **Install OpenCASCADE**:
   OpenCASCADE is not typically available in standard repositories with recent versions.
   
   ```bash
   # Build from source (recommended for latest version)
   git clone https://git.dev.opencascade.org/repos/occt.git
   cd occt
   git checkout V7_6_0
   mkdir build && cd build
   cmake .. -DBUILD_LIBRARY_TYPE=Shared -DCMAKE_BUILD_TYPE=Release -DBUILD_RELEASE_DISABLE_EXCEPTIONS=OFF
   make -j$(nproc)
   sudo make install
   ```

4. **Build IntuiCAM**:
   ```bash
   git clone https://github.com/your-org/IntuiCAM.git
   cd IntuiCAM
   git clone https://github.com/microsoft/vcpkg
   ./vcpkg/bootstrap-vcpkg.sh
   cmake --preset ninja-release
   cmake --build --preset ninja-release
   ```

### 3.2 Fedora/RHEL

1. **Install System Dependencies**:
   ```bash
   sudo dnf install gcc-c++ git cmake python3-devel mesa-libGL-devel
   ```

2. **Install Qt**:
   ```bash
   sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel
   ```

3. **For OpenCASCADE**, follow the build from source instructions in the Ubuntu section.

4. **Build IntuiCAM** following the same steps as Ubuntu.

---

## 4. macOS Installation

1. **Install Prerequisites with Homebrew**:
   ```bash
   brew install cmake python qt@6
   ```

2. **Install OpenCASCADE**:
   ```bash
   brew install opencascade
   ```

3. **Build IntuiCAM**:
   ```bash
   git clone https://github.com/your-org/IntuiCAM.git
   cd IntuiCAM
   git clone https://github.com/microsoft/vcpkg
   ./vcpkg/bootstrap-vcpkg.sh
   cmake --preset ninja-release
   cmake --build --preset ninja-release
   ```

---

## 5. Testing Your Installation

After installation, verify that everything works correctly:

1. **Run the tests**:
   ```bash
   cd build
   ctest --output-on-failure
   ```

2. **Launch the application**:
   * On Windows: `.\bin\Release\IntuiCAM_GUI.exe`
   * On Linux/macOS: `./bin/IntuiCAM_GUI`

---

## 6. Troubleshooting

### 6.1 Common Issues

* **CMake can't find Qt**:
  - Ensure Qt is installed to `C:/Qt/6.9.0/msvc2022_64` or update the path in CMakeLists.txt
  - Check that you're using a compatible Qt version (6.2+)
  - Verify the MSVC 2022 64-bit component is installed

* **OpenCASCADE linking errors (RESOLVED)**:
  - ✅ **jemalloc.lib path errors**: Fixed in CMakeLists.txt with automatic path correction
  - ✅ **Missing TKAIS.lib**: Removed non-existent library reference
  - ✅ **Missing 3rdparty libraries**: Added all required 3rdparty library paths
  - Ensure OpenCASCADE is installed to `C:/OpenCASCADE/` with 3rdparty dependencies

* **Missing library errors**:
  - ✅ **openvr_api.lib**: Added path `C:/OpenCASCADE/3rdparty-vc14-64/openvr-1.14.15-64/lib/win64`
  - ✅ **FreeImage.lib**: Added path `C:/OpenCASCADE/3rdparty-vc14-64/freeimage-3.18.0-x64/lib`
  - ✅ **FFmpeg, TBB, Tcl/Tk libraries**: All 3rdparty paths added to CMakeLists.txt

* **Compilation errors**:
  - Ensure your compiler supports C++17
  - Use Visual Studio 2019/2022 for best compatibility
  - Check for compatibility issues between OCCT and your compiler

* **Vulkan headers warning**:
  - This is a non-fatal Qt warning and can be ignored
  - IntuiCAM builds successfully without Vulkan support

### 6.2 Getting Help

If you encounter issues not covered here:
* Check the [GitHub Issues](https://github.com/your-org/IntuiCAM/issues) for similar problems
* Open a new issue with detailed information about your system and the error
* Join our community channels for direct assistance

---

