#ifndef TOOLPATHGENERATIONCONTROLLER_H
#define TOOLPATHGENERATIONCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QMutex>
#include <QThread>
#include <QProgressBar>
#include <QTextEdit>
#include <QMap>
#include <QVariant>
#include <QElapsedTimer>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>

// Include operation headers for Parameters types
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Toolpath/PartingOperation.h>

// Include GUI operation parameter dialog
#include "operationparameterdialog.h"

// Forward declarations
class ToolpathManager;
class ToolpathTimelineWidget;
class WorkspaceController;  // Forward declaration for the WorkspaceController
class WorkpieceManager;     // new forward declaration
class RawMaterialManager;   // new forward declaration

namespace IntuiCAM {
namespace GUI {
    class SetupConfigurationPanel;
    enum class MaterialType;
    enum class SurfaceFinish;
}
namespace Toolpath {
    class Operation;
    class Tool;
    class Toolpath;
}
namespace Geometry {
    class Part;
}
}

class QProgressBar;
class QTextEdit;

namespace IntuiCAM {
namespace GUI {

/**
 * @brief Parameter change types for incremental updates
 */
enum class ParameterChangeType {
    Geometry,      ///< Affects profile extraction - requires full regeneration
    Tool,          ///< Affects all toolpaths - requires toolpath regeneration
    Operation,     ///< Affects specific operations - requires partial regeneration
    Visual         ///< Affects only display - requires display updates only
};

/**
 * @brief Advanced toolpath generation controller with real-time parameter synchronization
 * 
 * This controller manages the complete toolpath generation pipeline including:
 * - Profile extraction from part geometry
 * - Multi-operation toolpath generation (facing, roughing, finishing)
 * - Real-time parameter synchronization and incremental updates
 * - Visual feedback and progress tracking
 * - Advanced caching and performance optimization
 */
class ToolpathGenerationController : public QObject
{
    Q_OBJECT

public:
    struct GenerationRequest {
        QString stepFilePath;
        TopoDS_Shape partShape;
        IntuiCAM::GUI::MaterialType materialType;
        double rawDiameter;
        double distanceToChuck;
        bool orientationFlipped;
        
        // Operation settings
        QStringList enabledOperations;
        double facingAllowance;
        double roughingAllowance;
        double finishingAllowance;
        double partingWidth;
        
        // Quality settings
        IntuiCAM::GUI::SurfaceFinish surfaceFinish;
        double tolerance;
        
        // Additional required fields
        std::shared_ptr<IntuiCAM::Toolpath::Tool> tool;
        double profileTolerance = 0.01;
        int profileSections = 100;
    };

    struct GenerationResult {
        bool success;
        QString errorMessage;
        QStringList generatedOperations;
        QStringList warnings;
        double estimatedMachiningTime; // minutes
        int totalToolpaths;
    };

    enum class GenerationStatus {
        Idle,
        Analyzing,
        Planning,
        Generating,
        Optimizing,
        Completed,
        Error
    };

    /**
     * @brief Parameter change information for incremental updates
     */
    struct ParameterChange {
        ParameterChangeType type;
        QString parameterName;
        QVariant oldValue;
        QVariant newValue;
        QStringList affectedOperations;
        
        ParameterChange(ParameterChangeType t, const QString& name, 
                       const QVariant& oldVal, const QVariant& newVal)
            : type(t), parameterName(name), oldValue(oldVal), newValue(newVal) {}
    };

    explicit ToolpathGenerationController(QObject *parent = nullptr);
    ~ToolpathGenerationController();

    // Initialize with 3D viewer context
    void initialize(Handle(AIS_InteractiveContext) context);

    // Set the workspace controller
    void setWorkspaceController(WorkspaceController* workspaceController);

    // Main generation interface
    void generateToolpaths(const GenerationRequest& request);
    void cancelGeneration();
    
    // Status and progress
    GenerationStatus getStatus() const { return m_status; }
    int getProgressPercentage() const { return m_progressPercentage; }
    QString getCurrentStatusMessage() const { return m_statusMessage; }
    
    // Connect UI components for feedback
    void connectProgressBar(QProgressBar* progressBar);
    void connectStatusText(QTextEdit* statusText);

    // New methods for direct toolpath handling
    void generateAndDisplayToolpath(const QString& operationName, 
                                   const QString& operationType,
                                   std::shared_ptr<IntuiCAM::Toolpath::Tool> tool);
    
    void connectTimelineWidget(ToolpathTimelineWidget* timelineWidget);
    
    std::shared_ptr<IntuiCAM::Toolpath::Tool> createDefaultTool(const QString& operationType);
    
