# Workspace Management System

## Overview

IntuiCAM includes a modular workspace management system that provides intelligent workpiece alignment and raw material sizing for CNC turning operations. The system is built with a clean separation of concerns, using a dedicated WorkspaceController to coordinate specialized managers for different aspects of the workflow.

## Architecture

The workspace management system follows the modular architecture principles with clear component separation:

### Core Architecture Components

#### WorkspaceController
**Responsibility:** Top-level business logic coordination
- Orchestrates complete workflows between specialized managers
- Provides unified error handling and progress reporting
- Manages component lifecycle and initialization
- Coordinates chuck setup, workpiece processing, and raw material creation
- Uses dependency injection for testability and modularity

#### OpenGL3DWidget
**Responsibility:** Pure 3D visualization component
- Handles OpenCASCADE rendering pipeline setup
- Manages user interaction (mouse events, view manipulation)
- Provides basic display operations (show shape, clear, fit view)
- Emits signals for controller coordination
- No business logic - focused solely on visualization

#### Specialized Domain Managers

##### ChuckManager
**Responsibility:** Chuck-specific functionality only
- Loads and displays the 3-jaw chuck STEP file permanently
- Manages chuck material properties and positioning
- Handles chuck status and configuration
- Uses dependency injection for STEP loading (IStepLoader interface)

##### WorkpieceManager
**Responsibility:** Workpiece handling and analysis
- Workpiece loading and display
- Cylinder detection in workpieces
- Geometric analysis and feature extraction
- Workpiece material properties
- Self-contained analysis algorithms

##### RawMaterialManager
**Responsibility:** Raw material creation and sizing
- Raw material cylinder creation and display
- Standard diameter matching with ISO metric standards
- Material properties and transparency control
- Sizing calculations and optimization
- Independent material creation logic

##### IStepLoader Interface
**Responsibility:** STEP file loading abstraction
- Provides a clean interface for STEP file operations
- Enables dependency injection and testability
- Implemented by StepLoader class
- Follows SOLID principles (Dependency Inversion)

## Features

### Automatic Chuck Display
- The 3-jaw chuck STEP file is permanently displayed in the workspace
- Realistic metallic appearance with proper material properties
- Chuck remains visible at all times during workpiece operations
- Managed by ChuckManager under WorkspaceController coordination

### Intelligent Workpiece Analysis
- Automatic detection of cylindrical features in imported workpieces
- Intelligent filtering to identify turning-relevant cylinders (5-500mm diameter)
- Real-time feedback on detected cylinder parameters
- Geometric analysis handled by WorkpieceManager

### Standard Material Matching
- Comprehensive list of ISO metric standard stock diameters
- Automatic selection of next larger standard diameter for raw material
- Support for common turning stock sizes from 6mm to 500mm
- Custom diameter calculation for oversized parts
- Managed by RawMaterialManager

### Raw Material Visualization
- Transparent raw material cylinder display
- Automatic positioning and alignment with workpiece axis
- Configurable transparency levels for optimal visualization
- Real-time updates when workpieces change

## Usage

### Automatic Operation
1. **Workspace Initialization**: WorkspaceController coordinates all component setup
2. **Chuck Initialization**: WorkspaceController manages chuck loading through ChuckManager
3. **Workpiece Import**: WorkspaceController handles STEP file processing workflow
4. **Automatic Analysis**: WorkpieceManager detects cylinders and extracts dimensions
5. **Raw Material Creation**: RawMaterialManager generates and displays transparent stock material

### Component Coordination
The WorkspaceController orchestrates the interaction between all components:
```cpp
// Workspace-managed workflow
workspaceController->initializeChuck(chuckFilePath);
workspaceController->addWorkpiece(shape);
// Controller automatically coordinates:
// - WorkpieceManager analysis
// - RawMaterialManager sizing and display
// - Error handling and progress reporting
```

### Separation of Concerns
```cpp
// UI Layer (MainWindow)
connect(workspaceController, &WorkspaceController::workpieceWorkflowCompleted,
        this, &MainWindow::handleWorkpieceWorkflowCompleted);

// Business Logic Layer (WorkspaceController)  
workspaceController->executeWorkpieceWorkflow(workpiece);

// Visualization Layer (OpenGL3DWidget)
viewer->fitAll(); // Pure visualization operation
```

## Standard Diameter Database

The RawMaterialManager includes a comprehensive database of standard turning stock diameters:

```cpp
// Common turning stock diameters (mm) - ISO metric standards
6.0, 8.0, 10.0, 12.0, 16.0, 20.0, 25.0, 30.0, 
32.0, 40.0, 50.0, 60.0, 63.0, 80.0, 100.0, 110.0, 
125.0, 140.0, 160.0, 180.0, 200.0, 220.0, 250.0, 
280.0, 315.0, 355.0, 400.0, 450.0, 500.0
```

