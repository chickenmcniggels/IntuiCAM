# Chuck Management API Documentation

## Overview

The IntuiCAM chuck management system provides a modular architecture for intelligent workpiece alignment and raw material sizing. The system is built with clean separation of concerns, using dedicated managers for different aspects of the workflow and dependency injection for better testability and maintainability.

## Architecture Components

### Core Interfaces

#### IStepLoader Interface

The `IStepLoader` interface provides an abstraction for STEP file loading, enabling dependency injection and loose coupling.

```cpp
#include "isteploader.h"

class IStepLoader
{
public:
    virtual ~IStepLoader() = default;
    
    /**
     * @brief Load a STEP file and return the shape
     * @param filename Path to the STEP file
     * @return Loaded shape, or null shape if failed
     */
    virtual TopoDS_Shape loadStepFile(const QString& filename) = 0;
    
    /**
     * @brief Get the last error message
     * @return Error message string
     */
    virtual QString getLastError() const = 0;
    
    /**
     * @brief Check if the last operation was successful
     * @return True if valid, false otherwise
     */
    virtual bool isValid() const = 0;
};
```

**Benefits:**
- Decouples managers from concrete STEP loading implementation
- Enables dependency injection for better testability
- Follows SOLID principles (Dependency Inversion)

### Manager Classes

#### ChuckManager Class

The `ChuckManager` class handles only chuck-specific functionality with clean separation of concerns.

```cpp
#include "chuckmanager.h"

class ChuckManager : public QObject
{
    Q_OBJECT

public:
    explicit ChuckManager(QObject *parent = nullptr);
    ~ChuckManager();

    /**
     * @brief Initialize the chuck manager with AIS context and STEP loader
     * @param context OpenCASCADE AIS context for 3D display
     * @param stepLoader STEP file loader interface (dependency injection)
     */
    void initialize(Handle(AIS_InteractiveContext) context, IStepLoader* stepLoader);

    /**
     * @brief Load and display the 3-jaw chuck permanently
     * @param chuckFilePath Path to the chuck STEP file
     * @return True if loaded successfully, false otherwise
     */
    bool loadChuck(const QString& chuckFilePath);

    /**
     * @brief Clear chuck display
     */
    void clearChuck();

    /**
     * @brief Get the chuck shape if loaded
     * @return Chuck shape, or null shape if not loaded
     */
    TopoDS_Shape getChuckShape() const { return m_chuckShape; }

    /**
     * @brief Check if chuck is loaded and displayed
     * @return True if chuck is loaded, false otherwise
     */
    bool isChuckLoaded() const { return !m_chuckShape.IsNull(); }

signals:
    /**
     * @brief Emitted when chuck is successfully loaded
     */
    void chuckLoaded();

    /**
     * @brief Emitted when an error occurs
     * @param message Error description
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    IStepLoader* m_stepLoader; // Dependency injection
    TopoDS_Shape m_chuckShape;
    Handle(AIS_Shape) m_chuckAIS;
    
    void setChuckMaterial(Handle(AIS_Shape) chuckAIS);
};
```

**Key Features:**
- **Single Responsibility**: Only handles chuck-related operations
- **Dependency Injection**: Uses IStepLoader interface for STEP loading
- **Clean Interface**: Simple, focused public API
- **Error Handling**: Comprehensive validation and error reporting

#### WorkpieceManager Class

The `WorkpieceManager` class handles workpiece display and geometric analysis.

```cpp
#include "workpiecemanager.h"

class WorkpieceManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkpieceManager(QObject *parent = nullptr);
    ~WorkpieceManager();

    /**
     * @brief Initialize with AIS context
     * @param context OpenCASCADE AIS context for 3D display
     */
    void initialize(Handle(AIS_InteractiveContext) context);

    /**
     * @brief Add a workpiece to the scene
     * @param workpiece The workpiece shape to add
     * @return True if added successfully, false otherwise
     */
    bool addWorkpiece(const TopoDS_Shape& workpiece);

    /**
     * @brief Detect cylindrical features in a workpiece
     * @param workpiece The workpiece shape to analyze
     * @return Vector of cylinder axes found in the workpiece
     */
    QVector<gp_Ax1> detectCylinders(const TopoDS_Shape& workpiece);

    /**
     * @brief Clear all workpieces
     */
    void clearWorkpieces();

    /**
     * @brief Get all current workpieces
     * @return Vector of AIS shapes representing workpieces
     */
    QVector<Handle(AIS_Shape)> getWorkpieces() const { return m_workpieces; }

    /**
     * @brief Get the main cylinder axis from the current workpiece
     * @return Axis of the main detected cylinder
     */
    gp_Ax1 getMainCylinderAxis() const { return m_mainCylinderAxis; }

    /**
     * @brief Get the detected cylinder diameter
     * @return Diameter of the largest detected cylinder
     */
    double getDetectedDiameter() const { return m_detectedDiameter; }

signals:
    /**
     * @brief Emitted when a cylinder is detected in a workpiece
     * @param diameter Cylinder diameter in mm
     * @param length Estimated cylinder length in mm
     * @param axis Cylinder axis orientation
     */
    void cylinderDetected(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Emitted when an error occurs
     * @param message Error description
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    QVector<Handle(AIS_Shape)> m_workpieces;
    gp_Ax1 m_mainCylinderAxis;
    double m_detectedDiameter;
    
    void analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders);
    void setWorkpieceMaterial(Handle(AIS_Shape) workpieceAIS);
};
```

