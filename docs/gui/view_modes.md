# View Modes Documentation

## Overview

IntuiCAM's 3D viewer supports two distinct viewing modes that can be seamlessly switched without loading a second viewer. This feature is particularly useful for CNC lathe operations where a traditional 2D XZ plane view is often preferred.

## Viewing Modes

### 3D Mode (Default)
- **Description**: Full 3D view with complete rotation freedom
- **Mouse Controls**:
  - Left Click + Drag: Rotate view around the model
  - Middle Click + Drag: Pan the view
  - Right Click + Drag: Zoom in/out
  - Mouse Wheel: Zoom in/out
- **Features**:
  - Orthographic projection for consistent 3D visualization
  - Coordinate trihedron display (bottom-left corner)
  - Full 3D navigation capabilities
  - Camera state preservation when switching views

### XZ Plane (Lathe) Mode
- **Description**: 2D orthographic view representing the standard lathe coordinate system
- **Mouse Controls**:
  - Left Click + Drag: Pan the view
  - Right Click + Drag: Zoom in/out
  - Mouse Wheel: Zoom in/out
- **Features**:
  - True orthographic projection for accurate dimensional representation
  - Proper lathe coordinate system (X+ down, Z+ left)
  - Highlighted X and Z axes (red for Z, green for X)
  - Enhanced trihedron oriented to match lathe conventions

## Lathe Coordinate System

The XZ plane view implements the standard lathe coordinate system:
- Z-axis runs horizontally, with positive values increasing from right to left
- X-axis runs vertically, with positive values increasing from top to bottom
- The view is observed from the negative Y direction (looking toward the origin)

This convention matches ISO standard G-code programming for lathes, where:
- Z coordinates typically represent the distance from the chuck/spindle
- X coordinates represent the diameter distance from the centerline
- Tool movements follow the same coordinate system for intuitive programming

## Switching Between Modes

### UI Controls
1. **Menu**: View → Toggle Lathe View (or Switch to 3D View)
2. **Toolbar**: Click the view mode toggle button
3. **Keyboard Shortcut**: Press F2

### Programmatic Control
```cpp
// Switch to XZ lathe mode
viewer->setViewMode(ViewMode::LatheXZ);

// Switch to 3D mode
viewer->setViewMode(ViewMode::Mode3D);

// Toggle between modes
viewer->toggleViewMode();

// Get current mode
ViewMode currentMode = viewer->getViewMode();
```

## Implementation Details

### Projection Type
- 3D Mode: Uses Graphic3d_Camera::Projection_Orthographic for consistent visualization
- XZ Mode: Uses Graphic3d_Camera::Projection_Orthographic for accurate dimensional representation

### State Preservation
When switching between modes, the 3D camera state is preserved, allowing users to:
1. Switch to XZ view for precise measurement and toolpath programming
2. Return to the exact same 3D view without losing context

### Camera Management
 - **State Preservation**: When switching away from 3D mode, the camera state (position, orientation, scale, projection type) is automatically stored and restored when returning to 3D mode
- **Smooth Transitions**: Camera changes are applied smoothly with proper view fitting
- **Mode-Specific Settings**: Each mode has optimized camera settings for its intended use

### OpenCASCADE Integration
- Uses OpenCASCADE's `V3d_View` camera settings
- Explicitly sets `Graphic3d_Camera::Projection_Orthographic` for 3D mode
- Explicitly sets `Graphic3d_Camera::Projection_Orthographic` for XZ mode
- Proper camera positioning and orientation for each mode


### Event Handling
- Mouse interaction behavior changes based on current mode
- XZ mode disables rotation while maintaining pan and zoom
- Consistent zoom and pan behavior across both modes

## User Experience Features

### Visual Feedback
- Status bar messages indicate current mode
- Output log provides detailed mode information and usage hints
- Dynamic menu text updates based on current mode

### Usage Guidance
When switching to Lathe XZ mode, users receive helpful information:
- "View mode: Lathe XZ - X increases top to bottom, Z right to left"
- "Use left click to pan, wheel to zoom. Rotation disabled in this mode."
- "Grid with 10mm spacing shown for reference"

### Seamless Operation
- No interruption to workflow when switching modes
- All displayed objects remain visible and properly oriented
- Consistent selection and interaction behavior
- Proper projection mode automatically applied

## Technical Architecture

### Class Structure
```cpp
enum class ViewMode {
    Mode3D,     // Full 3D viewing with free rotation
    LatheXZ     // Locked to XZ plane for lathe operations
};

class OpenGL3DWidget {
    // Core mode management
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const;
    void toggleViewMode();
    
    // Camera management
    void setupCamera3D();
    void setupCameraXZ();
    void store3DCameraState();
    void restore3DCameraState();
    

signals:
    void viewModeChanged(ViewMode mode);
};
```

### Integration Points
- **MainWindow**: Provides UI controls and handles mode change events
- **OpenGL3DWidget**: Core viewing mode implementation
- **WorkspaceController**: Unaffected by mode changes (business logic separation)

## Best Practices

### When to Use Each Mode
- **3D Mode**: General modeling, assembly viewing, complex geometry analysis
- **Lathe XZ Mode**: Toolpath visualization, dimensional analysis, traditional lathe operation planning

### Performance Considerations
- Mode switching is lightweight (no geometry reloading)
- Camera state storage has minimal memory overhead
- True orthographic projection in XZ mode improves precision for technical drawings

### Development Guidelines
- Mode-specific behavior should be clearly documented
- Mouse interaction changes should be intuitive
- Camera state management should be robust and error-free
- Grid display should maintain proper OpenCASCADE resource management

## Future Enhancements

### Potential Extensions
1. **Additional View Modes**: YZ plane, custom oriented planes
2. **Mode-Specific Tools**: Measurement tools optimized for each mode
4. **Saved Views**: Named camera positions for quick access
5. **Position Markers**: Origin and machine coordinate system indicators

### API Extensions
```cpp
// Potential future API
void setCustomViewPlane(const gp_Pln& plane);
void saveNamedView(const QString& name);
void restoreNamedView(const QString& name);
```

This documentation covers the comprehensive view mode system that provides both traditional 3D viewing and specialized lathe XZ plane visualization in a single, seamless interface with precise orthographic projection and measurement tools.
