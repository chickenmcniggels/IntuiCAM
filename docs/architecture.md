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
├── core/                   # C++ CAM Engine (library)
│   ├── include/            # Public headers (Core API)
|   │   ├── IntuiCAMCore/
|   |   │   ├── io/
|   |   |   │   ├── StepLoader.h
│   |   |   |   └── ...
|   |   │   ├── path/
|   |   |   │   ├── RoughingTurningPath.h
│   |   |   |   └── ...
|   |   │   ├── Core.h
│   ├── src/                # Implementation files
|   │   ├── io/
|   |   │   ├── StepLoader.cpp
|   |   │   └── ...
│   └── tests/              # Core-specific unit tests
├── gui/                    # Qt-based frontend application
│   ├── src/                # Widgets, viewports, controllers
│   └── resources/          # UI files, icons, translations
├── plugins/                # (Optional) Built-in or downloaded extensions
├── docs/                   # Documentation (Markdown files)
│   ├── architecture.md     # ← This file
│   └── ...                 # Other docs as per docs/ skeleton
├── tests/                  # Integration and end-to-end tests
├── cmake/                  # CMake modules and helper scripts
├── .github/                # GitHub Actions workflows and issue templates
├── examples/               # Sample projects and usage demos
└── CMakeLists.txt          # Top-level CMake configuration
```

* **Mono-repo**: All code, docs, CI, and examples in one repository, with clear subfolders.
* **CMake targets**: `IntuiCAMCore` (library), `IntuiCAMGui` (executable), `IntuiCAMCli` (CLI tool), and individual `Plugin` targets.

---

## 3. Module Breakdown

### 3.1 Core Engine (`/core`)

* **Language**: C++17/20
* **Responsibilities**:

  * STEP import and B-Rep handling (via OpenCASCADE)
  * Lathe operations: Facing, Roughing, Finishing, Parting
  * Toolpath generation and Post-Processing (generic G‑Code)
  * Public API headers in `core/include/IntuiCAM/Core`
* **Design**:

  * Namespace `IntuiCAM::Core`
  * Separate submodules: `model`, `toolpath`, `postprocessor`, `serialization`
  * pybind11-based Python bindings (in `core/bindings/`)

### 3.2 GUI Frontend (`/gui`)

* **Framework**: Qt
* **Responsibilities**:

  * 3D viewport (OpenCASCADE Viewer or QOpenGLWidget)
  * Interactive setup wizard (import → axis → stock → tools → operations)
  * Parameter panels and real-time preview
  * Progress dialogs and error reporting
* **Design**:

  * Model–View–Controller (MVC) pattern
  * Communicates with Core via C++ API

### 3.3 Plugin & Scripting (`/plugins` + Core bindings)

* **Mechanism**:

  * C++ plugins via `QPluginLoader` or custom dynamic loading
  * Python scripting interface using pybind11 (embedded Python interpreter)
* **Use-cases**:

  * Custom postprocessors
  * Automated feature recognition (future AI helpers)
  * Integration as a FreeCAD Workbench via Python

### 3.4 Cloud Integration (future)

* **Client module** (`/core/cloud`) with a clean interface
* **Responsibilities**:

  * Send geometry and parameters to AI services
  * Receive suggested operation sequences
  * Optional: usage-based billing hooks
* **Design**: Decoupled; Core functionality must work offline.

### 3.5 Simulation & Verification (future)

* **Plugin module** (`/plugins/simulator`)
* **Responsibilities**:

  * Stepped material-ablation simulation (2D bitmap or voxel-based)
  * Collision detection (tool vs. stock vs. chuck)
  * Visualization mesh output

### 3.6 Command-Line Interface (CLI)

* **Target**: `IntuiCAMCli`
* **Responsibilities**: Headless toolpath generation, batch workflows, CI integration
* **Design**: Based on Core API; lightweight argument parsing (e.g., CLI11)

---

## 4. Data Flow Overview

1. **Import**: User loads a STEP file → Core parses into B-Rep model.
2. **Setup**: User defines rotation axis, stock, and chuck in GUI → Core stores `Setup` object.
3. **Tool & Operation**: GUI or script configures tools and operations → Core validates and attaches to project model.
4. **Compute**: GUI calls `Core.generateAllToolpaths()` → Core computes and stores toolpaths.
5. **Preview**: GUI queries Core for mesh & curve data → 3D viewport renders model + toolpaths.
6. **Export**: GUI or CLI invokes `Core.exportGCode()` → Postprocessor formats G-Code and writes file.

---

## 5. Build & CI Integration

* **CMake** is the build system: top-level `CMakeLists.txt` includes subdirectories.
* **Targets**:

  * `IntuiCAMCore` (STATIC/PUBLIC)
  * `IntuiCAMGui` (EXE, links to Core)
  * `IntuiCAMCli` (EXE, links to Core)
  * Individual plugin/shared-library targets
* **CI** (.github/workflows/ci.yml):

  * Build on Windows, Linux, macOS
  * Run unit tests (`core/tests/*`) and integration tests (`tests/*`)
  * clang-format and clang-tidy checks

---

## 6. Key Design Patterns & Practices

* **SOLID Principles**: Single Responsibility for classes, Open/Closed for extensions
* **Observer/Signal–Slot** for UI updates on model changes
* **Factory** for creating operations/toolpath strategies
* **Strategy** for interchangeable CAM algorithms
* **Facade** for exposing simplified Core API to GUI/CLI

---

By following this architecture, IntuiCAM achieves a balance of **clarity**, **extensibility**, and **performance**, while remaining **inviting** for new contributors and future integrations into other platforms.
