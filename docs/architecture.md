# Architecture Overview

This document outlines the high-level design and module structure of **IntuiCAM**, ensuring a clear separation of concerns, reusability of the CAM core, and a contributor-friendly layout.

---

## 1. Goals and Principles

* **Modularity**: Each component (Core, GUI, Plugins, etc.) lives in its own folder and CMake target.
* **Reusability**: The CAM Core is a standalone C++ library, embeddable in other applications (e.g., FreeCAD, future Slicers).
* **Clarity**: A minimal, self-explanatory directory layout and clear API boundaries.
* **Contributor-Friendly**: Small, focused modules; consistent naming; in-repo documentation; CI enforcement of style and tests.
* **Performance**: Core implemented in modern C++17/20, optimized for turning operations.

---

## 2. Repository Structure

```
IntuiCAM/                   # Root of the repo
├── core/                   # C++ CAM Engine (modular libraries)
│   ├── common/             # Shared types and utilities
│   │   ├── include/        # Public headers
│   │   │   ├── IntuiCAM/
│   │   │   │   ├── Common/
│   │   │   │   │   ├── Types.h
│   │   │   │   │   └── ...
│   │   ├── src/           # Implementation files
│   │   └── tests/         # Common-specific unit tests
│   ├── geometry/          # Geometry handling and STEP import
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── toolpath/          # Toolpath generation algorithms
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── postprocessor/     # G-code generation
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── simulation/        # Material removal simulation
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   └── python/            # Python bindings for all core modules
│       ├── common/
│       ├── geometry/
│       ├── toolpath/
│       └── ...
├── gui/                    # Qt-based frontend application
│   ├── src/                # Widgets, viewports, controllers
│   └── resources/          # UI files, icons, translations
├── cli/                    # Command-line interface
│   ├── src/
│   └── tests/
├── plugins/                # (Optional) Built-in or downloaded extensions
├── docs/                   # Documentation (Markdown files)
│   ├── architecture.md     # ← This file
│   └── ...                 # Other docs as per docs/ skeleton
├── tests/                  # Integration and end-to-end tests
├── cmake/                  # CMake modules and helper scripts
├── .github/                # GitHub Actions workflows and issue templates
├── examples/               # Sample projects and usage demos
├── vcpkg.json              # Dependency specification
└── CMakeLists.txt          # Top-level CMake configuration
```

* **Mono-repo**: All code, docs, CI, and examples in one repository, with clear subfolders.
* **CMake targets**: Multiple modular libraries (`intuicam-geometry`, `intuicam-toolpath`, etc.), GUI executable (`IntuiCAMGui`), CLI tool (`IntuiCAMCli`), and individual `Plugin` targets.

---

## 3. Module Breakdown

### 3.1 Core Libraries (`/core`)

* **Language**: C++17/20
* **Modular Architecture**:
  * **`common`**: Shared data structures, utilities, and interfaces
  * **`geometry`**: STEP import and B-Rep handling (via OpenCASCADE)
  * **`toolpath`**: Lathe operations algorithms (Facing, Roughing, Finishing, Parting)
  * **`postprocessor`**: G-code generation and machine-specific adaptations
  * **`simulation`**: Material removal simulation
* **Design**:
  * Namespace `IntuiCAM::<Module>` (e.g., `IntuiCAM::Geometry`)
  * Each module compiles to a separate library with clean dependency tracking
  * Public API headers in `<module>/include/IntuiCAM/<Module>`
  * pybind11-based Python bindings for all modules

### 3.2 Python Bindings (`/core/python`)

* **Framework**: pybind11
* **Structure**: One module per C++ library
* **Advantages**:
  * Integration with FreeCAD via Python API
  * User scripting and automation
  * Rapid prototyping
  * Testing flexibility
  * Plugin architecture
* **Considerations**:
  * Memory management via smart pointers
  * Pythonic interfaces
  * Comprehensive docstrings
  * Careful type conversion

### 3.3 GUI Frontend (`/gui`)

* **Framework**: Qt6 + OpenCASCADE visualization
* **Responsibilities**:
  * 3D viewport (using OpenCASCADE AIS)
  * Interactive setup wizard (import → axis → stock → tools → operations)
  * Parameter panels and real-time preview
  * Progress dialogs and error reporting
* **Design**:
  * Model–View–Controller (MVC) pattern
  * Communicates with Core via C++ API

### 3.4 Simulation & Verification

* **Path** (`/core/simulation`)
* **Responsibilities**:
  * Stepped material-ablation simulation (2D bitmap or voxel-based)
  * Collision detection (tool vs. stock vs. chuck)
  * Visualization mesh output

### 3.5 Plugin & Scripting (`/plugins` + Core bindings)

* **Mechanism**:
  * C++ plugins via `QPluginLoader` or custom dynamic loading
  * Python scripting interface using pybind11 (embedded Python interpreter)
* **Use-cases**:
  * Custom postprocessors
  * Automated feature recognition (future AI helpers)
  * Integration as a FreeCAD Workbench via Python

### 3.6 Cloud Integration (future)

* **Client module** (`/core/cloud`) with a clean interface
* **Responsibilities**:
  * Send geometry and parameters to AI services
  * Receive suggested operation sequences
  * Optional: usage-based billing hooks
* **Design**: Decoupled; Core functionality must work offline.

### 3.7 Command-Line Interface (`/cli`)

* **Target**: `IntuiCAMCli`
* **Responsibilities**: Headless toolpath generation, batch workflows, CI integration
* **Design**: Based on Core API; lightweight argument parsing (e.g., CLI11)

---

## 4. Data Types and Interfaces

### 4.1 Core Data Types