For parts requiring larger diameters, the system automatically rounds up to the next 50mm increment.

## Technical Architecture

### Separation of Concerns

#### WorkspaceController Class
**Unified workflow coordination:**
- `initializeChuck()`: Coordinates chuck setup through ChuckManager
- `addWorkpiece()`: Manages complete workpiece processing pipeline
- `clearWorkpieces()`: Handles cleanup while preserving chuck
- `clearWorkspace()`: Complete workspace reset including chuck
- `executeWorkpieceWorkflow()`: Orchestrates multi-step processing

**Dependencies and Initialization:**
- Requires AIS context and STEP loader for initialization
- Coordinates all manager initialization and dependencies
- Provides unified error handling with source identification

#### OpenGL3DWidget Class
**Pure visualization operations:**
- `initializeViewer()`: Sets up OpenCASCADE rendering pipeline
- `displayShape()`: Basic shape display operations
- `fitAll()`: View manipulation and camera controls
- Mouse interaction handling (rotation, panning, zooming)
- `viewerInitialized()` signal for controller coordination

**No Business Logic:**
- Focused solely on 3D rendering and user interaction
- Does not manage workflows or coordinate between components
- Provides AIS context to WorkspaceController for manager initialization

### Interface Design

#### IStepLoader Interface
```cpp
class IStepLoader {
public:
    virtual ~IStepLoader() = default;
    virtual TopoDS_Shape loadStepFile(const QString& filename) = 0;
    virtual QString getLastError() const = 0;
    virtual bool isValid() const = 0;
};
```

**Benefits:**
- Decouples managers from concrete STEP loading implementation
- Enables dependency injection for better testability
- Follows SOLID principles (Dependency Inversion)

#### WorkspaceController Signal Interface
```cpp
signals:
    void chuckInitialized();
    void workpieceWorkflowCompleted(double diameter, double rawMaterialDiameter);
    void workspaceCleared();
    void errorOccurred(const QString& source, const QString& message);
```

### Integration Pattern

The system follows a clear layered architecture:

**UI Layer (MainWindow):**
- Handles user events and file dialogs
- Delegates to WorkspaceController for business operations
- Updates UI based on controller signals
- No direct manager access

**Controller Layer (WorkspaceController):**
- Coordinates workflows between managers
- Handles business logic and error cases
- Provides unified interface to UI layer
- Manages component lifecycle

**Manager Layer (ChuckManager, WorkpieceManager, RawMaterialManager):**
- Focused domain-specific functionality
- No knowledge of other managers
- Communicate through controller coordination
- Testable in isolation

**Visualization Layer (OpenGL3DWidget):**
- Pure rendering and interaction
- No business logic or workflow knowledge
- Provides AIS context for manager display operations

## Configuration

### Chuck File Location
Chuck STEP file path configured in MainWindow:
```cpp
QString chuckFilePath = "C:/Users/nikla/Downloads/three_jaw_chuck.step";
```

### Material Properties
Each manager handles its own material properties:

**Chuck (ChuckManager):**
- Steel appearance with metallic gray coloring
- High shininess and reflectivity

**Workpiece (WorkpieceManager):**
- Aluminum-like appearance (blue-gray)
- Standard metallic properties

**Raw Material (RawMaterialManager):**
- Brass-colored with configurable transparency
- Default 70% transparency

## Examples

### Component Initialization
```cpp
// Create workspace controller (automatically creates managers)
WorkspaceController* workspaceController = new WorkspaceController(parent);

// Initialize when 3D viewer is ready
workspaceController->initialize(viewer->getContext(), stepLoader);

// Initialize chuck
workspaceController->initializeChuck("chuck.step");
```

### Workpiece Processing Workflow
```cpp
// Complete workflow through controller
TopoDS_Shape workpiece = stepLoader->loadStepFile("part.step");
bool success = workspaceController->addWorkpiece(workpiece);

// Controller automatically handles:
// - WorkpieceManager::addWorkpiece()
// - WorkpieceManager::detectCylinders()
// - RawMaterialManager::getNextStandardDiameter()
// - RawMaterialManager::displayRawMaterial()
// - Error handling and progress reporting
```

### Error Handling
```cpp
// Unified error handling from controller
connect(workspaceController, &WorkspaceController::errorOccurred,
        [](const QString& source, const QString& message) {
            qDebug() << "Error in" << source << ":" << message;
        });

// Workflow completion handling
connect(workspaceController, &WorkspaceController::workpieceWorkflowCompleted,
        [](double diameter, double rawMaterialDiameter) {
            qDebug() << "Workflow completed - Detected:" << diameter 
                     << "mm, Raw material:" << rawMaterialDiameter << "mm";
        });
```

