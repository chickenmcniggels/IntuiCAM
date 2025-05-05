cmake_minimum_required(VERSION 3.16)
project(IntuiCAM_GUI LANGUAGES CXX)

# Find required Qt6 components for the GUI (e.g., Widgets and Quick if needed)
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

# You can list UI files and use qt6_wrap_ui, or rely on AUTOUIC (enabled by qt_standard_project_setup in top-level)
qt6_wrap_ui(GENERATED_UI_FILES ${UI_FILES})   # Generates headers from .ui files (if not using AUTOUIC)

# List any Qt resource files if present (e.g., .qrc files)
# set(RESOURCE_FILES resources.qrc)
# qt6_add_resources(QRC_FILES ${RESOURCE_FILES})

# Define the GUI executable target (Qt-specific CMake function)
qt_add_executable(IntuiCAM_GUI
        MANUAL_FINALIZATION                  # Manual finalization for Qt (if needed)
        WIN32                                # Use WIN32 for a Windows app to hide console (if applicable)
        ${UI_FILES}                          # .ui files (AUTOUIC will handle them) or use ${GENERATED_UI_FILES}
        src/main.cpp                        # Main source file for the GUI application
        # ... other GUI source and header files
)

# Link the Qt6 libraries and core library to the executable
target_link_libraries(IntuiCAM_GUI PRIVATE
        Qt6::Widgets Qt6::Gui               # Qt6 widgets (includes Qt6Core and Qt6Gui transitively)
        IntuiCAMCore                        # Link to our core library (brings in OCCT via transitive link)
)

# If Qt plugins or other Qt modules are used, consider qt_finalize_executable (for Qt 6.2+)
qt_finalize_executable(IntuiCAM_GUI)    # Ensures all Qt runtime dependencies are bundled on Windows/macOS

# (Optional) If using an alias for core:
# target_link_libraries(IntuiCAM_GUI PRIVATE Qt6::Widgets Qt6::Gui IntuiCAM::IntuiCAMCore)
