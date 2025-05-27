# IntuiCAM

[![Build Status](https://img.shields.io/github/actions/workflow/status/your-org/IntuiCAM/ci.yml?branch=main)](https://github.com/your-org/IntuiCAM/actions)  
[![License: MIT](https://img.shields.io/badge/License-MIT-blue)](LICENSE)

**IntuiCAM** will be an open-source CAM application initially focused on CNC turning. It will provide an intuitive desktop GUI for generating reliable toolpaths—and a standalone **C++ Core library** for CAM processing, complete with Python bindings to enable seamless integration in other workflows and applications.

---

## Key Features

- **Modular C++ Core**  
  A standalone library (`libIntuiCAMCore`) implements all core CAM algorithms (facing, roughing, finishing, parting) in high-performance C++.  

- **Python Bindings**  
  Expose core functionality via Python (using pybind11) for scripting, automation, or embedding in other projects (e.g. FreeCAD, Slicer tools).

- **Cross-Platform GUI (Qt)**  
  A native desktop interface (Windows, macOS, Linux) guides you step-by-step from STEP import to G-code export.

- **STEP Import & B-Rep Accuracy**  
  Leverage OpenCASCADE to load exact CAD geometry, define spindle axis interactively, and configure stock and chucks.

- **G-Code Generation & Post-Processing**  
  Produce ISO-compliant G-code for common CNC controls. Flexible post-processor layer accommodates custom machine dialects.

- **Extensible & Plugin-Friendly**  
  Core and GUI are cleanly separated. Contributors can add new operations, tools or backends without touching unrelated code.

---

## Getting Started

### Prerequisites

- C++17-capable compiler (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 2019)  
- CMake 3.16+
- Qt 6.2+ (tested with Qt 6.9.0)
- OpenCASCADE 7.5.0+ (tested with 7.6.0) - Note: Recent versions have reorganized library names
- VTK 9.4+ (included with OpenCASCADE)
- pybind11 (optional, for Python bindings)

### Quick Start

For detailed installation instructions, see [docs/installation.md](docs/installation.md).  
For common build and runtime issues, see [docs/troubleshooting.md](docs/troubleshooting.md).

### Current Status

✅ **Building Successfully** - All core modules and GUI compile without errors  
✅ **Basic GUI Running** - Main window with professional CAM application layout  
✅ **Project Structure** - Modular architecture with Qt6, OpenCASCADE, and VTK integration  
✅ **OpenCASCADE Integration** - Updated to use current library names (TKDESTEP, TKDEIGES, etc.)  
✅ **3D Visualization Ready** - OpenGL widget with STEP file import capability  

#### Windows (Tested Configuration)

**Requirements:**
- Visual Studio 2019/2022 with C++ workload
- Qt 6.9.0 for MSVC 2022 (installed to `C:/Qt/6.9.0/msvc2022_64`)
- OpenCASCADE 7.6.0 with 3rdparty dependencies (installed to `C:/OpenCASCADE/`)

```powershell
git clone https://github.com/your-org/IntuiCAM.git
cd IntuiCAM

# Create and enter build directory
mkdir build
cd build

# Configure (CMake will find dependencies automatically)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build all targets
cmake --build . --config Release

# Run the applications (Note: All required DLLs are automatically copied)
.\Release\IntuiCAMGui.exe    # Qt GUI Application (self-contained with all DLLs)
.\Release\IntuiCAMCli.exe    # Command Line Interface
```

**Note:** All required runtime DLLs are automatically copied during build, including Qt6, OpenCASCADE 3rd party libraries (TBB, FreeImage, FFmpeg, OpenVR, Jemalloc, etc.), and VTK libraries. No additional PATH configuration is required.

### Current GUI Features

The main window now includes:
- **Professional Layout**: Menu bar, toolbar, status bar, and splitter-based layout
- **Project Tree**: Hierarchical view of CAM projects (Parts, Tools, Operations)
- **3D Viewport**: Full OpenCASCADE integration with 3-jaw chuck management
- **Properties Panel**: Context-sensitive properties display
- **Output Log**: Application messages and operation feedback
- **Menu System**: File, Edit, View, Tools, and Help menus with shortcuts
- **About Dialog**: Application information and dependency versions

#### Chuck Management Features

- **Automatic Chuck Display**: 3-jaw chuck STEP file permanently displayed
- **Cylinder Detection**: Automatic detection of cylindrical features in workpieces
- **Standard Material Matching**: ISO metric standard stock diameter database
- **Raw Material Visualization**: Transparent raw material cylinder with automatic alignment
- **Workpiece Alignment**: Intelligent positioning relative to chuck axis

#### Linux/macOS

```bash
git clone https://github.com/your-org/IntuiCAM.git
cd IntuiCAM

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Run the applications
./bin/IntuiCAMGui    # Qt GUI Application  
./bin/IntuiCAMCli    # Command Line Interface
```