## Testing and Validation

### Unit Testing Strategy
The modular architecture enables comprehensive testing:

**WorkspaceController Testing:**
- Mock managers for workflow testing
- Signal emission verification
- Error handling validation

**Manager Testing:**
- **ChuckManager**: Mock IStepLoader for STEP loading tests
- **WorkpieceManager**: Test geometric analysis with known shapes
- **RawMaterialManager**: Validate standard diameter calculations

**OpenGL3DWidget Testing:**
- Viewer initialization testing
- User interaction simulation

### Integration Testing
- Component coordination through WorkspaceController
- End-to-end workflow validation
- Signal-slot communication verification
- Visual rendering verification

### Architectural Benefits for Testing
- Clear separation enables isolated unit testing
- Dependency injection facilitates mocking
- Controller coordination is easily testable
- UI layer can be tested independently of business logic 

# Chuck Management

## Overview

The `ChuckManager` handles the persistent display and management of the 3-jaw chuck in IntuiCAM's 3D viewer. The chuck serves as a reference object for workpiece alignment and positioning.

## Non-Selectable Chuck Implementation

### Problem
By default, all AIS objects displayed in the OpenCASCADE viewer are selectable and show hover highlighting. For the chuck, which serves as a permanent reference fixture, this behavior is undesirable as:

1. **User Experience**: Users should not accidentally select the chuck when trying to select workpieces
2. **Visual Clarity**: Hover highlighting on the chuck can be distracting during workpiece selection
3. **Workflow**: The chuck is a fixture, not a workpiece, so it shouldn't be part of the selection workflow

### Solution
The implementation uses OpenCASCADE AIS APIs to make the chuck non-selectable and disable hover highlighting:

```cpp
// Method 1: Disable automatic highlighting for this specific object
m_context->SetAutomaticHilight(m_chuckAIS, false);

// Method 2: Deactivate all selection modes for the chuck to prevent selection
m_context->Deactivate(m_chuckAIS);
```

### Implementation Details

#### 1. SetAutomaticHilight(object, false)
- **Purpose**: Disables hover highlighting for the specific chuck object
- **Effect**: The chuck will not highlight when the mouse moves over it
- **Scope**: Object-specific, doesn't affect other objects in the scene

#### 2. Deactivate(object)
- **Purpose**: Removes the object from all active selection modes
- **Effect**: The chuck cannot be selected via mouse clicks
- **Scope**: Object-specific, prevents chuck from being detected during MoveTo operations

#### 3. Verification Method
The `isChuckNonSelectable()` method provides verification that the chuck is properly configured:

```cpp
bool ChuckManager::isChuckNonSelectable() const
{
    // Check if automatic highlighting is disabled
    bool highlightDisabled = !m_context->AutomaticHilight(m_chuckAIS);
    
    // Verify no active selection modes
    bool hasActiveSelectionModes = false;
    for (int mode = 0; mode <= 10; ++mode) {
        if (m_context->IsActivated(m_chuckAIS, mode)) {
            hasActiveSelectionModes = true;
            break;
        }
    }
    
    return !hasActiveSelectionModes && highlightDisabled;
}
```

### Integration with Context7 Research

This implementation was developed using Context7 to research OpenCASCADE AIS best practices:

1. **API Research**: Context7 provided documentation on `AIS_InteractiveContext` methods for selection control
2. **Best Practices**: Learned proper sequence of operations (Display first, then configure selection)
3. **Verification Patterns**: Discovered methods to verify object selection state

### Benefits

1. **Improved User Experience**: Chuck doesn't interfere with workpiece selection workflows
2. **Visual Clarity**: No distracting hover effects on the reference fixture
3. **Professional Behavior**: Matches expectations for CAM software where fixtures are non-interactive
4. **Robust Implementation**: Includes verification to ensure proper configuration

### Future Considerations

- **Custom AIS Object**: For more complex chuck interactions, consider creating a custom `AIS_InteractiveObject` that inherits from `AIS_Shape` and overrides `ComputeSelection()` to be empty
- **Context Menu**: Could add right-click context menu for chuck-specific operations while maintaining non-selectability for primary interactions
- **Visual Indicators**: Could add visual styling to clearly indicate the chuck is a fixture (e.g., different material properties or transparency)

## Usage

The chuck is automatically configured as non-selectable when loaded through the `ChuckManager::loadChuck()` method. No additional configuration is required from the UI layer.

The verification can be called manually for debugging:
```cpp
if (chuckManager->isChuckNonSelectable()) {
    qDebug() << "Chuck properly configured as non-selectable";
}
``` 