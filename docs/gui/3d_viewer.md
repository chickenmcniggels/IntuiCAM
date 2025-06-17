# 3D Viewer Component

## Overview

The `OpenGL3DWidget` is the primary 3D visualization component in IntuiCAM, built on OpenCASCADE Technology (OCCT) and Qt's QOpenGLWidget. It provides interactive 3D visualization of workpieces, raw materials, and tooling setups.

## Key Features

### Core Functionality
- **OpenCASCADE Integration**: Direct integration with OCCT's V3d_Viewer for professional CAD visualization
- **Interactive Navigation**: Mouse-based rotation, panning, and zooming
- **Multi-object Display**: Simultaneous visualization of workpieces, raw materials, and chuck components
- **Focus-Independent Rendering**: Reliable rendering regardless of widget focus state
- **Advanced Selection System**: Multi-mode selection with highlighting and feedback

### Enhanced Selection Capabilities (v1.2)

**Problem Resolved**: Previous selection implementation had limited functionality and poor feedback.

**Solution**: Implemented comprehensive OCCT-native selection system:
- **Multi-mode Selection**: Supports whole objects (mode 0), edges (mode 2), and faces (mode 4)
- **Automatic Highlighting**: Visual feedback during mouse hover and selection
- **Sub-shape Selection**: Ability to select specific faces, edges, or vertices
- **Proper Event Handling**: Robust mouse interaction with accurate 3D point calculation
- **Selection Feedback**: Clear status messages and debugging information

```cpp
// Enhanced selection with multiple modes
void OpenGL3DWidget::setSelectionMode(bool enabled)
{
    if (enabled) {
        m_context->SetAutomaticHilight(Standard_True);
        m_context->SetAutoActivateSelection(Standard_True);
        
        // Activate multiple selection modes for all displayed shapes
        AIS_ListOfInteractive allObjects;
        m_context->DisplayedObjects(allObjects);
        
        for (AIS_ListOfInteractive::Iterator anIter(allObjects); anIter.More(); anIter.Next()) {
            Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(anIter.Value());
            if (!aShape.IsNull()) {
                m_context->Activate(aShape, 0, Standard_False); // Whole shape
                m_context->Activate(aShape, 4, Standard_False); // Faces
                m_context->Activate(aShape, 2, Standard_False); // Edges
            }
        }
    }
}
```

### Improved Raw Material Positioning (v1.2)

**Problem Resolved**: Raw material cylinders were not properly positioned relative to the chuck, and the rightmost part didn't always have the largest Z value.

**Solution**: Enhanced positioning algorithm with proper lathe-oriented calculations:
- **Axis-aligned Calculation**: Projects all workpiece bounding box corners onto the rotation axis
- **Proper Extent Detection**: Finds true min/max extents along the machining axis
- **Chuck-relative Positioning**: Positions raw material from chuck end (minimum Z) to rightmost end (maximum Z)
- **Intelligent Allowances**: Adds proper machining allowances (10% minimum 5mm per end)

```cpp
// Enhanced positioning ensures rightmost part has largest Z value
TopoDS_Shape RawMaterialManager::createCylinderForWorkpiece(double diameter, double length, 
                                                           const gp_Ax1& axis, const TopoDS_Shape& workpiece)
{
    // Project all 8 bounding box corners onto the axis
    for (int i = 0; i < 8; i++) {
        gp_Vec toCorner(axisLoc, corners[i]);
        double projection = toCorner.Dot(axisDir);
        minProjection = std::min(minProjection, projection);
        maxProjection = std::max(maxProjection, projection);
    }
    
    // Add machining allowance and position from chuck (minProjection) to rightmost (maxProjection)
    double allowance = std::max((maxProjection - minProjection) * 0.1, 5.0);
    cylinderStartPoint = axisLoc.Translated(gp_Vec(axisDir) * (minProjection - allowance));
    length = (maxProjection - minProjection) + 2 * allowance;
}
```

### Recent Improvements (v1.1)

#### Black Screen Fix
**Problem**: The 3D viewer would go black when the widget lost focus or wasn't actively interacted with.

**Solution**: Implemented robust focus and event handling:
- Enhanced `updateView()` method with explicit context management
- Added continuous update timer support
- Proper focus event handling (`focusInEvent`, `focusOutEvent`) with immediate redraw
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

## Selection System Architecture

### Native OCCT vs OpenGL Approaches

**Current Implementation**: Uses native OCCT AIS_InteractiveContext - **This is the optimal approach**.

**Why OCCT Native Selection is Superior**:
1. **Built-in Selection Modes**: Supports object, face, edge, vertex selection out of the box
2. **Automatic Highlighting**: Provides visual feedback during detection
3. **Precise Picking**: Accurate 3D point calculation and geometry intersection
4. **Filters and Constraints**: Advanced selection filtering capabilities
5. **Professional CAD Standard**: Industry-standard approach used by major CAD systems

**Alternative Considered**: Custom OpenGL picking - Not recommended because:
- Requires manual implementation of complex 3D picking algorithms
- No built-in highlighting or visual feedback
- Limited to basic object-level selection
- Significantly more development effort for inferior results

### Selection Workflow

1. **Enable Selection Mode**: `setSelectionMode(true)` activates multi-mode selection
2. **Visual Feedback**: Objects highlight on mouse hover due to automatic highlighting
3. **Click Detection**: `MoveTo()` detects entities at mouse position
4. **Selection Processing**: `SelectDetected()` confirms selection
5. **Shape Extraction**: Extracts complete shapes or sub-shapes (faces/edges)
6. **3D Point Calculation**: Calculates accurate 3D coordinates of selection point
7. **Signal Emission**: Emits `shapeSelected()` with shape and 3D point

## Raw Material Management

### Automatic Sizing

The raw material manager now automatically calculates optimal dimensions with proper lathe-oriented positioning:

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

### Positioning Algorithm

The enhanced algorithm ensures proper lathe-oriented positioning:

1. **Bounding Box Analysis**: Calculates 3D bounding box of workpiece
2. **Axis Projection**: Projects all 8 corners onto the rotation axis
3. **Extent Calculation**: Finds minimum and maximum projections
4. **Chuck Alignment**: Positions cylinder from chuck end (minimum) to tailstock end (maximum)
5. **Allowance Addition**: Adds machining allowances for proper material coverage

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
**Solution**: Enhanced focus event handling with immediate redraws and continuous update support

### Issue: Raw Material Not Encompassing Part
**Status**: ✅ Fixed in v1.2
**Solution**: Improved bounding box calculation and cylinder positioning

### Issue: Selection Not Working Properly
**Status**: ✅ Fixed in v1.2
**Solution**: Enhanced multi-mode selection with proper OCCT integration

### Issue: Performance with Large Models
**Status**: Ongoing optimization
**Workaround**: Use `setContinuousUpdate(false)` for large models when not animating

## Future Enhancements

### Planned Features
- Advanced selection filters for specific geometry types
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
- `setSelectionMode(bool enabled)`: Enable/disable interactive selection
- `isViewerInitialized()`: Check initialization status

### Signals

- `viewerInitialized()`: Emitted when OpenCASCADE viewer is ready
- `shapeSelected(const TopoDS_Shape& shape, const gp_Pnt& point)`: Emitted when shape is selected in selection mode 