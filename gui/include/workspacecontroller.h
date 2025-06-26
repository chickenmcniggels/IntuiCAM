#ifndef WORKSPACECONTROLLER_H
#define WORKSPACECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QVector>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Circ.hxx>
#include <Precision.hxx>

// IntuiCAM includes
#include <IntuiCAM/Toolpath/ToolpathGenerationPipeline.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/OperationParameterManager.h>
#include <IntuiCAM/Toolpath/Operations.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Geometry/Types.h>

// Forward declarations
class ChuckManager;
class WorkpieceManager;
class RawMaterialManager;
class WorkspaceCoordinateManager;
class OpenGL3DWidget;
namespace IntuiCAM { namespace Geometry { class IStepLoader; } }
using IStepLoader = IntuiCAM::Geometry::IStepLoader;

// Include CylinderInfo structure
struct CylinderInfo;

/**
 * @brief Top-level workspace controller that orchestrates all CAM workflow components
 * 
 * This controller follows the modular architecture principles by:
 * - Providing clear separation of concerns between UI and business logic
 * - Coordinating workflow between specialized managers
 * - Maintaining clean API boundaries for reusability
 * - Supporting extensibility for future CAM operations
 * 
 * The WorkspaceController manages:
 * - Chuck setup and display
 * - Workpiece loading and analysis
 * - Raw material sizing and positioning
 * - Workflow coordination and error handling
 */
class WorkspaceController : public QObject
{
    Q_OBJECT

public:
    explicit WorkspaceController(QObject *parent = nullptr);
    ~WorkspaceController();

    /**
     * @brief Initialize the workspace with required dependencies
     * @param context OpenCASCADE AIS context for 3D display
     * @param stepLoader STEP file loader interface
     */
    void initialize(Handle(AIS_InteractiveContext) context, IStepLoader* stepLoader);

    /**
     * @brief Initialize chuck fixture in the workspace
     * @param chuckFilePath Path to the chuck STEP file
     * @return True if chuck loaded successfully
     */
    bool initializeChuck(const QString& chuckFilePath);

    /**
     * @brief Add a workpiece to the workspace with full workflow processing
     * @param workpiece The workpiece shape to add
     * @return True if workpiece processed successfully
     */
    bool addWorkpiece(const TopoDS_Shape& workpiece);

    /**
     * @brief Manually select which detected cylinder to use as the main axis
     * @param cylinderIndex Index of the cylinder in the detected cylinders list
     * @return True if selection was successful
     */
    bool selectWorkpieceCylinderAxis(int cylinderIndex);

    /**
     * @brief Get information about all detected cylinders
     * @return Vector of CylinderInfo structures
     */
    QVector<CylinderInfo> getDetectedCylinders() const;

    /**
     * @brief Check if chuck centerline has been detected
     * @return True if chuck has a valid centerline
     */
    bool hasChuckCenterline() const;

    /**
     * @brief Get the chuck centerline axis
     * @return The chuck centerline axis
     */
    gp_Ax1 getChuckCenterlineAxis() const;

    /**
     * @brief Clear all workpieces while preserving chuck
     */
    void clearWorkpieces();

    /**
     * @brief Clear entire workspace including chuck
     */
    void clearWorkspace();

    /**
     * @brief Check if workspace is properly initialized
     */
    bool isInitialized() const;

    /**
     * @brief Check if chuck is loaded in workspace
     */
    bool isChuckLoaded() const;

    /**
     * @brief Update raw material diameter with current workpiece and axis
     * @param diameter New raw material diameter in mm
     * @return True if update was successful
     */
    bool updateRawMaterialDiameter(double diameter);

    /**
     * @brief Update distance to chuck (workpiece positioning)
     * @param distance Distance from chuck face in mm
     * @return True if update was successful
     */
    bool updateDistanceToChuck(double distance);

    /**
     * @brief Flip the workpiece orientation
     * @param flipped True to flip orientation, false to restore
     * @return True if flip was successful
     */
    bool flipWorkpieceOrientation(bool flipped);

    /**
     * @brief Apply all current part loading settings from the panel
     * @param distance Distance to chuck
     * @param diameter Raw material diameter
     * @param flipped Orientation flipped state
     * @return True if all settings applied successfully
     */
    bool applyPartLoadingSettings(double distance, double diameter, bool flipped);

    /**
     * @brief Calculate recommended raw material diameter from the current part
     * @return Suggested raw material diameter in mm, or 0.0 if unavailable
     */
    double getAutoRawMaterialDiameter() const;

    /**
     * @brief Process manually selected shape from 3D view and extract cylindrical axis
     * @param selectedShape The shape selected in the 3D view
     * @param clickPoint The 3D point where selection occurred
     * @return True if a valid cylindrical axis was extracted and applied
     */
    bool processManualAxisSelection(const TopoDS_Shape& selectedShape, const gp_Pnt& clickPoint);

