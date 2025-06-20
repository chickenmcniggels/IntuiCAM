#!/bin/bash
# Minimal build script for the Codex environment
set -e

if [ "$FULL_BUILD" = "1" ]; then
  echo "[codex] Performing full CMake build"
  apt-get update
  apt-get install -y qt6-base-dev qtbase5-dev libvtk9-dev \
    libocct-data-exchange-dev libocct-modeling-data-dev \
    libocct-modeling-algorithms-dev libocct-ocaf-dev \
    libocct-visualization-dev libocct-draw-dev libopenmpi-dev

  cmake -B build -G Ninja -DINTUICAM_BUILD_GUI=ON -DINTUICAM_BUILD_TESTS=OFF
  cmake --build build -j$(nproc)
else
  echo "[codex] Building lightweight test binary"
  g++ tests/test_core.cpp -Icore/common/include -Icore/geometry/include -std=c++17 -o test_core
  ./test_core
fi