**Key Features:**
- **Geometric Analysis**: Advanced cylinder detection using OpenCASCADE topology exploration
- **Material Management**: Handles workpiece-specific visual properties
- **Signal-Based Communication**: Emits signals for detected cylinders
- **Performance Optimized**: Efficient filtering and analysis algorithms

#### RawMaterialManager Class

The `RawMaterialManager` class handles raw material creation and sizing.

```cpp
#include "rawmaterialmanager.h"

class RawMaterialManager : public QObject
{
    Q_OBJECT

public:
    explicit RawMaterialManager(QObject *parent = nullptr);
    ~RawMaterialManager();

    // Standard material diameters in mm (ISO metric standard stock sizes)
    static const QVector<double> STANDARD_DIAMETERS;

    /**
     * @brief Initialize with AIS context
     * @param context OpenCASCADE AIS context for 3D display
     */
    void initialize(Handle(AIS_InteractiveContext) context);

    /**
     * @brief Create and display raw material cylinder
     * @param diameter Raw material diameter in mm
     * @param length Raw material length in mm
     * @param axis Cylinder axis for positioning
     */
    void displayRawMaterial(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Find the next largest standard diameter for a given diameter
     * @param diameter Input diameter in mm
     * @return Next larger standard diameter in mm
     */
    double getNextStandardDiameter(double diameter);

    /**
     * @brief Clear raw material display
     */
    void clearRawMaterial();

    /**
     * @brief Set transparency for raw material display
     * @param transparency Transparency value (0.0 = opaque, 1.0 = fully transparent)
     */
    void setRawMaterialTransparency(double transparency = 0.7);

    /**
     * @brief Get current raw material shape
     * @return Current raw material shape, or null shape if none
     */
    TopoDS_Shape getCurrentRawMaterial() const { return m_currentRawMaterial; }

    /**
     * @brief Check if raw material is currently displayed
     * @return True if raw material is displayed, false otherwise
     */
    bool isRawMaterialDisplayed() const { return !m_rawMaterialAIS.IsNull(); }

signals:
    /**
     * @brief Emitted when raw material is created and displayed
     * @param diameter Raw material diameter in mm
     * @param length Raw material length in mm
     */
    void rawMaterialCreated(double diameter, double length);

    /**
     * @brief Emitted when an error occurs
     * @param message Error description
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    Handle(AIS_Shape) m_rawMaterialAIS;
    TopoDS_Shape m_currentRawMaterial;
    double m_rawMaterialTransparency;
    
    TopoDS_Shape createCylinder(double diameter, double length, const gp_Ax1& axis);
    void setRawMaterialMaterial(Handle(AIS_Shape) rawMaterialAIS);
};
```

**Key Features:**
- **Standard Sizing**: Comprehensive ISO metric standard diameter database
- **Material Properties**: Transparent brass appearance with configurable transparency
- **Geometric Creation**: Precise cylinder creation with proper axis alignment
- **Smart Sizing**: Automatic selection of optimal standard diameters

### Integration Component

#### OpenGL3DWidget Integration

The `OpenGL3DWidget` orchestrates all managers and provides the main integration point.

```cpp
class OpenGL3DWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGL3DWidget(QWidget *parent = nullptr);
    ~OpenGL3DWidget();

    // Component managers
    ChuckManager* getChuckManager() const { return m_chuckManager; }
    WorkpieceManager* getWorkpieceManager() const { return m_workpieceManager; }
    RawMaterialManager* getRawMaterialManager() const { return m_rawMaterialManager; }
    
    /**
     * @brief Initialize chuck with default chuck file
     * @param chuckFilePath Path to the chuck STEP file
     */
    void initializeChuck(const QString& chuckFilePath);
    
    /**
     * @brief Add workpiece with automatic alignment and raw material creation
     * @param workpiece The workpiece shape to add
     */
    void addWorkpiece(const TopoDS_Shape& workpiece);

    // Standard 3D viewer functionality
    void displayShape(const TopoDS_Shape& shape);
    void clearAll();
    void fitAll();
    Handle(AIS_InteractiveContext) getContext() const { return m_context; }

private:
    ChuckManager* m_chuckManager;
    WorkpieceManager* m_workpieceManager;
    RawMaterialManager* m_rawMaterialManager;
    
    // OpenCASCADE components
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(WNT_Window) m_window;
};
```

