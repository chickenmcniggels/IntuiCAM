# IntuiCAM FreeCAD Integration Examples

cmake_minimum_required(VERSION 3.20)

# FreeCAD integration examples
# These are primarily Python scripts for FreeCAD workbench

install(FILES 
    freecad_workbench.py
    path_generation.py
    model_import.py
    DESTINATION examples/freecad_integration
    OPTIONAL
)

# Install FreeCAD macro files
install(FILES 
    IntuiCAM_Macro.py
    DESTINATION examples/freecad_integration/macros
    OPTIONAL
)

# Install README
install(FILES 
    README.md
    DESTINATION examples/freecad_integration
    OPTIONAL
) 