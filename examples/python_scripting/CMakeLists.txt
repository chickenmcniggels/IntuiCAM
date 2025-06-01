# IntuiCAM Python Scripting Examples

cmake_minimum_required(VERSION 3.20)

# Python scripting examples don't need compilation
# Just install the Python scripts

install(FILES 
    basic_cam_workflow.py
    parametric_operations.py
    batch_processing.py
    DESTINATION examples/python_scripting
    OPTIONAL
)

# Install README
install(FILES 
    README.md
    DESTINATION examples/python_scripting
    OPTIONAL
) 