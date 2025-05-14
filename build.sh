#!/bin/bash

# Configure build variables
BUILD_DIR="build"
INSTALL_DIR="install"
BUILD_TYPE="Debug"
GENERATOR="Ninja"

# Environment paths - change these to match your system setup
QT_DIR="/usr/lib/qt6"
OPENCASCADE_DIR="/usr/local/lib/opencascade"
OPENCASCADE_THIRD_PARTY_DIR="/usr/local/lib/opencascade"

echo "================================================================"
echo "Building IntuiCAM with the following configuration:"
echo "Build Directory: $BUILD_DIR"
echo "Install Directory: $INSTALL_DIR"
echo "Build Type: $BUILD_TYPE"
echo "Generator: $GENERATOR"
echo "Qt Directory: $QT_DIR"
echo "OpenCASCADE Directory: $OPENCASCADE_DIR"
echo "OpenCASCADE Third Party Directory: $OPENCASCADE_THIRD_PARTY_DIR"
echo "================================================================"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Create installation directory if it doesn't exist
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Creating installation directory..."
    mkdir -p "$INSTALL_DIR"
fi

# Configure with CMake
echo "Configuring project with CMake..."
cmake -S . -B "$BUILD_DIR" \
    -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$(pwd)/$INSTALL_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_DIR/lib/cmake;$OPENCASCADE_DIR/cmake" \
    -DOpenCASCADE_ROOT_DIR="$OPENCASCADE_DIR" \
    -DOpenCASCADE_THIRD_PARTY_DIR="$OPENCASCADE_THIRD_PARTY_DIR"

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building project..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Install the project
echo "Installing project..."
cmake --install "$BUILD_DIR" --config "$BUILD_TYPE"

if [ $? -ne 0 ]; then
    echo "Installation failed!"
    exit 1
fi

echo "================================================================"
echo "Build completed successfully!"
echo "Installation directory: $(pwd)/$INSTALL_DIR"
echo "================================================================" 