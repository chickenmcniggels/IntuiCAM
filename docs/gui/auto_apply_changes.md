# Auto-Apply Changes in the Setup Panel

## Overview

The setup panel features automatic application of changes to the 3D viewer. When you modify any setting, the result is immediately reflected without requiring manual application.

## Features

### Automatic Updates

- **Raw Material Diameter**: Changes instantly update the raw material cylinder size
- **Distance to Chuck**: Adjusts workpiece positioning relative to the chuck
- **Orientation Flip**: Immediately flips the workpiece orientation
- **Cylinder Selection**: Switching between detected axes updates the setup instantly

### Real-Time Feedback

- Status bar shows confirmation of successful updates
- Error messages appear if updates fail
- 3D viewer automatically fits the view to show changes
- Output log maintains a history of all changes

## Technical Implementation

### Workflow Controller Integration

The auto-apply functionality is implemented through the `WorkspaceController` class, which provides these methods:

```cpp
// Update raw material diameter while preserving workpiece and axis
bool updateRawMaterialDiameter(double diameter);

// Adjust workpiece positioning relative to chuck
bool updateDistanceToChuck(double distance);

// Flip workpiece orientation 180 degrees
bool flipWorkpieceOrientation(bool flipped);

// Apply all settings comprehensively
bool applyPartLoadingSettings(double distance, double diameter, bool flipped, int cylinderIndex);
```

### Signal Flow

1. **User changes setting** in the setup panel
2. **Panel emits signal** with new value
3. **MainWindow handler** receives signal
4. **WorkspaceController method** applies change
5. **3D viewer updates** automatically
6. **Status feedback** provided to user

### Original Workpiece Preservation

The system stores the original workpiece shape (`TopoDS_Shape m_currentWorkpiece`) to enable proper re-processing when settings change. This ensures:

- Accurate raw material sizing calculations
- Proper axis alignment preservation
- Consistent workpiece-to-chuck relationships

## Usage Examples

### Changing Raw Material Diameter

```cpp
// User adjusts diameter spinbox to 60.0mm
// Panel emits: rawMaterialDiameterChanged(60.0)
// MainWindow calls: workspaceController->updateRawMaterialDiameter(60.0)
// Result: Raw material cylinder updates to 60mm diameter instantly
```

### Flipping Workpiece Orientation

```cpp
// User checks "Flip Part Orientation" checkbox
// Panel emits: orientationFlipped(true)
// MainWindow calls: workspaceController->flipWorkpieceOrientation(true)
// Result: Workpiece orientation flips 180 degrees
```

### Comprehensive Settings Update

```cpp
// User clicks "Apply Changes" button
// All current settings are gathered and applied together
bool success = workspaceController->applyPartLoadingSettings(
    distance, diameter, flipped, cylinderIndex
);
// Result: All settings applied in coordinated manner
```

## Error Handling

### Graceful Degradation

- Invalid parameters are validated before application
- Missing components (no workpiece, no chuck) are handled gracefully
- Partial failures allow other settings to still be applied
- User receives clear feedback about any issues

### Recovery Scenarios

- If raw material update fails, original material remains visible
- Network/file errors don't crash the application
- Failed updates can be retried by adjusting settings again

## Performance Considerations

### Efficient Updates

- Only the affected components are regenerated (e.g., just raw material for diameter changes)
- Original workpiece geometry is preserved and reused
- 3D viewer updates are optimized to prevent flickering
- Redundant calculations are avoided through caching

### Memory Management

- OpenCASCADE handles cleanup automatically through smart pointers
- Original shapes are stored efficiently without duplication
- Viewer contexts are properly managed during updates

## Future Enhancements

### Planned Features

- **Live Preview**: Show changes while dragging sliders
- **Undo/Redo**: Allow reverting changes easily
- **Preset Management**: Save and load common configurations
- **Batch Updates**: Optimize multiple simultaneous changes

### Advanced Positioning

- **Precise Chuck Distance**: Implementation of actual workpiece translation
- **Orientation Transforms**: Full 3D rotation capability
- **Multi-Axis Positioning**: Support for complex workpiece orientations

## Developer Notes

### Adding New Auto-Apply Features

To add new auto-apply functionality:

1. **Add method to WorkspaceController**:
```cpp
bool updateNewSetting(ParameterType value);
```

2. **Connect signal in MainWindow**:
```cpp
connect(panel, &Panel::newSettingChanged, this, &MainWindow::handleNewSetting);
```

3. **Implement handler**:
```cpp
void MainWindow::handleNewSetting(ParameterType value) {
    bool success = m_workspaceController->updateNewSetting(value);
    // Handle success/failure feedback
}
```

### Testing Guidelines

- Test with various workpiece sizes and geometries
- Verify error handling with missing components
- Check performance with complex models
- Validate undo/redo functionality when implemented 