    /**
     * @brief Reprocess the current workpiece workflow from the beginning
     * @return True if reprocessing was successful
     */
    bool reprocessCurrentWorkpiece();

    // Manager access for UI components (read-only interface)
    ChuckManager* getChuckManager() const { return m_chuckManager; }
    WorkpieceManager* getWorkpieceManager() const { return m_workpieceManager; }
    RawMaterialManager* getRawMaterialManager() const { return m_rawMaterialManager; }
    WorkspaceCoordinateManager* getCoordinateManager() const { return m_coordinateManager; }

    /**
     * @brief Check if there is a part shape loaded
     * @return True if a part shape is loaded
     */
    bool hasPartShape() const;

    /**
     * @brief Get the current part shape
     * @return The current part shape
     */
    TopoDS_Shape getPartShape() const;

    /**
     * @brief Redisplay all scene objects (chuck, workpieces, raw material). Used after a global context clear.
     */
    void redisplayAll();

    /**
     * @brief Generate toolpaths for enabled operations using the current part geometry
     * @return True if toolpath generation was successful
     */
    bool generateToolpaths();

    /**
     * @brief Extract and display the 2D profile from the current workpiece
     * @return True if profile extraction and display was successful
     */
    bool extractAndDisplayProfile();

    /**
     * @brief Update profile visibility in the 3D viewer
     * @param visible True to show profiles, false to hide
     */
    void setProfileVisible(bool visible);

    /**
     * @brief Check if profile is currently visible
     * @return True if profile is visible
     */
    bool isProfileVisible() const;

    /**
     * @brief Get the extracted profile data
     * @return The extracted profile, or empty profile if not available
     */
    IntuiCAM::Toolpath::LatheProfile::Profile2D getExtractedProfile() const;

signals:
    /**
     * @brief Emitted when chuck is successfully initialized
     */
    void chuckInitialized();

    /**
     * @brief Emitted when chuck centerline is detected
     * @param axis The detected chuck centerline axis
     */
    void chuckCenterlineDetected(const gp_Ax1& axis);

    /**
     * @brief Emitted when multiple cylinders are detected in workpiece
     * @param cylinders Vector of detected cylinder information
     */
    void multipleCylindersDetected(const QVector<CylinderInfo>& cylinders);

    /**
     * @brief Emitted when a cylinder axis is manually selected
     * @param index Index of the selected cylinder
     * @param cylinderInfo Information about the selected cylinder
     */
    void cylinderAxisSelected(int index, const CylinderInfo& cylinderInfo);

    /**
     * @brief Emitted when manual axis selection from 3D view is completed
     * @param diameter Extracted axis diameter
     * @param axis The extracted and aligned axis (now aligned with Z-axis)
     */
    void manualAxisSelected(double diameter, const gp_Ax1& axis);

    /**
     * @brief Emitted when the workpiece workflow is completed
     * @param detectedDiameter Detected workpiece diameter
     * @param rawMaterialDiameter Selected raw material diameter
     */
    void workpieceWorkflowCompleted(double detectedDiameter, double rawMaterialDiameter);

    /**
     * @brief Emitted when the workspace is cleared
     */
    void workspaceCleared();

    /**
     * @brief Emitted when an error occurs in any component
     * @param source Component that generated the error
     * @param message Error description
     */
    void errorOccurred(const QString& source, const QString& message);

    /**
     * @brief Emitted when the workpiece position is changed
     * @param distance New distance from chuck
     */
    void workpiecePositionChanged(double distance);

private slots:
    /**
     * @brief Handle errors from chuck manager
     */
    void handleChuckError(const QString& message);

    /**
     * @brief Handle errors from workpiece manager
     */
    void handleWorkpieceError(const QString& message);

    /**
     * @brief Handle errors from raw material manager
     */
    void handleRawMaterialError(const QString& message);

    /**
     * @brief Handle cylinder detection from workpiece analysis
     */
    void handleCylinderDetected(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Handle chuck centerline detection
     */
    void handleChuckCenterlineDetected(const gp_Ax1& axis);

    /**
     * @brief Handle multiple cylinders detection
     */
    void handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders);

    /**
     * @brief Handle cylinder axis selection
     */
    void handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo);

