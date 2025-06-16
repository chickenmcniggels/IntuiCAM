# IntuiCAM

[![Build Status](https://img.shields.io/github/actions/workflow/status/your-org/IntuiCAM/ci.yml?branch=main)](https://github.com/your-org/IntuiCAM/actions)  
[![License: MIT](https://img.shields.io/badge/License-MIT-blue)](LICENSE)

**IntuiCAM** is an open-source CAM project in **active development**. The goal is to deliver an intuitive, cross-platform solution for CNC turning. A modular **C++ core library** drives the toolpath algorithms while a Qt-based GUI guides you from STEP import to G-code export. Python bindings allow integration and automation.
> **Project Status:** Pre-release / development in progress. Expect missing features and rapid changes.

---
## Project Goals
- Provide a modern, free CAM solution focused on CNC turning
- Offer an intuitive workflow from CAD import to optimized G-code
- Remain extensible via plugins and Python scripting
- Explore future automation with AI-assisted features


## Key Features

- **Modular C++ Core**
  High-performance library implementing lathe toolpath algorithms (facing, roughing, finishing, parting).
- **Python Bindings**
  Access core CAM functionality from Python for scripting or integration in other tools (pybind11).
- **Cross-Platform GUI (Qt)**
  Desktop interface for Windows, macOS and Linux that guides you from model import to G-code output.
- **STEP Import & B-Rep Accuracy**
  Uses OpenCASCADE to load precise CAD geometry and configure spindle axis, stock and chuck.
- **G-Code Generation & Post-Processing**
  Generates ISO-compliant programs with a flexible post-processing layer for custom machine dialects.
- **Extensible & Plugin-Friendly**
  Clean separation of core and GUI allows new operations or backends to be added with minimal changes.
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

#### Windows (via vcpkg)

```powershell
git clone https://github.com/your-org/IntuiCAM.git
cd IntuiCAM
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat

cmake --preset ninja-release
cmake --build --preset ninja-release

.\build_ninja\IntuiCAMGui.exe    # Qt GUI Application
```

**Note:** vcpkg installs all runtime dependencies and `windeployqt` is invoked automatically during the build, so no manual DLL copying or PATH setup is necessary.

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
git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.sh

cmake --preset ninja-release
cmake --build --preset ninja-release

./build_ninja/IntuiCAMGui    # Qt GUI Application
```
