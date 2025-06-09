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

// Forward declarations
class ToolpathManager;

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

public slots:
    void onGenerationRequested(const GenerationRequest& request);

signals:
    void generationStarted();
    void progressUpdated(int percentage, const QString& statusMessage);
    void operationCompleted(const QString& operationName, bool success, const QString& message);
    void generationCompleted(const GenerationResult& result);
    void generationCancelled();
    void errorOccurred(const QString& errorMessage);

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
};

} // namespace GUI
} // namespace IntuiCAM

#endif // TOOLPATHGENERATIONCONTROLLER_H 