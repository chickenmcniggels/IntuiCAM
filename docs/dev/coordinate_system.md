# Coordinate System Overview

IntuiCAM uses a simple layered coordinate system for lathe operations.

1. **Global Viewer Coordinates** – the OpenCASCADE scene. The chuck face is positioned at `Z=0` and the spindle axis aligns with the positive `Z` direction. Raw material extends 50 mm into negative `Z` by default.
2. **Work Coordinates (G54)** – origin located at the end face of the raw material. The Z‑axis points along the spindle, X is radial. 2D profiles and toolpaths are created in this space.
3. **Machine Coordinates** – currently identical to work coordinates but reserved for future offsets.

When the user positions the workpiece relative to the chuck, the raw material manager recalculates the stock so that its end aligns with this distance. The `WorkspaceCoordinateManager` then initializes the work coordinate system at that raw material end. All profiles are extracted and toolpaths are generated in work coordinates and later transformed to the global viewer using this matrix.
