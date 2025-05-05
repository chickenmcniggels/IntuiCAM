# IntuiCAM

[![Build Status](https://img.shields.io/github/actions/workflow/status/your-org/IntuiCAM/ci.yml?branch=main)](https://github.com/your-org/IntuiCAM/actions)  
[![License: MIT](https://img.shields.io/badge/License-MIT-blue)](LICENSE)

**IntuiCAM** is an open-source CAM application initially focused on CNC turning. It will provide an intuitive desktop GUI for generating reliable toolpaths—and a standalone **C++ Core library** for CAM processing, complete with Python bindings to enable seamless integration in other workflows and applications.

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
- CMake ≥ 3.16  
- Qt 5.15+ (or Qt 6)  
- OpenCASCADE (OCCT)  
- pybind11  

### Build & Install

```bash
git clone https://github.com/your-org/IntuiCAM.git
cd IntuiCAM

# Create build directory
mkdir build && cd build

# Configure and build Core and GUI
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# (Optional) Install to system
cmake --install .
