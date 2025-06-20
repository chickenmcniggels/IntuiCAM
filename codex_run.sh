#!/bin/bash
# Minimal build script for the Codex environment
set -e

# Compile a lightweight test binary without external dependencies

g++ tests/test_core.cpp -Icore/common/include -Icore/geometry/include -std=c++17 -o test_core

# Run the binary to verify that the basic headers compile
./test_core
