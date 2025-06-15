# Lathe Coordinate System Implementation

## Overview

This document describes the implementation of the lathe coordinate system in IntuiCAM's 3D viewer.

## Implemented Features

The following coordinate system improvements have been implemented:

1. **True Orthographic Projection**
   - Properly implemented orthographic projection in the XZ view using `Graphic3d_Camera::Projection_Orthographic`
   - Ensures accurate dimensional representation without perspective distortion

2. **Standard Lathe Coordinate System**
   - X-axis runs vertically (positive from top to bottom)
   - Z-axis runs horizontally (positive from right to left)
   - View from negative Y-axis toward origin

3. **Grid Visualization**
   - Created a grid display for the XZ view to provide visual reference
   - Grid lines at specified intervals (default 10mm)
   - Highlighted X and Z axes with distinct colors (X: green, Z: red)

4. **Camera State Preservation**
   - 3D camera state is preserved when switching to XZ view
   - Projection type (perspective or orthographic) is restored when returning to 3D view
   - XZ view settings are consistent with standard lathe conventions
   - Smooth transition between views with proper handling of different projection types

5. **Proper Cleanup and Redisplay**
   - Added support for removing and redisplaying objects when switching views
   - Implemented helper methods in all managers to support redisplay operations
   - Efficient handling of object transforms and materials

## Implementation Details

The implementation involved changes to several key components:

1. **OpenGL3DWidget**
   - Added methods for creating and displaying a grid in the XZ view
   - Updated camera setup methods to use proper orthographic projection
   - Implemented state preservation between view modes

2. **WorkspaceController**
   - Added `redisplayAll()` method to coordinate redisplay of all workspace objects
   - Improved error handling for coordinate system transitions

3. **Supporting Managers**
   - Added `redisplayChuck()` to ChuckManager
   - Added `redisplayWorkpiece()` to WorkpieceManager
   - Added `redisplayRawMaterial()` to RawMaterialManager

## Usage

To switch between coordinate systems:

1. Use the view mode toggle button in the UI, or
2. Call `OpenGL3DWidget::setViewMode(ViewMode mode)` with either:
   - `ViewMode::Mode3D` for standard 3D view
   - `ViewMode::LatheXZ` for lathe 2D view

The view will automatically adjust the projection, grid display, and camera orientation to match the selected mode.

## Benefits

This implementation provides several benefits for lathe operations:

1. **Improved accuracy** in measuring and visualizing lathe operations
2. **Familiar coordinate system** for CNC lathe programmers
3. **Visual grid references** for easier part dimensioning
4. **Better depth perception** with true orthographic projection
5. **Consistent axis orientation** aligned with G-code conventions

## Future Improvements

Potential future enhancements to consider:

1. Additional grid customization options (spacing, color, etc.)
2. Measurement tools specific to the lathe view
3. Dimension display in the XZ view
4. Enhanced visualization of cutting depth in lathe operations 