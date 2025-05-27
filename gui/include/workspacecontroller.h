#ifndef WORKSPACECONTROLLER_H
#define WORKSPACECONTROLLER_H

#include <QObject>
#include <QString>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>

// Forward declarations
class ChuckManager;
class WorkpieceManager;
class RawMaterialManager;
class IStepLoader;

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
    
    /**
     * @brief Set up signal connections between managers
     */
    void setupManagerConnections();
    
    /**
     * @brief Coordinate the complete workpiece processing workflow
     * @param workpiece The workpiece shape to process
     */
    void executeWorkpieceWorkflow(const TopoDS_Shape& workpiece);
};

#endif // WORKSPACECONTROLLER_H 