private:
    // Component managers
    ChuckManager* m_chuckManager;
    WorkpieceManager* m_workpieceManager;
    RawMaterialManager* m_rawMaterialManager;
    WorkspaceCoordinateManager* m_coordinateManager;
    
    // Dependencies
    Handle(AIS_InteractiveContext) m_context;
    IStepLoader* m_stepLoader;
    
    // State
    bool m_initialized;
    TopoDS_Shape m_currentWorkpiece; // Store original workpiece for re-processing

    // Remember last requested distance-to-chuck so that flips and reloads can reapply it
    double m_lastDistanceToChuck { 0.0 };

    // Profile management
    IntuiCAM::Toolpath::LatheProfile::Profile2D m_extractedProfile;
    Handle(AIS_InteractiveObject) m_profileDisplayObject;
    bool m_profileVisible;

    // Timer for debouncing raw material/profile updates when position changes
    QTimer* m_materialUpdateTimer { nullptr };
    
    /**
     * @brief Set up signal connections between managers
     */
    void setupManagerConnections();
    
    /**
     * @brief Coordinate the complete workpiece processing workflow
     * @param workpiece The workpiece shape to process
     */
    void executeWorkpieceWorkflow(const TopoDS_Shape& workpiece);

    /**
     * @brief Align workpiece axis with chuck centerline
     * @param workpieceAxis The original workpiece axis
     * @return The aligned axis
     */
    gp_Ax1 alignWorkpieceWithChuckCenterline(const gp_Ax1& workpieceAxis);

    /**
     * @brief Create transformation to align source axis with Z-axis
     * @param sourceAxis The axis to be aligned with Z-axis
     * @return Transformation matrix to perform the alignment
     */
    gp_Trsf createAxisAlignmentTransformation(const gp_Ax1& sourceAxis);

    /**
     * @brief Recalculate raw material for current workpiece and settings
     * @param diameter Optional diameter override, uses current if <= 0
     * @return True if recalculation was successful
     */
    bool recalculateRawMaterial(double diameter = 0.0);
    
    /**
     * @brief Initialize work coordinate system based on raw material positioning
     * @param axis The spindle axis for the coordinate system
     */
    void initializeWorkCoordinateSystem(const gp_Ax1& axis);

    /**
     * @brief Create profile display object from extracted 2D profile
     * @param profile The 2D profile to visualize
     * @return AIS object for the profile display
     */
    Handle(AIS_InteractiveObject) createProfileDisplayObject(const IntuiCAM::Toolpath::LatheProfile::Profile2D& profile);

    /**
     * @brief Update profile display when workpiece transforms
     */
    void updateProfileDisplay();

    /**
     * @brief Clear profile display from viewer
     */
    void clearProfileDisplay();
};

/**
 * @brief Manages work coordinate system transformations for lathe operations
 * 
 * This class implements a proper work coordinate system where:
 * - Origin (0,0,0) is positioned at the end of the raw material
 * - Z-axis is the spindle/rotational axis
 * - X-axis is radial (lathe X coordinate)  
 * - Toolpaths are generated in work coordinates and transformed for display
 */
class WorkspaceCoordinateManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkspaceCoordinateManager(QObject *parent = nullptr);
    
    /**
     * @brief Initialize the work coordinate system from raw material and spindle axis
     * @param rawMaterialEnd Global position of the raw material end (work origin)
     * @param spindleAxis Direction of the spindle axis (Z-axis in work coordinates)
     */
    void initializeWorkCoordinates(const IntuiCAM::Geometry::Point3D& rawMaterialEnd, 
                                   const IntuiCAM::Geometry::Vector3D& spindleAxis);
    
    /**
     * @brief Get the work coordinate system
     */
    const IntuiCAM::Geometry::WorkCoordinateSystem& getWorkCoordinateSystem() const;
    
    /**
     * @brief Convert global coordinates to work coordinates
     */
    IntuiCAM::Geometry::Point3D globalToWork(const IntuiCAM::Geometry::Point3D& globalPoint) const;
    
    /**
     * @brief Convert work coordinates to global coordinates
     */
    IntuiCAM::Geometry::Point3D workToGlobal(const IntuiCAM::Geometry::Point3D& workPoint) const;
    
    /**
     * @brief Convert global coordinates to lathe coordinates (X=radius, Z=axial)
     */
    IntuiCAM::Geometry::Point2D globalToLathe(const IntuiCAM::Geometry::Point3D& globalPoint) const;
    
    /**
     * @brief Convert lathe coordinates to global coordinates
     */
    IntuiCAM::Geometry::Point3D latheToGlobal(const IntuiCAM::Geometry::Point2D& lathePoint) const;
    
    /**
     * @brief Update work coordinate origin (e.g., when raw material position changes)
     */
    void updateWorkOrigin(const IntuiCAM::Geometry::Point3D& newOrigin);
    
    /**
     * @brief Get transformation matrix from work coordinates to global coordinates
     */
    const IntuiCAM::Geometry::Matrix4x4& getWorkToGlobalMatrix() const;
    
    /**
     * @brief Get transformation matrix from global coordinates to work coordinates  
     */
    const IntuiCAM::Geometry::Matrix4x4& getGlobalToWorkMatrix() const;
    
    /**
     * @brief Check if work coordinate system is initialized
     */
    bool isInitialized() const { return initialized_; }
    
signals:
    /**
     * @brief Emitted when work coordinate system changes
     */
    void workCoordinateSystemChanged();

private:
    IntuiCAM::Geometry::WorkCoordinateSystem workCoordinateSystem_;
    bool initialized_;
};

#endif // WORKSPACECONTROLLER_H 