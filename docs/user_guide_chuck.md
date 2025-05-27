# User Guide: Chuck Management and Workpiece Alignment

## Overview

IntuiCAM provides intelligent 3-jaw chuck management that automatically aligns workpieces and determines appropriate raw material sizes for CNC turning operations. The system uses a modular architecture with specialized managers for different aspects of the workflow, providing reliable and efficient operation.

## System Components

The chuck management system consists of several integrated components working together:

- **Chuck Manager**: Handles the 3-jaw chuck display and positioning
- **Workpiece Manager**: Manages workpiece loading, display, and geometric analysis
- **Raw Material Manager**: Creates and displays raw material stock with standard sizing
- **STEP Loader**: Handles STEP file loading for all components

These components work seamlessly together to provide an integrated user experience.

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
   - Load and display the workpiece with aluminum-like appearance
   - Detect cylindrical features in your workpiece
   - Determine the optimal raw material diameter from standard sizes
   - Create and display a transparent raw material cylinder
   - Align everything properly for turning operations

## Understanding the Display

### Visual Elements

**Chuck (Permanent)**
- Metallic gray appearance with high reflectivity
- Always visible and positioned at the origin
- Represents the 3-jaw chuck fixture
- Managed by the Chuck Manager

**Workpiece**
- Aluminum-like appearance (light blue-gray)
- Your imported part geometry
- Shows the final machined result
- Managed by the Workpiece Manager

**Raw Material**
- Transparent brass-colored cylinder
- Represents the stock material before machining
- Automatically sized to the next standard diameter
- Managed by the Raw Material Manager

### Material Colors and Properties
- **Chuck**: Steel gray with metallic finish (permanent fixture)
- **Workpiece**: Aluminum blue-gray with standard metallic properties
- **Raw Material**: Transparent brass with 70% transparency (configurable)

## Automatic Features

### Intelligent Workpiece Analysis

The Workpiece Manager automatically analyzes your workpiece for cylindrical features:

**Detection Criteria:**
- Diameter between 5mm and 500mm
- Recognizable cylindrical surfaces from STEP geometry
- Suitable for turning operations

**Analysis Process:**
- Topology exploration of the STEP file geometry
- Identification of all cylindrical faces
- Selection of the largest/most suitable cylinder for alignment
- Extraction of axis orientation and diameter information

**Feedback:**
- Console output shows detected cylinder parameters
- Visual alignment occurs automatically when cylinders are found
- Multiple cylinders are detected (largest used for alignment)

### Smart Raw Material Sizing

The Raw Material Manager automatically sizes raw stock using standard diameters:

**Standard Sizes (mm) - ISO Metric:**
```
6, 8, 10, 12, 16, 20, 25, 30, 32, 40, 50, 60, 63, 80, 
100, 110, 125, 140, 160, 180, 200, 220, 250, 280, 315, 
355, 400, 450, 500
```

**Selection Logic:**
- System finds the smallest standard diameter larger than your workpiece
- For oversized parts (>500mm), rounds up to next 50mm increment
- Automatically adds 20% length margin for machining operations
- Considers material availability and cost optimization

## User Controls

### File Operations

**Clear Workpieces:**
```
File → Clear Workpieces (or load new STEP file)
```
- The chuck remains visible while workpieces and raw material are cleared
- Each new STEP file replaces the previous workpiece
- Raw material is recalculated for each new part

**Multiple Workpieces:**
- Currently supports one workpiece at a time
- Loading a new STEP file automatically clears the previous workpiece
- System ensures clean state for each new project

### View Controls

**3D Navigation:**
- **Left Mouse**: Rotate view around the scene
- **Middle Mouse**: Pan view horizontally and vertically
- **Right Mouse**: Zoom in and out
- **Mouse Wheel**: Zoom in/out incrementally

**Automatic Fitting:**
- View automatically fits all objects after loading
- Manual fit available through view controls
- System maintains optimal viewing angle for turning operations

### Component Status

**System Status Indicators:**
- Output window shows loading and processing status
- Debug console provides detailed technical information
- Visual feedback through object display and positioning

## Workflow Examples

### Basic Workpiece Processing
1. **Start Application**: Chuck loads automatically
2. **Load STEP File**: Select your workpiece file
3. **Automatic Analysis**: 
   - Workpiece appears in aluminum color
   - System detects cylindrical features
   - Raw material appears as transparent cylinder
