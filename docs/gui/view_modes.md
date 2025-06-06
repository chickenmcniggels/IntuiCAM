# View Modes Documentation

## Overview

IntuiCAM's 3D viewer now supports two distinct viewing modes that can be seamlessly switched without loading a second viewer. This feature is particularly useful for CNC lathe operations where a traditional 2D XZ plane view is often preferred.

## Viewing Modes

### 3D Mode (Default)
- **Description**: Full 3D perspective view with complete rotation freedom
- **Mouse Controls**:
  - Left Click + Drag: Rotate view around the model
  - Middle Click + Drag: Pan the view
  - Right Click + Drag: Zoom in/out
  - Mouse Wheel: Zoom in/out
- **Features**:
  - Perspective projection
  - Coordinate trihedron display (bottom-left corner)
  - Full 3D navigation capabilities
  - Camera state preservation when switching away and back

### Lathe XZ Mode
- **Description**: Locked to XZ plane view following standard lathe coordinate system
- **Coordinate System**:
  - X-axis: Increases from top to bottom (vertical axis)
  - Z-axis: Increases from right to left (horizontal axis)
  - Y-axis: Depth (camera looks from negative Y toward origin)
- **Mouse Controls**:
  - Left Click + Drag: Pan the view (rotation disabled)
  - Middle Click + Drag: Pan the view
  - Right Click + Drag: Zoom in/out
  - Mouse Wheel: Zoom in/out
- **Features**:
  - Orthographic projection for technical drawing accuracy
  - No rotation allowed (maintains XZ plane orientation)
  - Coordinate trihedron hidden (not relevant for 2D view)
  - Optimized for lathe operation visualization

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

### Camera Management
- **State Preservation**: When switching away from 3D mode, the camera state (position, orientation, scale) is automatically stored and restored when returning to 3D mode
- **Smooth Transitions**: Camera changes are applied smoothly with proper view fitting
- **Mode-Specific Settings**: Each mode has optimized camera settings for its intended use

### OpenCASCADE Integration
- Uses OpenCASCADE's `V3d_View` projection settings
- Leverages `Graphic3d_Camera::Projection_Perspective` for 3D mode
- Uses `Graphic3d_Camera::Projection_Orthographic` for XZ mode
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

### Seamless Operation
- No interruption to workflow when switching modes
- All displayed objects remain visible and properly oriented
- Consistent selection and interaction behavior

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
- Orthographic projection in XZ mode may improve performance for complex scenes

### Development Guidelines
- Mode-specific behavior should be clearly documented
- Mouse interaction changes should be intuitive
- Camera state management should be robust and error-free

## Future Enhancements

### Potential Extensions
1. **Additional View Modes**: YZ plane, custom oriented planes
2. **Mode-Specific Tools**: Measurement tools optimized for each mode
3. **Grid Overlays**: Technical drawing grids for XZ mode
4. **Saved Views**: Named camera positions for quick access

### API Extensions
```cpp
// Potential future API
void setCustomViewPlane(const gp_Pln& plane);
void saveNamedView(const QString& name);
void restoreNamedView(const QString& name);
```

This documentation covers the comprehensive view mode system that provides both traditional 3D viewing and specialized lathe XZ plane visualization in a single, seamless interface. 