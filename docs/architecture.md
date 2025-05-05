# Architecture Overview

This document provides a high-level overview of IntuiCAM’s architecture, detailing its modular structure, core components, and design principles. It is intended as a reference for developers contributing to or extending the project.

---

## 1. Repository Layout

```
IntuiCAM/
├── core/            # C++ CAM engine (library)
├── gui/             # Qt-based desktop application
├── docs/            # Project documentation (Markdown files)
│   └── architecture.md  # This file
├── tests/           # Unit and integration tests
├── cmake/           # CMake helper modules and toolchain files
├── .github/         # GitHub Actions workflows
├── examples/        # Sample projects and usage examples
└── scripts/         # Utility scripts (e.g. code generation, deployment)
```

* **core/**: Contains the performance‑critical CAM core, responsible for model import, toolpath generation, and G‑code export. Built as a standalone library (`IntuiCAMCore`).
* **gui/**: Implements the user interface (3D viewport, parameter panels, workflow). Links against the core library.
* **docs/**: Markdown files guiding users and contributors (`architecture.md`, `installation.md`, etc.).
* **tests/**: Automated tests for both core algorithms and GUI integration.
* **.github/**: CI configurations (build, test, lint) using GitHub Actions.

---

## 2. Core Engine (`core/`)

* **Language**: Modern C++ (C++17/20)
* **Build**: CMake target `IntuiCAMCore`
* **Responsibilities**:

  * STEP import via OpenCASCADE
  * Definition of data models: `Part`, `Stock`, `Tool`, `Operation`, `Toolpath`
  * Toolpath algorithms for facing, roughing, finishing, parting
  * G‑code generation and generic post‑processing
  * Python bindings (via pybind11) for scripting and headless use
* **Design**:

  * Clear API in `include/IntuiCAMCore/`
  * No GUI or third‑party UI dependencies
  * Unit‑tested: algorithms validated in `tests/core/`

---

## 3. GUI Application (`gui/`)

* **Framework**: Qt 5/6
* **Build**: CMake target `IntuiCAMApp`
* **Responsibilities**:

  * 3D scene rendering (OCCT or OpenGL) of parts, stock, and toolpaths
  * Interactive axis and fixture definition
  * Workflow panels for importing models, setting up operations, previewing toolpaths
  * Triggering core functions and displaying results
* **Design**:

  * Model–View–Controller pattern: GUI emits user events, controller invokes core API, view updates
  * Responsive UI: long‑running tasks run in worker threads
  * Plugin loader for GUI extensions (e.g. custom tool libraries)

---

## 4. Plugin & Scripting System

* **Python Integration**: `pybind11` wraps core classes for use in scripts.
* **Plugin Interface**: Core and GUI support loading dynamic modules (`.so`/`.dll`) implementing defined interfaces.
* **Use Cases**:

  * Custom operation strategies
  * Automated batch processing via scripts
  * FreeCAD Workbench integration in the future

---

## 5. Data Model & Persistence

* **Project Structure**:

  * `Project` holds references to imported models, stock definitions, operations, and tools.
  * Serialized as JSON (`.camproj`), ensuring human‑readability and easy merging.
* **Change Notifications**: Observer pattern informs GUI of model updates.

---

## 6. Cloud & AI Services (Planned)

* **AI Service Client**: Modular component using HTTP (e.g. libcurl or QtNetwork) to request feature detection or parameter suggestions.
* **Offline Safety**: Core features work fully offline; cloud calls optional.

---

## 7. Simulation & Verification (Future)

* **Simulation Module**: Plugin responsible for material removal and collision checks.
* **APIs**: Core exposes interfaces for simulator to consume toolpath data and return analysis results.

---

## 8. CLI & Batch Mode

* **Command‑Line Tool**: `intui_cam` executable for headless operations (import, compute paths, export G‑code).
* **Use Cases**: CI integration, automated CAM pipelines, remote servers.

---

## 9. Build & CI

* **CMake**: Configures all targets, manages dependencies, enables multi‑platform builds.
* **GitHub Actions**: Automated build/test matrix for Windows, Linux, macOS. Steps include:

  1. Checkout & setup dependencies
  2. Configure & build (Release & Debug)
  3. Run unit tests
  4. Lint & format checks (clang-format, clang-tidy)

---

## 10. Design Principles

* **Modularity**: Core and GUI decoupled; each component has a single responsibility.
* **SOLID**: Interfaces designed for extension without modification.
* **Clean Code**: Readable, well‑documented, self‑explaining functions.
* **Performance**: Critical sections optimized; multithreading for long operations.

*This overview serves as a living reference. As IntuiCAM evolves, update this document to reflect new modules, APIs, and design patterns.*