4. **Review Setup**: 
   - Check alignment and dimensions
   - Verify raw material size is appropriate
   - Proceed with CAM operations

### Multiple Parts Workflow
1. **Load First Part**: Complete analysis and setup
2. **Record Settings**: Note raw material requirements
3. **Load Next Part**: Automatic clearing and new analysis
4. **Compare Results**: Optimize for material efficiency

## Troubleshooting

### Common Issues

**Chuck Not Loading:**
- Verify the chuck STEP file exists at the specified path
- Check file permissions and accessibility
- Look for error messages in the output log
- Ensure OpenCASCADE libraries are properly installed

**No Cylinder Detection:**
- Ensure your workpiece contains cylindrical features
- Check that cylinder diameters are within 5-500mm range
- Verify STEP file contains valid geometry
- Try with a known working STEP file for comparison

**Raw Material Not Visible:**
- Check transparency settings (default 70%)
- Ensure workpiece analysis completed successfully
- Look for error messages in the output window
- Verify the detected cylinder has valid dimensions

**Display Issues:**
- Check graphics driver compatibility
- Verify OpenGL support is available
- Try resizing the window to trigger redraw
- Check system requirements for OpenCASCADE

### Debug Information

**Output Window:**
The output log shows detailed information about:
- Chuck loading status and file validation
- Workpiece processing and cylinder detection results
- Raw material calculations and sizing decisions
- Error messages and warnings with context

**Console Output (Advanced):**
Additional technical details available in debug console:
- Detected cylinder dimensions and coordinates
- Standard diameter selection process
- Geometric transformation details
- OpenCASCADE operation status

## Advanced Usage

### Understanding Component Separation

While the system appears seamless, understanding the component architecture can help with troubleshooting:

**Chuck Manager:**
- Handles only chuck-related operations
- Independent of workpiece processing
- Uses dependency injection for STEP loading

**Workpiece Manager:**
- Focuses solely on workpiece analysis
- Performs geometric computations
- Provides cylinder detection results

**Raw Material Manager:**
- Specializes in material sizing and display
- Manages standard diameter database
- Handles transparency and visual properties

### Integration Benefits

**Modular Design:**
- Each component can be tested independently
- Issues are isolated to specific functionality
- Updates and improvements are component-specific

**Performance Optimization:**
- Efficient memory usage per component
- Selective processing based on component needs
- Optimized rendering for each material type

## Tips and Best Practices

### Workpiece Design
- Include clear cylindrical features for automatic detection
- Use standard diameters when possible to minimize material waste
- Consider chuck capacity when designing parts
- Ensure STEP files contain high-quality geometry

### File Management
- Keep chuck STEP file in the designated location
- Use descriptive names for workpiece STEP files
- Organize projects by part families or material types
- Maintain backup copies of critical STEP files

### Workflow Optimization
1. **Load workpiece** and verify automatic alignment
2. **Check raw material** diameter and length calculations
3. **Verify chuck clearances** and part accessibility
4. **Review geometry** for any detection issues
5. **Proceed with tool** and operation setup

## System Requirements

### Hardware Requirements
- OpenGL-capable graphics card
- Sufficient RAM for OpenCASCADE operations
- Multi-core processor recommended for complex geometry

### Software Requirements
- Windows 10 or later
- OpenCASCADE 7.6.0 or compatible
- Qt 6.9.0 or compatible
- Visual C++ runtime libraries

## Future Enhancements

Planned improvements include:
- Manual cylinder selection interface
- Custom raw material dimensions
- Multiple workpiece support with collision detection
- Advanced fixture positioning options
- Tool path validation against chuck geometry
- Setup optimization suggestions
- Integration with CAM planning tools

## Getting Help

**Error Messages:**
- Check the output window for detailed error information
- Common issues are usually related to file paths or geometry quality
- Error messages include context and suggested solutions

**Technical Support:**
- Review the debug output for technical details
- Check the architecture documentation for implementation details
- Consult the API documentation for developer information

**Community Support:**
- Share STEP files that demonstrate issues
- Provide output log information when reporting problems
- Include system specifications and software versions

This integrated chuck management system provides a solid foundation for CNC turning operations in IntuiCAM, with intelligent automation and reliable component separation for maintainable, extensible functionality. 