## Usage Examples

### Basic Setup and Initialization

```cpp
// Create the 3D widget (managers are created automatically)
OpenGL3DWidget* viewer = new OpenGL3DWidget(parent);

// Get manager references
ChuckManager* chuckManager = viewer->getChuckManager();
WorkpieceManager* workpieceManager = viewer->getWorkpieceManager();
RawMaterialManager* rawMaterialManager = viewer->getRawMaterialManager();

// Initialize chuck (typically called automatically)
viewer->initializeChuck("chuck.step");
```

### Workpiece Processing Workflow

```cpp
// Load workpiece using the integrated workflow
TopoDS_Shape workpiece = stepLoader->loadStepFile("part.step");
viewer->addWorkpiece(workpiece);

// Alternative: Manual component interaction
workpieceManager->addWorkpiece(workpiece);
QVector<gp_Ax1> cylinders = workpieceManager->detectCylinders(workpiece);

if (!cylinders.isEmpty()) {
    double detectedDiameter = workpieceManager->getDetectedDiameter();
    gp_Ax1 mainAxis = workpieceManager->getMainCylinderAxis();
    
    double rawDiameter = rawMaterialManager->getNextStandardDiameter(detectedDiameter);
    rawMaterialManager->displayRawMaterial(rawDiameter, 150.0, mainAxis);
}
```

### Component Coordination with Signals

```cpp
// Connect workpiece analysis to raw material creation
connect(workpieceManager, &WorkpieceManager::cylinderDetected,
        [this, rawMaterialManager](double diameter, double length, const gp_Ax1& axis) {
            double rawDiameter = rawMaterialManager->getNextStandardDiameter(diameter);
            rawMaterialManager->displayRawMaterial(rawDiameter, length * 1.2, axis);
        });

// Handle errors from any component
connect(chuckManager, &ChuckManager::errorOccurred,
        this, &MainWindow::handleChuckError);
connect(workpieceManager, &WorkpieceManager::errorOccurred,
        this, &MainWindow::handleWorkpieceError);
connect(rawMaterialManager, &RawMaterialManager::errorOccurred,
        this, &MainWindow::handleRawMaterialError);
```

### Advanced Material and Display Control

```cpp
// Customize raw material appearance
rawMaterialManager->setRawMaterialTransparency(0.5); // 50% transparent

// Check component status
if (chuckManager->isChuckLoaded()) {
    TopoDS_Shape chuck = chuckManager->getChuckShape();
    // Process chuck geometry
}

if (rawMaterialManager->isRawMaterialDisplayed()) {
    TopoDS_Shape rawMaterial = rawMaterialManager->getCurrentRawMaterial();
    // Analyze raw material requirements
}

// Get workpiece analysis results
QVector<Handle(AIS_Shape)> workpieces = workpieceManager->getWorkpieces();
double diameter = workpieceManager->getDetectedDiameter();
gp_Ax1 axis = workpieceManager->getMainCylinderAxis();
```

### Dependency Injection Example

```cpp
// Custom STEP loader implementation
class CustomStepLoader : public IStepLoader {
public:
    TopoDS_Shape loadStepFile(const QString& filename) override {
        // Custom implementation
        return shape;
    }
    
    QString getLastError() const override {
        return m_lastError;
    }
    
    bool isValid() const override {
        return m_isValid;
    }
    
private:
    QString m_lastError;
    bool m_isValid;
};

// Inject custom loader
CustomStepLoader* customLoader = new CustomStepLoader();
chuckManager->initialize(context, customLoader);
```

## Standard Diameter Database

The `RawMaterialManager` includes a comprehensive database of ISO metric standard turning stock diameters:

```cpp
const QVector<double> RawMaterialManager::STANDARD_DIAMETERS = {
    // Common turning stock diameters (mm)
    6.0, 8.0, 10.0, 12.0, 16.0, 20.0, 25.0, 30.0, 
    32.0, 40.0, 50.0, 60.0, 63.0, 80.0, 100.0, 110.0, 
    125.0, 140.0, 160.0, 180.0, 200.0, 220.0, 250.0, 
    280.0, 315.0, 355.0, 400.0, 450.0, 500.0
};
```

**Selection Algorithm:**
```cpp
double RawMaterialManager::getNextStandardDiameter(double diameter)
{
    for (double standardDiam : STANDARD_DIAMETERS) {
        if (standardDiam > diameter) {
            return standardDiam;
        }
    }
    
    // For oversized parts, round up to next 50mm increment
    return qCeil(diameter / 50.0) * 50.0;
}
```