* **Common Data Structures**:
  * `Point3D`, `Vector3D`, `Matrix4x4` - Basic geometric primitives
  * `BoundingBox` - Spatial containment and intersection
  * `Mesh` - Triangulated geometry for visualization and simulation
  * `GeometricEntity` - Base class for all geometric objects
  * `Part` - Complete part or assembly with topology
  * `Tool` - Tool definition with geometry and cutting parameters
  * `Operation` - Base class for machining operations
  * `Toolpath` - Sequence of movements with types and parameters

* **Design Principles**:
  * Serializable to/from JSON for persistence and cross-language communication
  * Smart pointers for memory management
  * Immutable where appropriate, with builder patterns
  * Convertible to/from OpenCASCADE types

### 4.2 External Integration Types

* **OpenCASCADE Adapters**:
  * Conversion functions between IntuiCAM and OCCT types
  * STEP import/export functions
  * Mesh generation utilities

* **FreeCAD Integration**:
  * Python functions to convert FreeCAD objects to IntuiCAM types
  * Workbench implementation using IntuiCAM Python modules
  * Path object generators

* **Visualization Types**:
  * Conversion to/from OpenCASCADE AIS objects
  * Optional Qt3D representations

---

## 5. Data Flow Overview

1. **Import**: User loads a STEP file → Core geometry module parses into B-Rep model.
2. **Setup**: User defines rotation axis, stock, and chuck in GUI → Core stores `Setup` object.
3. **Tool & Operation**: GUI or script configures tools and operations → Core validates and attaches to project model.
4. **Compute**: GUI calls appropriate toolpath generation functions → Core computes and stores toolpaths.
5. **Preview**: GUI queries Core for mesh & curve data → 3D viewport renders model + toolpaths.
6. **Export**: GUI or CLI invokes postprocessor → G-Code is formatted and written to file.

---

## 6. Build System and Dependency Management

### 6.1 CMake Structure

* **Top-level `CMakeLists.txt`**:
  * Project definition and global options
  * Find required packages (OpenCASCADE, Qt6, pybind11, etc.)
  * Include subdirectories for each module
  * Installation rules

* **Module-specific `CMakeLists.txt`**:
  * Library/executable definition
  * Include directories
  * Link dependencies
  * Export headers
  * Installation rules

* **Options**:
  * `INTUICAM_BUILD_GUI` - Build the Qt GUI application
  * `INTUICAM_BUILD_CLI` - Build the command-line interface
  * `INTUICAM_BUILD_PYTHON` - Build Python bindings
  * `INTUICAM_BUILD_TESTS` - Build unit tests

### 6.2 Dependency Management with vcpkg

* **`vcpkg.json`** for dependency specification:
  * Qt6 (for GUI)
  * OpenCASCADE (core geometry)
  * pybind11 (Python bindings)
  * Eigen3 (math operations)

* **CMake presets** for build configurations:
  * Development preset with vcpkg toolchain
  * Release preset for optimized builds
  * CI preset for automated testing

* **Docker development containers** for consistent environments:
  * Development container with all dependencies
  * CI container for automated builds
  * Lightweight distribution container

### 6.3 CI Integration

* **GitHub Actions**:
  * Build on Windows, Linux, macOS
  * Run unit tests and integration tests
  * Code style checks with clang-format and clang-tidy
  * Documentation generation

---

## 7. Visualization Strategy

### 7.1 OpenCASCADE Visualization (Primary)

* **Benefits**:
  * Direct integration with OpenCASCADE geometry
  * No format conversions needed
  * Specialized for CAD visualization
  * Already required as a dependency

* **Implementation**:
  * AIS_InteractiveContext for display management
  * Custom AIS_InteractiveObject subclasses for IntuiCAM entities
  * Integration with Qt6 via OpenGL widget

### 7.2 Qt6 3D Framework (Secondary)

* **Use cases**:
  * UI elements and overlays
  * Non-CAD visualizations (graphs, charts)
  * Mobile version (future)

* **Integration**:
  * Conversion from IntuiCAM types to Qt3D entities
  * Shared OpenGL context with OpenCASCADE view

### 7.3 Coin3D (Optional)

* **Only if needed for FreeCAD integration**
* **Alternative**: Direct OpenCASCADE communication via Python

---

## 8. FreeCAD Integration

### 8.1 Integration Strategy

* **Python Workbench**:
  * Register commands in FreeCAD interface
  * Provide UI dialogs for operation configuration
  * Convert between FreeCAD and IntuiCAM types

* **Data Conversion**:
  * STEP file as intermediary (robust but slower)
  * Direct OCCT shape transfer (faster but requires version compatibility)
  * Toolpath to FreeCAD Path object conversion

### 8.2 Implementation Approach

* **Python module structure**:
  * `intuicam_freecad/` - FreeCAD-specific wrapper
  * `intuicam_freecad/commands/` - UI command implementations
  * `intuicam_freecad/conversion/` - Type conversion utilities

* **User workflow**:
  1. Select part in FreeCAD
  2. Configure turning operation in IntuiCAM workbench
  3. Generate toolpath
  4. Optionally simulate using IntuiCAM's simulation
  5. Create FreeCAD Path object for further processing

---

## 9. Key Design Patterns & Practices

* **SOLID Principles**: Single Responsibility for classes, Open/Closed for extensions
* **Observer/Signal–Slot** for UI updates on model changes
* **Factory** for creating operations/toolpath strategies
* **Strategy** for interchangeable CAM algorithms
* **Facade** for exposing Core API to GUI/CLI
* **Builder** for constructing complex objects
* **Adapter** for converting between different type systems

---

By following this architecture, IntuiCAM achieves a balance of **clarity**, **extensibility**, and **performance**, while remaining **inviting** for new contributors and future integrations into other platforms.