    // New methods for operation parameter updates
    void updateOperationParameters(const QString& operationName, const QString& operationType, void* params);
    void regenerateToolpath(const QString& operationName, const QString& operationType);
    /**
     * @brief Regenerate every currently generated toolpath using updated part position.
     */
    void regenerateAllToolpaths();

    /**
     * @brief Update specific parameters with incremental regeneration
     * @param changes List of parameter changes to apply
     */
    void updateParameters(const QList<ParameterChange>& changes);

    /**
     * @brief Update a single parameter with immediate feedback
     * @param changeType Type of parameter change
     * @param parameterName Name of the parameter
     * @param newValue New parameter value
     * @param operationName Optional operation name for operation-specific parameters
     */
    void updateParameter(ParameterChangeType changeType, 
                        const QString& parameterName, 
                        const QVariant& newValue,
                        const QString& operationName = QString());

    /**
     * @brief Enable or disable real-time parameter synchronization
     * @param enabled True to enable real-time updates
     */
    void setRealTimeUpdatesEnabled(bool enabled) { m_realTimeUpdatesEnabled = enabled; }

    /**
     * @brief Check if real-time updates are enabled
     * @return True if real-time updates are enabled
     */
    bool isRealTimeUpdatesEnabled() const { return m_realTimeUpdatesEnabled; }

    /**
     * @brief Set the debounce delay for parameter changes
     * @param milliseconds Delay in milliseconds (default: 500ms)
     */
    void setParameterDebounceDelay(int milliseconds) { m_debounceTimer->setInterval(milliseconds); }

    /**
     * @brief Validate parameter value without applying changes
     * @param parameterName Name of the parameter to validate
     * @param value Value to validate
     * @return Empty string if valid, error message if invalid
     */
    QString validateParameterValue(const QString& parameterName, const QVariant& value);

    /**
     * @brief Get current cached parameter values
     * @return Map of parameter names to values
     */
    QMap<QString, QVariant> getCurrentParameters() const { return m_cachedParameters; }

public slots:
    void onGenerationRequested(const GenerationRequest& request);

    /**
     * @brief Handle parameter changes from UI components
     * @param parameterName Name of the changed parameter
     * @param newValue New parameter value
     * @param operationName Optional operation name
     */
    void onParameterChanged(const QString& parameterName, const QVariant& newValue, 
                           const QString& operationName = QString());

    /**
     * @brief Handle batch parameter changes
     * @param parameters Map of parameter names to values
     */
    void onParametersChanged(const QMap<QString, QVariant>& parameters);

signals:
    void generationStarted();
    void progressUpdated(int percentage, const QString& statusMessage);
    void operationCompleted(const QString& operationName, bool success, const QString& message);
    void generationCompleted(const GenerationResult& result);
    void generationCancelled();
    void errorOccurred(const QString& errorMessage);
    
    // New signals for toolpath handling
    void toolpathAdded(const QString& name, const QString& type, const QString& toolName);
    void toolpathSelected(const QString& name, const QString& type);
    void toolpathRemoved(const QString& name);
    void toolpathRegenerated(const QString& name, const QString& type);

    /**
     * @brief Emitted when parameter validation completes
     * @param parameterName Name of the validated parameter
     * @param isValid True if parameter is valid
     * @param errorMessage Error message if invalid (empty if valid)
     */
    void parameterValidated(const QString& parameterName, bool isValid, const QString& errorMessage);

    /**
     * @brief Emitted when incremental update completes
     * @param affectedOperations List of operations that were updated
     * @param updateDuration Time taken for the update (milliseconds)
     */
    void incrementalUpdateCompleted(const QStringList& affectedOperations, int updateDuration);

    /**
     * @brief Emitted when parameter cache is updated
     * @param parameterName Name of the updated parameter
     * @param newValue New parameter value
     */
    void parameterCacheUpdated(const QString& parameterName, const QVariant& newValue);

private slots:
    void performAnalysis();
    void performPlanning();
    void performGeneration();
    void performOptimization();
    void finishGeneration();
    void handleError(const QString& errorMessage);

    /**
     * @brief Process pending parameter changes (debounced)
     */
    void processPendingParameterChanges();

private:
    // Core generation steps
    bool analyzePartGeometry();
    bool planOperationSequence();
    bool generateOperationToolpaths();
    bool optimizeToolpaths();
    bool validateResults();
    
    // Helper methods
    QStringList determineOptimalOperationSequence();
    bool validateOperationCompatibility(const QString& operationName);
    double estimateMachiningTime(const QStringList& operations);
    void updateProgress(int percentage, const QString& message);
    void logMessage(const QString& message);
    
    // Toolpath core integration
    std::shared_ptr<IntuiCAM::Toolpath::Tool> createToolForOperation(const QString& operationName);
    std::unique_ptr<IntuiCAM::Toolpath::Operation> createOperation(const QString& operationName);
    
