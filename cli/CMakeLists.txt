cmake_minimum_required(VERSION 3.16)
project(IntuiCAM_CLI LANGUAGES CXX)

# Find source files
file(GLOB_RECURSE CLI_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE CLI_HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# Create CLI executable
add_executable(IntuiCAM_CLI
    ${CLI_SOURCES}
    ${CLI_HEADERS}
)

# If no source files yet, create a minimal main.cpp
if(NOT CLI_SOURCES)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp 
"#include <iostream>
#include <string>
#include <vector>

// IntuiCAMCore includes will go here

int main(int argc, char** argv) {
    std::cout << \"IntuiCAM CLI\" << std::endl;
    // Process command line arguments and use core functionality
    
    return 0;
}
")
    set(CLI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
    target_sources(IntuiCAM_CLI PRIVATE ${CLI_SOURCES})
endif()

# Configure include directories
target_include_directories(IntuiCAM_CLI
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link with the core library
target_link_libraries(IntuiCAM_CLI
    PRIVATE
        IntuiCAM::IntuiCAMCore
)

# Installation
install(TARGETS IntuiCAM_CLI
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) 