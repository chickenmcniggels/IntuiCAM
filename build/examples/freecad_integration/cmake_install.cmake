# Install script for directory: C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/examples/freecad_integration

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/IntuiCAM")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/freecad_integration" TYPE FILE OPTIONAL FILES
    "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/examples/freecad_integration/freecad_workbench.py"
    "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/examples/freecad_integration/path_generation.py"
    "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/examples/freecad_integration/model_import.py"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/freecad_integration/macros" TYPE FILE OPTIONAL FILES "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/examples/freecad_integration/IntuiCAM_Macro.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/examples/freecad_integration" TYPE FILE OPTIONAL FILES "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/examples/freecad_integration/README.md")
endif()