    // Member variables
    GenerationRequest m_currentRequest;
    GenerationResult m_currentResult;
    GenerationStatus m_status;
    int m_progressPercentage;
    QString m_statusMessage;
    
    // Threading and timing
    QTimer* m_processTimer;
    QMutex m_statusMutex;
    bool m_cancellationRequested;
    
    // UI connections
    QProgressBar* m_connectedProgressBar;
    QTextEdit* m_connectedStatusText;
    
    // Toolpath manager for visualization
    ToolpathManager* m_toolpathManager;
    
    // Static configuration
    static const QStringList DEFAULT_OPERATION_ORDER;
    static const QMap<QString, double> OPERATION_TIME_ESTIMATES; // minutes per operation
    
    // Toolpath storage
    QMap<QString, std::shared_ptr<IntuiCAM::Toolpath::Toolpath>> m_generatedToolpaths;

    // Dependencies
    Handle(AIS_InteractiveContext) m_context;
    ToolpathTimelineWidget* m_timelineWidget;
    QTextEdit* m_statusText;
    WorkspaceController* m_workspaceController;
    WorkpieceManager* m_workpieceManager {nullptr};     // new member: access to workpiece orientation
    RawMaterialManager*  m_rawMaterialManager {nullptr}; // new member: access to raw material orientation
    
    // State
    std::map<QString, std::unique_ptr<IntuiCAM::Toolpath::Toolpath>> m_toolpaths;
    std::vector<QString> m_operationOrder;
    bool m_isGenerating;
    int m_currentOperationIndex;
    int m_totalOperations;
    
    // Operation parameters storage - fixed QMap template parameters
    QMap<QString, IntuiCAM::Toolpath::RoughingOperation::Parameters> m_roughingParams;
    QMap<QString, IntuiCAM::Toolpath::FacingOperation::Parameters> m_facingParams;
    QMap<QString, IntuiCAM::Toolpath::FinishingOperation::Parameters> m_finishingParams;
    QMap<QString, IntuiCAM::Toolpath::PartingOperation::Parameters> m_partingParams;
    QMap<QString, std::shared_ptr<IntuiCAM::Toolpath::Tool>> m_operationTools;
    
    // Helper methods
    QString getOperationTypeString(const QString& operationName) const;
    
    /**
     * @brief Display a generated toolpath and apply transformations
     * @param operationName Name of the operation
     * @param toolName Name of the tool used
     * @param toolpath The generated toolpath
     */
    void displayGeneratedToolpath(
        const QString& operationName,
        const QString& toolName,
        std::unique_ptr<IntuiCAM::Toolpath::Toolpath> toolpath);
        
    // Helper method to determine parameter dialog type
    IntuiCAM::GUI::OperationParameterDialog::OperationType getOperationParameterDialogType(const QString& operationType) const;

    // Parameter synchronization members
    bool m_realTimeUpdatesEnabled;
    QTimer* m_debounceTimer;
    int m_debounceInterval = 500; // Add missing member variable
    QMap<QString, QVariant> m_cachedParameters;
    QVector<ParameterChange> m_pendingChanges;
    GenerationRequest m_cachedRequest;
    bool m_hasCachedRequest;
    QMutex m_parameterMutex;
    
    // Performance tracking
    QElapsedTimer m_updateTimer;
    QMap<QString, int> m_updateDurations;

    /**
     * @brief Determine what needs to be updated based on parameter changes
     * @param changes List of parameter changes
     * @return Update strategy information
     */
    struct UpdateStrategy {
        bool needsProfileRegeneration;
        QStringList operationsToRegenerate;
        QStringList visualOnlyUpdates;
    };
    UpdateStrategy analyzeParameterChanges(const QList<ParameterChange>& changes);

    /**
     * @brief Perform incremental toolpath regeneration
     * @param strategy Update strategy determined by analyzeParameterChanges
     */
    void performIncrementalUpdate(const UpdateStrategy& strategy);

    /**
     * @brief Update visual properties without regenerating toolpaths
     * @param visualParameters List of visual parameters to update
     */
    void updateVisualProperties(const QStringList& visualParameters);

    /**
     * @brief Cache current parameter values
     * @param request Generation request to cache
     */
    void cacheParameters(const GenerationRequest& request);

    /**
     * @brief Detect changes between cached and new parameters
     * @param newRequest New generation request
     * @return List of detected parameter changes
     */
    QList<ParameterChange> detectParameterChanges(const GenerationRequest& newRequest);

    /**
     * @brief Regenerate the contouring operation with updated parameters
     */
    void regenerateContouringOperation();
};

} // namespace GUI
} // namespace IntuiCAM

#endif // TOOLPATHGENERATIONCONTROLLER_H 