## Error Handling

### Error Types and Handling

Each manager provides specific error handling with descriptive messages:

```cpp
// Chuck loading errors
connect(chuckManager, &ChuckManager::errorOccurred,
        [](const QString& message) {
            qDebug() << "Chuck error:" << message;
            // Handle: file not found, invalid STEP file, context issues
        });

// Workpiece analysis errors  
connect(workpieceManager, &WorkpieceManager::errorOccurred,
        [](const QString& message) {
            qDebug() << "Workpiece error:" << message;
            // Handle: invalid geometry, no cylinders found, analysis failure
        });

// Raw material creation errors
connect(rawMaterialManager, &RawMaterialManager::errorOccurred,
        [](const QString& message) {
            qDebug() << "Raw material error:" << message;
            // Handle: geometry creation failure, invalid parameters
        });
```

### Common Error Scenarios

1. **File Access Issues**
   - Chuck STEP file not found
   - Permission denied
   - Invalid file format

2. **Geometric Analysis Issues**
   - No cylindrical features detected
   - Invalid geometry in STEP file
   - Unsupported feature types

3. **Display and Rendering Issues**
   - OpenCASCADE context not initialized
   - Graphics driver compatibility
   - Memory allocation failures

## Performance Considerations

### Optimization Strategies

1. **Component Isolation**
   - Each manager operates independently
   - Failures in one component don't affect others
   - Selective processing based on component needs

2. **Efficient Geometric Analysis**
   - Optimized cylinder detection algorithms
   - Filtering to relevant diameter ranges (5-500mm)
   - Caching of analysis results

3. **Memory Management**
   - Smart pointers for OpenCASCADE objects
   - Proper cleanup in destructors
   - Selective object creation and destruction

4. **Rendering Optimization**
   - Material property caching per manager
   - Selective redraw operations
   - Efficient transparency handling

### Performance Monitoring

```cpp
// Monitor cylinder detection performance
QElapsedTimer timer;
timer.start();

QVector<gp_Ax1> cylinders = workpieceManager->detectCylinders(workpiece);

qDebug() << "Cylinder detection took:" << timer.elapsed() << "ms";
qDebug() << "Found" << cylinders.size() << "cylinders";
```

## Testing and Validation

### Unit Testing Strategy

Each manager can be tested independently due to clean separation of concerns:

```cpp
// Test ChuckManager with mock STEP loader
class MockStepLoader : public IStepLoader {
    // Implement mock behavior for testing
};

// Test WorkpieceManager with known geometries
void testCylinderDetection() {
    WorkpieceManager manager;
    TopoDS_Shape testCylinder = createTestCylinder(25.0, 100.0);
    
    QVector<gp_Ax1> cylinders = manager.detectCylinders(testCylinder);
    QCOMPARE(cylinders.size(), 1);
    QCOMPARE(manager.getDetectedDiameter(), 25.0);
}

// Test RawMaterialManager diameter calculations
void testStandardDiameters() {
    RawMaterialManager manager;
    QCOMPARE(manager.getNextStandardDiameter(24.5), 25.0);
    QCOMPARE(manager.getNextStandardDiameter(31.0), 32.0);
}
```

### Integration Testing

```cpp
// Test component coordination
void testWorkpieceWorkflow() {
    OpenGL3DWidget viewer;
    TopoDS_Shape workpiece = loadTestWorkpiece();
    
    viewer.addWorkpiece(workpiece);
    
    // Verify all components updated correctly
    QVERIFY(viewer.getWorkpieceManager()->getDetectedDiameter() > 0);
    QVERIFY(viewer.getRawMaterialManager()->isRawMaterialDisplayed());
}
```

## Thread Safety

### Current Implementation
- All managers are designed for single-threaded use
- Qt's signal-slot mechanism handles cross-thread communication
- OpenCASCADE operations are performed on the main GUI thread

### Future Considerations
- Geometric analysis could be moved to worker threads
- Thread-safe result caching mechanisms
- Asynchronous STEP file loading for large files

## API Versioning and Compatibility

### Current Version: 1.0.0
- Initial modular architecture
- Clean separation of concerns
- Dependency injection support
- Comprehensive signal-based communication

### Backward Compatibility
- Maintains compatibility with existing STEP files
- Standard diameter database can be extended
- Component interfaces designed for future enhancement

### Future API Enhancements
- Additional geometric analysis methods
- Custom material property management
- Multiple workpiece support
- Advanced fixture positioning algorithms

This API documentation provides comprehensive coverage of the modular chuck management system, enabling developers to effectively integrate, extend, and maintain the functionality within IntuiCAM. 