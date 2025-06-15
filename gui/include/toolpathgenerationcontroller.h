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

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>

// Include operation headers for Parameters types
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Toolpath/PartingOperation.h>

// Include GUI operation parameter dialog

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
 * @brief Controller for automatic toolpath generation
 * 
 * This controller coordinates the automatic generation of toolpaths based on:
 * - Part geometry analysis
 * - Material properties
 * - Selected operations
 * - Quality requirements
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

public slots:
    void onGenerationRequested(const GenerationRequest& request);

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

private slots:
    void performAnalysis();
    void performPlanning();
    void performGeneration();
    void performOptimization();
    void finishGeneration();
    void handleError(const QString& errorMessage);

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
    OperationParameterDialog::OperationType getOperationParameterDialogType(const QString& operationType) const;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // TOOLPATHGENERATIONCONTROLLER_H 