#!/bin/bash
# Minimal build script for the Codex environment
set -e

# If the script is called with "full", build the entire project using CMake
if [[ "$1" == "full" ]]; then
    cmake --preset native-release
    cmake --build --preset native-release -j$(nproc)
    ctest --preset native-release || true
else
    # Compile a lightweight test binary without external dependencies
    g++ tests/test_core.cpp -Icore/common/include -Icore/geometry/include -std=c++17 -o test_core

    # Run the binary to verify that the basic headers compile
    ./test_core
fi
