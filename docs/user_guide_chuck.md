# User Guide: Chuck Management and Workpiece Alignment

## Overview

IntuiCAM provides intelligent 3-jaw chuck management that automatically aligns workpieces and determines appropriate raw material sizes for CNC turning operations. This guide explains how to use these features effectively.

## Getting Started

### Automatic Chuck Loading

When you start IntuiCAM, the 3-jaw chuck is automatically loaded and displayed in the 3D viewport. The chuck file (`three_jaw_chuck.step`) should be located at:
```
C:\Users\nikla\Downloads\three_jaw_chuck.step
```

**Visual Indicators:**
- The chuck appears with a realistic metallic gray appearance
- It remains permanently visible during all operations
- The chuck is positioned at the workspace origin

### Loading Workpieces

1. **Open STEP File**: Use `File → Open STEP File` or `Ctrl+Shift+O`
2. **Browse**: Select your workpiece STEP file
3. **Automatic Processing**: The system will:
   - Detect cylindrical features in your workpiece
   - Determine the optimal raw material diameter
   - Create and display a transparent raw material cylinder
   - Align everything with the chuck axis

## Understanding the Display

### Visual Elements

**Chuck (Permanent)**
- Metallic gray appearance
- Always visible
- Represents the 3-jaw chuck fixture

**Workpiece**
- Aluminum-like appearance (light blue-gray)
- Your imported part geometry
- Shows the final machined result

**Raw Material**
- Transparent brass-colored cylinder
- Represents the stock material before machining
- Automatically sized to the next standard diameter

### Material Colors
- **Chuck**: Steel gray (permanent fixture)
- **Workpiece**: Aluminum blue-gray (finished part)
- **Raw Material**: Transparent brass (stock material)

## Automatic Features

### Cylinder Detection

The system automatically analyzes your workpiece for cylindrical features:

**Detection Criteria:**
- Diameter between 5mm and 500mm
- Recognizable cylindrical surfaces
- Suitable for turning operations

**Feedback:**
- Console output shows detected cylinder parameters
- Automatic alignment occurs when cylinders are found
- Multiple cylinders are detected (largest used for alignment)

### Standard Diameter Matching

Raw material is automatically sized using standard stock diameters:

**Standard Sizes (mm):**
```
6, 8, 10, 12, 16, 20, 25, 30, 32, 40, 50, 60, 63, 80, 
100, 110, 125, 140, 160, 180, 200, 220, 250, 280, 315, 
355, 400, 450, 500
```

**Selection Logic:**
- System finds the smallest standard diameter larger than your workpiece
- For oversized parts (>500mm), rounds up to next 50mm increment
- Automatically adds 20% length margin for machining operations

## User Controls

### File Operations

**Clear Workpieces:**
- The chuck remains visible while workpieces and raw material are cleared
- Use when loading a new project or starting fresh

**Multiple Workpieces:**
- Each new STEP file replaces the previous workpiece
- Raw material is recalculated for each new part

### View Controls

**3D Navigation:**
- **Left Mouse**: Rotate view
- **Middle Mouse**: Pan view
- **Right Mouse**: Zoom
- **Mouse Wheel**: Zoom in/out

**Automatic Fitting:**
- View automatically fits all objects after loading
- Manual fit available through view controls

## Troubleshooting

### Common Issues

**Chuck Not Loading:**
- Verify the chuck STEP file exists at the specified path
- Check file permissions and accessibility
- Look for error messages in the output log

**No Cylinder Detection:**
- Ensure your workpiece contains cylindrical features
- Check that cylinder diameters are within 5-500mm range
- Verify STEP file contains valid geometry

**Raw Material Not Visible:**
- Check transparency settings (default 70%)
- Ensure workpiece alignment completed successfully
- Look for error messages in the output window

### Debug Information

**Output Window:**
The output log shows detailed information about:
- Chuck loading status
- Cylinder detection results
- Raw material calculations
- Error messages and warnings

**Console Output:**
Additional technical details available in debug console:
- Detected cylinder dimensions
- Standard diameter selection
- Geometric transformation details

## Advanced Usage

### Custom Transparency

While not exposed in the UI yet, transparency can be programmatically adjusted:
- Default: 70% transparency (0.7)
- Range: 0% (opaque) to 100% (invisible)
- Future versions will include UI controls

### Integration with CAM Operations

The chuck management system provides the foundation for:
- Tool path generation
- Collision detection
- Workholding verification
- Setup validation

## Tips and Best Practices

### Workpiece Design
- Include clear cylindrical features for automatic detection
- Use standard diameters when possible to minimize material waste
- Consider chuck capacity when designing parts

### File Management
- Keep chuck STEP file in the designated location
- Use descriptive names for workpiece STEP files
- Organize projects by part families or material types

### Workflow Optimization
1. Load workpiece and verify automatic alignment
2. Check raw material diameter and length
3. Verify chuck clearances
4. Proceed with tool and operation setup

## Future Enhancements

Planned improvements include:
- Manual cylinder selection interface
- Custom raw material dimensions
- Multiple workpiece support
- Advanced fixture positioning
- Tool collision detection
- Setup optimization suggestions

## Getting Help

**Error Messages:**
- Check the output window for detailed error information
- Common issues are usually related to file paths or geometry

**Technical Support:**
- Review the debug output for technical details
- Check the architecture documentation for implementation details
- Consult the development team for complex issues

This user guide covers the essential aspects of using IntuiCAM's chuck management system for efficient CNC turning setup and visualization. 