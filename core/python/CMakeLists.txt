# IntuiCAM/core/python/CMakeLists.txt

# Python bindings for IntuiCAM core modules using pybind11

find_package(pybind11 REQUIRED)

# Common module bindings
pybind11_add_module(intuicam_common_py
    common/common_bindings.cpp
)

target_link_libraries(intuicam_common_py PRIVATE
    intuicam_core_common
)

target_compile_definitions(intuicam_common_py PRIVATE
    VERSION_INFO=${EXAMPLE_VERSION_INFO}
)

# Geometry module bindings
pybind11_add_module(intuicam_geometry_py
    geometry/geometry_bindings.cpp
)

target_link_libraries(intuicam_geometry_py PRIVATE
    intuicam_core_geometry
    intuicam_core_common
)

# Toolpath module bindings
pybind11_add_module(intuicam_toolpath_py
    toolpath/toolpath_bindings.cpp
)

target_link_libraries(intuicam_toolpath_py PRIVATE
    intuicam_core_toolpath
    intuicam_core_geometry
    intuicam_core_common
)

# PostProcessor module bindings
pybind11_add_module(intuicam_postprocessor_py
    postprocessor/postprocessor_bindings.cpp
)

target_link_libraries(intuicam_postprocessor_py PRIVATE
    intuicam_core_postprocessor
    intuicam_core_toolpath
    intuicam_core_common
)

# Simulation module bindings
pybind11_add_module(intuicam_simulation_py
    simulation/simulation_bindings.cpp
)

target_link_libraries(intuicam_simulation_py PRIVATE
    intuicam_core_simulation
    intuicam_core_toolpath
    intuicam_core_geometry
    intuicam_core_common
)

# Combined module that imports all submodules
pybind11_add_module(intuicam_py
    intuicam_bindings.cpp
)

target_link_libraries(intuicam_py PRIVATE
    intuicam_core_common
    intuicam_core_geometry
    intuicam_core_toolpath
    intuicam_core_postprocessor
    intuicam_core_simulation
)

# Installation
install(TARGETS 
    intuicam_common_py
    intuicam_geometry_py
    intuicam_toolpath_py
    intuicam_postprocessor_py
    intuicam_simulation_py
    intuicam_py
    DESTINATION python/intuicam
) 