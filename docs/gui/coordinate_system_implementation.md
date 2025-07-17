#Lathe Coordinate System Implementation

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

3. **Lathe-Oriented Default 3D View**
   - The initial 3D camera now positions the Z axis horizontally, matching the lathe orientation

4. **Grid Visualization (Removed)**
   - The previous grid overlay for the XZ view has been removed to simplify the interface

5. **Camera State Preservation**
   - 3D camera state is preserved when switching to XZ view
   - Orthographic projection is used consistently in both views
   - XZ view settings are consistent with standard lathe conventions
   - Smooth transition between views with proper handling of different projection types

6. **Proper Cleanup and Redisplay**
   - Added support for removing and redisplaying objects when switching views
   - Implemented helper methods in all managers to support redisplay operations
   - Efficient handling of object transforms and materials

## Implementation Details

The implementation involved changes to several key components:

1. **OpenGL3DWidget**
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

The view will automatically adjust the projection and camera orientation to match the selected mode.

### Work Origin Synchronization

The work coordinate system places its origin at the end face of the raw material.
All generated toolpaths now derive their `Z0` datum from this point to ensure
that operations remain aligned with the displayed stock regardless of the
part's distance from the chuck.

## Benefits

This implementation provides several benefits for lathe operations:

1. **Improved accuracy** in measuring and visualizing lathe operations
2. **Familiar coordinate system** for CNC lathe programmers
3. **Removed grid overlay** for a cleaner view
4. **Better depth perception** with true orthographic projection
5. **Consistent axis orientation** aligned with G-code conventions

## Future Improvements

Potential future enhancements to consider:

1. Measurement tools specific to the lathe view
2. Dimension display in the XZ view
3. Enhanced visualization of cutting depth in lathe operations
