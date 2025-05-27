# Changelog: 3D Viewer and Raw Material Improvements

## Version 1.1 - December 2024

### 🎯 Major Issues Resolved

#### Issue #1: 3D Viewer Black Screen on Focus Loss
**Problem**: The 3D viewer would go black when the widget lost focus or wasn't actively interacted with, making the application appear broken.

**Root Cause**: Qt's QOpenGLWidget default behavior combined with insufficient focus event handling and context management.

**Solution Implemented**:
- Enhanced `updateView()` method with explicit OpenGL context management
- Added continuous update timer support for animation scenarios
- Implemented robust focus event handling (`focusInEvent`, `focusOutEvent`)
- Added show/hide event management for proper rendering lifecycle
- Improved OpenGL buffer swapping to ensure complete frame rendering

**Files Modified**:
- `gui/src/opengl3dwidget.cpp`: Enhanced rendering and event handling
- `gui/include/opengl3dwidget.h`: Added new member variables and method declarations

**Code Changes**:
```cpp
// Enhanced updateView with robust context management
void OpenGL3DWidget::updateView()
{
    if (!m_view.IsNull() && !m_window.IsNull())
    {
        makeCurrent();                    // Ensure context is current
        m_view->Redraw();                // Force redraw regardless of focus
        if (context()) {
            context()->swapBuffers(context()->surface());  // Complete rendering
        }
    }
}

// New continuous update support
void OpenGL3DWidget::setContinuousUpdate(bool enabled);

// Focus event handling to prevent black screen
void OpenGL3DWidget::focusOutEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusOutEvent(event);
    update();  // Force update even when losing focus
}
```

#### Issue #2: Raw Material Not Encompassing Workpiece
**Problem**: Raw material cylinders were created with default dimensions and positioning, often not properly encompassing the entire workpiece.

**Root Cause**: Insufficient geometric analysis and lack of workpiece-aware sizing algorithms.

**Solution Implemented**:
- Advanced bounding box calculation along rotation axis
- Intelligent cylinder positioning to encompass entire workpiece
- Automatic length calculation with machining allowances (20% extra)
- Support for both automatic and manual diameter selection

**Files Modified**:
- `gui/src/rawmaterialmanager.cpp`: Enhanced with workpiece analysis
- `gui/include/rawmaterialmanager.h`: Added new method declarations
- `gui/src/workspacecontroller.cpp`: Updated to use new sizing methods

**Code Changes**:
```cpp
// New workpiece-aware raw material creation
void RawMaterialManager::displayRawMaterialForWorkpiece(
    double diameter, 
    const TopoDS_Shape& workpiece, 
    const gp_Ax1& axis);

// Intelligent length calculation
double RawMaterialManager::calculateOptimalLength(
    const TopoDS_Shape& workpiece, 
    const gp_Ax1& axis);

// Proper cylinder positioning
TopoDS_Shape RawMaterialManager::createCylinderForWorkpiece(
    double diameter, 
    double length, 
    const gp_Ax1& axis, 
    const TopoDS_Shape& workpiece);
```

#### Issue #3: Limited Manual Control Options
**Problem**: Users had no way to manually adjust raw material dimensions or part alignment when automatic detection failed.

**Solution Implemented**:
- Added support for custom diameter selection
- Enhanced standard diameter database access
- Prepared foundation for manual axis selection (UI pending)

**Files Modified**:
- `gui/src/rawmaterialmanager.cpp`: Added custom diameter support
- `gui/include/rawmaterialmanager.h`: New public methods for manual control

**Code Changes**:
```cpp
// Manual diameter override
void RawMaterialManager::setCustomDiameter(
    double diameter, 
    const TopoDS_Shape& workpiece, 
    const gp_Ax1& axis);

// Access to standard diameter database
const QVector<double>& RawMaterialManager::getStandardDiameters() const;
```

### 🔧 Technical Improvements

#### Enhanced Geometric Analysis
- **Bounding Box Projection**: Accurate calculation of workpiece extent along rotation axis
- **Corner Analysis**: All 8 bounding box corners projected to find true extent
- **Machining Allowances**: Automatic 20% length addition with 10% on each end
- **Minimum Constraints**: Ensures minimum 10mm raw material length

#### Robust OpenGL Context Management
- **Explicit Context Binding**: `makeCurrent()` called before all rendering operations
- **Buffer Management**: Proper swap buffer handling for complete frame rendering
- **Timer-based Updates**: Optional continuous updates for animation scenarios
- **Focus Independence**: Reliable rendering regardless of widget focus state

#### Standard Material Database
- **ISO Metric Compliance**: Complete database of standard turning stock sizes (6-500mm)
- **Smart Selection**: Automatic selection of next larger standard size
- **Custom Override**: Support for non-standard diameter specification
- **Public Access**: UI can access standard sizes for dropdowns and validation

### 📝 Documentation Updates

#### New Documentation Files
- `docs/gui/3d_viewer.md`: Comprehensive 3D viewer documentation
- `docs/CHANGELOG_viewer_improvements.md`: This changelog document

#### Updated Documentation
- `docs/architecture.md`: Enhanced GUI Frontend section with v1.1 improvements
- Added technical details and architectural benefits

### 🧪 Testing Considerations

#### Manual Testing Scenarios
1. **Focus Test**: Click in/out of 3D viewer window - should remain visible
2. **Raw Material Test**: Load various workpiece geometries - raw material should encompass all
3. **Performance Test**: Large models with continuous updates enabled/disabled
4. **Interaction Test**: Mouse navigation should work smoothly without black screens

#### Future Automated Tests
- Unit tests for bounding box calculation
- Integration tests for raw material positioning
- Focus event simulation tests
- OpenGL context management tests

### 🚀 Future Enhancements

#### Planned UI Features
- Raw material diameter selection dialog
- Manual axis selection by clicking cylindrical features
- Real-time dimension editing
- View preset buttons (front, top, isometric)

#### Performance Optimizations
- Level-of-detail (LOD) for large models
- Frustum culling implementation
- Optimized material transparency rendering
- GPU-accelerated bounding box calculations

### 🐛 Known Issues

#### Resolved in v1.1
- ✅ 3D viewer goes black when not focused
- ✅ Raw material doesn't encompass full part
- ✅ No manual diameter control

#### Remaining Issues
- ⏳ Chuck centerline alignment (in progress)
- ⏳ Manual axis selection UI (planned)
- ⏳ Performance with very large models (ongoing optimization)

### 💻 Developer Notes

#### Architecture Benefits
- **Single Responsibility**: Each component maintains clear purpose
- **Open/Closed Principle**: Easy extension without modification
- **Enhanced Reliability**: Focus-independent rendering prevents UI glitches
- **Improved UX**: Automatic intelligent sizing reduces user setup time

#### Code Quality
- Comprehensive error handling with meaningful messages
- Debug logging for troubleshooting and development
- RAII principles for OpenGL resource management
- Exception safety in geometric calculations

#### Backwards Compatibility
- All existing APIs maintained
- New features are additive, not breaking
- Graceful fallbacks for edge cases
- Existing workspaces continue to work 