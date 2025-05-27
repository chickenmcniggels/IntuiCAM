# DeployQt.cmake
# This module provides functions to automatically deploy Qt applications

function(deploy_qt_application target_name)
    if(WIN32)
        # Find Qt installation
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
        set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
        
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(DEPLOY_OPTIONS --debug)
        else()
            set(DEPLOY_OPTIONS --release)
        endif()
        
        # Add a post-build step to deploy Qt
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${QT_WINDEPLOYQT_EXECUTABLE} ${DEPLOY_OPTIONS} $<TARGET_FILE:${target_name}>
            COMMENT "Deploying Qt libraries for ${target_name}")
            
        # Create a launcher script
        set(LAUNCHER_SCRIPT "${CMAKE_BINARY_DIR}/run_${target_name}.bat")
        file(GENERATE OUTPUT ${LAUNCHER_SCRIPT}
            CONTENT "@echo off\nset PATH=${QT_BINARY_DIR};%PATH%\n$<TARGET_FILE:${target_name}>\n")
    endif()
endfunction()

# Function to add Qt to PATH for development
function(setup_qt_environment)
    if(WIN32)
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BINARY_DIR ${QT_QMAKE_EXECUTABLE} PATH)
        
        message(STATUS "Qt binary directory: ${QT_BINARY_DIR}")
        message(STATUS "Add this to your PATH for development: ${QT_BINARY_DIR}")
        
        # Create environment setup script
        set(ENV_SCRIPT "${CMAKE_SOURCE_DIR}/setup_environment.bat")
        file(WRITE ${ENV_SCRIPT}
            "@echo off\n"
            "echo Setting up IntuiCAM development environment...\n"
            "set PATH=${QT_BINARY_DIR};%PATH%\n"
            "echo Qt added to PATH: ${QT_BINARY_DIR}\n"
            "echo You can now run the application directly from build/Release/\n"
            "cmd /k\n")
    endif()
endfunction() 