#!/bin/bash

# setup.sh - Install dependencies for IntuiCAM project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f CMakeLists.txt ]; then
    echo "Error: CMakeLists.txt not found in $SCRIPT_DIR" >&2
    exit 1
fi

# Collect required packages based on CMakeLists.txt
REQ_PACKAGES="build-essential cmake ninja-build mpi-default-dev"

if grep -q "find_package(Qt6" CMakeLists.txt; then
    REQ_PACKAGES+=" qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools"
fi

if grep -q "find_package(VTK" CMakeLists.txt; then
    REQ_PACKAGES+=" libvtk9-dev libvtk9-qt-dev"
fi

if grep -q "find_package(OpenCASCADE" CMakeLists.txt; then
    REQ_PACKAGES+=" libocct-foundation-dev libocct-modeling-data-dev libocct-modeling-algorithms-dev libocct-data-exchange-dev libocct-ocaf-dev libocct-visualization-dev"
fi

# Update APT and install packages
sudo apt-get update
sudo apt-get install -y $REQ_PACKAGES

# Patch CMakeLists.txt to enable C language for MPI detection
if grep -q "project(IntuiCAM LANGUAGES CXX)" CMakeLists.txt; then
    sed -i 's/project(IntuiCAM LANGUAGES CXX)/project(IntuiCAM LANGUAGES C CXX)/' CMakeLists.txt
fi

# Remove Windows-specific hardcoded paths when building on non-Windows
if [[ "$(uname)" != "Windows_NT" ]]; then
    sed -i '/set(VTK_DIR/d' CMakeLists.txt
    sed -i '/set(OpenCASCADE_DIR/d' CMakeLists.txt
    sed -i '/link_directories(/d' CMakeLists.txt
fi

# Generate a toolchain file for CMake
cat > codex_toolchain.cmake <<'TCMAKE'
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake;/usr/lib/x86_64-linux-gnu;/usr/local;/usr" CACHE STRING "" FORCE)
TCMAKE

# Usage example:
# mkdir -p build && cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../codex_toolchain.cmake
# cmake --build .
