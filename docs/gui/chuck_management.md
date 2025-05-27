# Chuck Management and Workpiece Alignment

## Overview

IntuiCAM includes advanced 3-jaw chuck management functionality that automatically handles workpiece alignment and raw material sizing for CNC turning operations. The system provides intelligent cylinder detection, standard diameter matching, and transparent raw material visualization.

## Features

### Automatic Chuck Display
- The 3-jaw chuck STEP file is permanently displayed in the workspace
- Realistic metallic appearance with proper material properties
- Chuck remains visible at all times during workpiece operations

### Cylinder Detection
- Automatic detection of cylindrical features in imported workpieces
- Intelligent filtering to identify turning-relevant cylinders (5-500mm diameter)
- Real-time feedback on detected cylinder parameters

### Standard Material Matching
- Comprehensive list of ISO metric standard stock diameters
- Automatic selection of next larger standard diameter for raw material
- Support for common turning stock sizes from 6mm to 500mm
- Custom diameter calculation for oversized parts

### Raw Material Visualization
- Transparent raw material cylinder display
- Automatic positioning and alignment with chuck axis
- Configurable transparency levels for optimal visualization
- Real-time updates when workpieces change

## Usage

### Automatic Operation
1. **Chuck Initialization**: The chuck is automatically loaded on application startup
2. **Workpiece Import**: Load any STEP file containing cylindrical features
3. **Automatic Alignment**: The system detects cylinders and aligns the workpiece
4. **Raw Material Creation**: A transparent raw material cylinder is generated and displayed

### Manual Control
Users can also manually control various aspects:
- Clear workpieces while keeping the chuck
- Adjust raw material transparency
- Select specific cylinders for alignment (future enhancement)

## Standard Diameter Database

The system includes a comprehensive database of standard turning stock diameters:

```cpp
// Common turning stock diameters (mm)
6.0, 8.0, 10.0, 12.0, 16.0, 20.0, 25.0, 30.0, 
32.0, 40.0, 50.0, 60.0, 63.0, 80.0, 100.0, 110.0, 
125.0, 140.0, 160.0, 180.0, 200.0, 220.0, 250.0, 
280.0, 315.0, 355.0, 400.0, 450.0, 500.0
```

For parts requiring larger diameters, the system automatically rounds up to the next 50mm increment.

## Technical Architecture

### ChuckManager Class
The `ChuckManager` class handles all chuck-related functionality:

#### Key Methods
- `loadChuck()`: Loads and displays the 3-jaw chuck STEP file
- `addWorkpiece()`: Adds a workpiece and performs automatic alignment
- `detectCylinders()`: Analyzes workpiece geometry for cylindrical features
- `alignWorkpieceWithChuck()`: Aligns workpiece with chuck and creates raw material
- `displayRawMaterial()`: Creates and displays transparent raw material cylinder

#### Signals
- `cylinderDetected()`: Emitted when a cylinder is found in a workpiece
- `workpieceAligned()`: Emitted when alignment and raw material creation is complete
- `errorOccurred()`: Emitted for error conditions

### Integration with 3D Viewer
The chuck manager is integrated with the OpenGL 3D widget:
- Shared AIS context for consistent rendering
- Automatic view fitting after operations
- Coordinated material and lighting properties

## Configuration

### Chuck File Location
The chuck STEP file path is currently configured in the main window:
```cpp
QString chuckFilePath = "C:/Users/nikla/Downloads/three_jaw_chuck.step";
```

### Material Properties
The system uses realistic material properties:
- **Chuck**: Steel appearance with metallic gray coloring
- **Workpiece**: Aluminum-like appearance
- **Raw Material**: Brass-colored with configurable transparency

### Transparency Settings
Raw material transparency can be adjusted:
```cpp
// Default 70% transparency
chuckManager->setRawMaterialTransparency(0.7);
```

## Development Notes

### OpenCASCADE Integration
The system leverages OpenCASCADE for:
- STEP file loading and parsing
- Geometric analysis and cylinder detection
- 3D visualization and rendering
- Shape transformation and positioning

### Future Enhancements
Planned improvements include:
- Manual cylinder selection interface
- Custom raw material dimensions
- Multiple workpiece support
- Advanced fixture positioning
- Tool collision detection

## Error Handling

The system provides comprehensive error handling:
- File existence validation
- STEP file format verification
- Geometric analysis error recovery
- User feedback through Qt signals and debug output

## Performance Considerations

- Efficient cylinder detection using OpenCASCADE topology exploration
- Optimized rendering with proper material caching
- Selective redraw operations to maintain responsiveness
- Memory management through smart pointers and proper cleanup

## Examples

### Basic Workpiece Loading
```cpp
// Load workpiece (automatic alignment occurs)
TopoDS_Shape workpiece = stepLoader->loadStepFile("part.step");
chuckManager->addWorkpiece(workpiece);
```

### Manual Raw Material Creation
```cpp
// Create custom raw material
gp_Ax1 axis(gp_Pnt(0,0,0), gp_Dir(0,0,1));
chuckManager->displayRawMaterial(32.0, 150.0, axis);
```

### Chuck Status Checking
```cpp
if (chuckManager->isChuckLoaded()) {
    // Chuck is ready for operations
    TopoDS_Shape chuck = chuckManager->getChuckShape();
}
```

This documentation covers the comprehensive chuck management system that enables intelligent workpiece alignment and raw material visualization in IntuiCAM. 