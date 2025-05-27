# 3D Viewer Component

## Overview

The `OpenGL3DWidget` is the primary 3D visualization component in IntuiCAM, built on OpenCASCADE Technology (OCCT) and Qt's QOpenGLWidget. It provides interactive 3D visualization of workpieces, raw materials, and tooling setups.

## Key Features

### Core Functionality
- **OpenCASCADE Integration**: Direct integration with OCCT's V3d_Viewer for professional CAD visualization
- **Interactive Navigation**: Mouse-based rotation, panning, and zooming
- **Multi-object Display**: Simultaneous visualization of workpieces, raw materials, and chuck components
- **Focus-Independent Rendering**: Reliable rendering regardless of widget focus state

### Recent Improvements (v1.1)

#### Black Screen Fix
**Problem**: The 3D viewer would go black when the widget lost focus or wasn't actively interacted with.

**Solution**: Implemented robust focus and event handling:
- Enhanced `updateView()` method with explicit context management
- Added continuous update timer support
- Proper focus event handling (`focusInEvent`, `focusOutEvent`)
- Show/hide event management to ensure proper rendering lifecycle

```cpp
void OpenGL3DWidget::updateView()
{
    if (!m_view.IsNull() && !m_window.IsNull())
    {
        // Ensure the OpenGL context is current before updating
        makeCurrent();
        
        // Force redraw even if widget doesn't have focus
        m_view->Redraw();
        
        // Make sure the rendering is complete
        if (context()) {
            context()->swapBuffers(context()->surface());
        }
    }
}
```

#### Enhanced Raw Material Positioning
**Problem**: Raw material cylinders didn't properly encompass the entire workpiece.

**Solution**: Implemented intelligent workpiece analysis:
- Bounding box calculation along rotation axis
- Proper cylinder positioning to encompass workpiece
- Automatic length calculation with machining allowances
- Support for custom and standard diameter selection

## Usage

### Basic Display Operations

```cpp
// Display a workpiece shape
openglWidget->displayShape(workpieceShape);

// Clear all objects
openglWidget->clearAll();

// Fit all objects in view
openglWidget->fitAll();
```

### Continuous Updates

```cpp
// Enable continuous updates (useful for animations)
openglWidget->setContinuousUpdate(true);

// Disable continuous updates (default)
openglWidget->setContinuousUpdate(false);
```

## Raw Material Management

### Automatic Sizing

The raw material manager now automatically calculates optimal dimensions:

```cpp
// Display raw material that encompasses the workpiece
rawMaterialManager->displayRawMaterialForWorkpiece(diameter, workpiece, axis);
```

### Manual Control

Users can now manually control raw material properties:

```cpp
// Set custom diameter
rawMaterialManager->setCustomDiameter(customDiameter, workpiece, axis);

// Get standard diameters for UI
const QVector<double>& diameters = rawMaterialManager->getStandardDiameters();
```

## Technical Details

### OpenCASCADE Integration

The widget uses OCCT's modern OpenGL driver approach:

```cpp
// Create display connection
Handle(Aspect_DisplayConnection) aDisplayConnection = new Aspect_DisplayConnection();

// Create graphic driver
Handle(OpenGl_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);

// Create viewer
m_viewer = new V3d_Viewer(aGraphicDriver);
```

### Event Handling

Mouse interactions are translated to OCCT view operations:

- **Left Mouse**: Rotation (`m_view->StartRotation()`, `m_view->Rotation()`)
- **Middle Mouse**: Panning (`m_view->Pan()`)
- **Right Mouse**: Zooming (`m_view->Zoom()`)
- **Wheel**: Zoom in/out

### Focus Management

The widget maintains rendering even when losing focus:

```cpp
void OpenGL3DWidget::focusOutEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusOutEvent(event);
    // Force an update even when losing focus to prevent black screen
    update();
}
```

## Known Issues and Solutions

### Issue: Black Screen on Focus Loss
**Status**: ✅ Fixed in v1.1
**Solution**: Enhanced focus event handling and continuous update support

### Issue: Raw Material Not Encompassing Part
**Status**: ✅ Fixed in v1.1  
**Solution**: Improved bounding box calculation and cylinder positioning

### Issue: Performance with Large Models
**Status**: Ongoing optimization
**Workaround**: Use `setContinuousUpdate(false)` for large models when not animating

## Future Enhancements

### Planned Features
- Manual axis selection by clicking on cylindrical features
- Real-time material property editing
- Advanced visualization modes (wireframe, shaded, transparent)
- Performance profiling and optimization tools
- Export capabilities (images, 3D formats)

### UI Integration
- Raw material diameter selection dialog
- Axis selection tools
- View preset buttons (front, top, isometric)
- Measurement tools

## API Reference

See the header file `gui/include/opengl3dwidget.h` for complete API documentation.

### Key Methods

- `displayShape(const TopoDS_Shape& shape)`: Display a 3D shape
- `clearAll()`: Remove all displayed objects
- `fitAll()`: Fit all objects in view
- `setContinuousUpdate(bool enabled)`: Control continuous rendering
- `isViewerInitialized()`: Check initialization status

### Signals

- `viewerInitialized()`: Emitted when OpenCASCADE viewer is ready 