cmake_minimum_required(VERSION 3.16)
project(IntuiCAM_Python LANGUAGES CXX)

# Find Python
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Found Python: ${Python3_EXECUTABLE} (${Python3_VERSION})")

# Find pybind11 for Python bindings (optional)
find_package(pybind11 QUIET)
if(pybind11_FOUND)
    message(STATUS "Found pybind11: ${pybind11_VERSION}")

    # Python wrapper module
    pybind11_add_module(intuicam_py
        src/module.cpp
        src/core_bindings.cpp
    )

    # If no source files exist yet, create minimal wrapper
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/module.cpp")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/module.cpp"
"#include <pybind11/pybind11.h>

namespace py = pybind11;

void init_core(py::module &);

PYBIND11_MODULE(intuicam_py, m) {
    m.doc() = \"IntuiCAM Python bindings\";
    
    // Initialize submodules
    init_core(m);
}
")
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core_bindings.cpp")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/core_bindings.cpp"
"#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Include necessary IntuiCAM Core headers
// #include \"Path/To/CoreClasses.h\"

namespace py = pybind11;

void init_core(py::module &m) {
    // Create a submodule for core functionality
    auto core = m.def_submodule(\"core\", \"Core IntuiCAM functionality\");
    
    // Bind core classes and functions here
    // Example:
    // py::class_<MyClass>(core, \"MyClass\")
    //     .def(py::init<>())
    //     .def(\"my_method\", &MyClass::my_method)
    //     .def_property(\"my_property\", &MyClass::get_prop, &MyClass::set_prop);
}
")
    endif()

    # Link with core library
    target_link_libraries(intuicam_py PRIVATE 
        IntuiCAM::IntuiCAMCore
    )

    # Installation
    install(TARGETS intuicam_py 
        LIBRARY DESTINATION "${Python3_SITEARCH}/intuicam"
    )

    # Create package files
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py"
"from .intuicam_py import *
__version__ = '${PROJECT_VERSION}'
")
        install(FILES 
            "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py"
            DESTINATION "${Python3_SITEARCH}/intuicam"
        )
    endif()
else()
    message(STATUS "pybind11 not found, skipping Python module build")
endif() 