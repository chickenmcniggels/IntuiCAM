cmake_minimum_required(VERSION 3.16)
project(IntuiCAM_FreeCAD LANGUAGES CXX)

# Option to specify FreeCAD Module path
set(FREECAD_MOD_DIR "" CACHE PATH "FreeCAD Mod directory where the IntuiCAM plugin should be installed")

# Attempt to find FreeCAD (optional)
find_package(FreeCAD QUIET)

if(FreeCAD_FOUND)
    message(STATUS "Found FreeCAD: ${FREECAD_INCLUDE_DIR}")
    
    # Create FreeCAD plugin library
    add_library(IntuiCAM_FreeCAD MODULE
        src/InitGui.cpp
        src/AppIntuiCAM.cpp
        src/IntuiCAMGui.cpp
    )
    
    # If files don't exist yet, create simple templates
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
    
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/InitGui.cpp")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/InitGui.cpp"
"#include <FCConfig.h>
#include <Gui/Application.h>
#include <Gui/Workbench.h>
#include <Gui/Command.h>

// Workbench class
class IntuiCAMWorkbench : public Gui::StdWorkbench
{
public:
    IntuiCAMWorkbench() {}
    ~IntuiCAMWorkbench() {}

    void setupContextMenu(const char* recipient, Gui::MenuItem* item) const override
    {
        // Add context menu items
    }

    Gui::ToolBarItem* setupToolBars() const override
    {
        // Set up toolbars
        return nullptr;
    }

    Gui::MenuItem* setupMenuBar() const override
    {
        // Set up menu bar entries
        return nullptr;
    }
};

// Workbench factory
extern \"C\" {
    void InitGui()
    {
        // Register the workbench
        Gui::Application::Instance->addWorkbench(new IntuiCAMWorkbench);
    }
} 
")
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/AppIntuiCAM.cpp")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/AppIntuiCAM.cpp"
"#include <FCConfig.h>
#include <Base/Console.h>
#include <App/Application.h>

// IntuiCAM core functionality will be linked in here
// Include core headers

// Module initialization
extern \"C\" {
    void AppIntuiCAMInit()
    {
        // Initialize the module
        Base::Console().Message(\"IntuiCAM module initialized\\n\");
    }
}
")
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/IntuiCAMGui.cpp")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/IntuiCAMGui.cpp"
"#include <FCConfig.h>
#include <Base/Console.h>
#include <Gui/Application.h>

// Import the IntuiCAM module
extern void AppIntuiCAMInit();

// GUI module initialization
extern \"C\" {
    void IntuiCAMGuiInit()
    {
        // Initialize GUI components
        Base::Console().Message(\"IntuiCAM GUI initialized\\n\");
        
        // Make sure App part is initialized
        AppIntuiCAMInit();
    }
}
")
    endif()
    
    # Configure include directories
    target_include_directories(IntuiCAM_FreeCAD
        PRIVATE
            ${FREECAD_INCLUDE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # Link with FreeCAD and IntuiCAMCore
    target_link_libraries(IntuiCAM_FreeCAD
        PRIVATE
            IntuiCAM::IntuiCAMCore
            ${FREECAD_LIBRARIES}
    )
    
    # Installation path
    if(FREECAD_MOD_DIR)
        install(TARGETS IntuiCAM_FreeCAD
            LIBRARY DESTINATION "${FREECAD_MOD_DIR}/IntuiCAM")
            
        # Create module initialization file
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Init.py"
"# FreeCAD init script for the IntuiCAM module

# Import FreeCAD module
import FreeCAD

# Add the module's path to the FreeCAD module search path
FreeCAD.addImportType(\"IntuiCAM part (*.stp)\", \"importIntuiCAM\")
")
        
        install(FILES 
            "${CMAKE_CURRENT_BINARY_DIR}/Init.py"
            DESTINATION "${FREECAD_MOD_DIR}/IntuiCAM")
    else()
        message(STATUS "FREECAD_MOD_DIR not set, will not install FreeCAD module")
    endif()
else()
    message(STATUS "FreeCAD not found, skipping FreeCAD plugin build")
endif() 