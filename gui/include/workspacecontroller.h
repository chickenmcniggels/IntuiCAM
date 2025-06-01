#ifndef WORKSPACECONTROLLER_H
#define WORKSPACECONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <gp_Ax1.hxx>

// Forward declarations
class ChuckManager;
class WorkpieceManager;
class RawMaterialManager;
class IStepLoader;

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
     * @brief Get the index of the currently selected cylinder
     * @return Index of selected cylinder, or -1 if none selected
     */
    int getSelectedCylinderIndex() const;

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
     * @param cylinderIndex Selected cylinder index
     * @return True if all settings applied successfully
     */
    bool applyPartLoadingSettings(double distance, double diameter, bool flipped, int cylinderIndex);

    /**
     * @brief Reprocess the current workpiece workflow from the beginning
     * @return True if reprocessing was successful
     */
    bool reprocessCurrentWorkpiece();

    // Manager access for UI components (read-only interface)
    ChuckManager* getChuckManager() const { return m_chuckManager; }
    WorkpieceManager* getWorkpieceManager() const { return m_workpieceManager; }
    RawMaterialManager* getRawMaterialManager() const { return m_rawMaterialManager; }

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
     * @brief Emitted when workpiece workflow is completed
     * @param diameter Detected workpiece diameter
     * @param rawMaterialDiameter Selected raw material diameter
     */
    void workpieceWorkflowCompleted(double diameter, double rawMaterialDiameter);

    /**
     * @brief Emitted when workspace is cleared
     */
    void workspaceCleared();

    /**
     * @brief Emitted when an error occurs in any component
     * @param source Component that generated the error
     * @param message Error description
     */
    void errorOccurred(const QString& source, const QString& message);

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
    
    // Dependencies
    Handle(AIS_InteractiveContext) m_context;
    IStepLoader* m_stepLoader;
    
    // State
    bool m_initialized;
    TopoDS_Shape m_currentWorkpiece; // Store original workpiece for re-processing
    
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
};

#endif // WORKSPACECONTROLLER_H 