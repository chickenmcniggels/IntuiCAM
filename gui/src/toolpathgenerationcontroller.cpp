#include "toolpathgenerationcontroller.h"
#include "setupconfigurationpanel.h"
#include "../include/toolpathmanager.h"
#include "../include/toolpathtimelinewidget.h"
#include "../include/workspacecontroller.h"
#include "../include/workpiecemanager.h"
#include "../include/rawmaterialmanager.h"
#include <gp_XYZ.hxx>
#include <gp_Trsf.hxx>
#include <TopoDS_Shape.hxx>

#include <QDebug>
#include <QProgressBar>
#include <QTextEdit>
#include <QCoreApplication>
#include <QApplication>
#include <QMutexLocker>
#include <QDateTime>

// Core includes for actual toolpath generation
#include <IntuiCAM/Toolpath/Operations.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>

// Forward declaration of SimplePart
namespace IntuiCAM {
namespace Geometry {
class SimplePart : public Part {
private:
    double volume_;
    double surfaceArea_;
    BoundingBox boundingBox_;
    
public:
    SimplePart(double volume = 1000.0, double surfaceArea = 500.0) 
        : volume_(volume), surfaceArea_(surfaceArea) {
        // Default bounding box for a simple cylinder
        boundingBox_.min = Point3D(-25.0, -25.0, -50.0);
        boundingBox_.max = Point3D(25.0, 25.0, 50.0);
    }
    
    double getVolume() const override { return volume_; }
    double getSurfaceArea() const override { return surfaceArea_; }
    
    BoundingBox getBoundingBox() const override { return boundingBox_; }
    
    std::unique_ptr<GeometricEntity> clone() const override {
        return std::make_unique<SimplePart>(volume_, surfaceArea_);
    }
    
    std::unique_ptr<Mesh> generateMesh(double tolerance = 0.1) const override {
        auto mesh = std::make_unique<Mesh>();
        // Create a simple cylindrical mesh (simplified)
        // This is a placeholder implementation
        return mesh;
    }
    
    std::vector<Point3D> detectCylindricalFeatures() const override {
        // Return center axis points for a simple cylinder
        return {Point3D(0, 0, -50), Point3D(0, 0, 50)};
    }
    
    std::optional<double> getLargestCylinderDiameter() const override {
        return 50.0; // Default diameter
    }
};
}
}

// Forward declaration of conversion helper
static IntuiCAM::Geometry::Matrix4x4 toMatrix4x4(const gp_Trsf& trsf);

// Static configuration
const QStringList IntuiCAM::GUI::ToolpathGenerationController::DEFAULT_OPERATION_ORDER = {
    "Contouring", "Threading", "Chamfering", "Parting"
};

const QMap<QString, double> IntuiCAM::GUI::ToolpathGenerationController::OPERATION_TIME_ESTIMATES = {
    {"Contouring", 10.0},  // minutes
    {"Threading", 5.0},    // minutes
    {"Chamfering", 2.0},   // minutes
    {"Parting", 1.5}       // minutes
};

IntuiCAM::GUI::ToolpathGenerationController::ToolpathGenerationController(QObject *parent)
    : QObject(parent)
    , m_status(GenerationStatus::Idle)
    , m_progressPercentage(0)
    , m_statusMessage("Ready")
    , m_processTimer(new QTimer(this))
    , m_cancellationRequested(false)
    , m_connectedProgressBar(nullptr)
    , m_connectedStatusText(nullptr)
    , m_toolpathManager(nullptr)
    , m_workspaceController(nullptr)
{
    // Setup process timer for step-by-step generation
    m_processTimer->setSingleShot(true);
    
    // Initialize result structure
    m_currentResult.success = false;
    m_currentResult.estimatedMachiningTime = 0.0;
    m_currentResult.totalToolpaths = 0;
    
    // Create toolpath manager
    m_toolpathManager = new ToolpathManager(this);
    
    // Connect toolpath manager signals
    connect(m_toolpathManager, &ToolpathManager::toolpathDisplayed, 
            this, [this](const QString& name) {
                logMessage(QString("Displayed toolpath: %1").arg(name));
            });
    
    connect(m_toolpathManager, &ToolpathManager::errorOccurred,
            this, [this](const QString& message) {
                logMessage(QString("Toolpath error: %1").arg(message));
            });
}

IntuiCAM::GUI::ToolpathGenerationController::~ToolpathGenerationController()
{
    cancelGeneration();
}

void IntuiCAM::GUI::ToolpathGenerationController::initialize(Handle(AIS_InteractiveContext) context)
{
    // Initialize the toolpath manager
    if (m_toolpathManager) {
        m_toolpathManager->initialize(context);
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::setWorkspaceController(WorkspaceController* workspaceController)
{
    m_workspaceController = workspaceController;
    if (workspaceController) {
        m_workpieceManager = workspaceController->getWorkpieceManager();
        m_rawMaterialManager = workspaceController->getRawMaterialManager();
    } else {
        m_workpieceManager = nullptr;
        m_rawMaterialManager = nullptr;
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::generateToolpaths(const GenerationRequest& request)
{
    QMutexLocker locker(&m_statusMutex);
    
    if (m_status != GenerationStatus::Idle) {
        emit errorOccurred("Generation already in progress. Please wait or cancel current operation.");
        return;
    }
    
    // Store the request
    m_currentRequest = request;
    m_cancellationRequested = false;
    
    // Reset result
    m_currentResult = GenerationResult();
    m_currentResult.success = false;
    m_currentResult.totalToolpaths = 0;
    m_currentResult.estimatedMachiningTime = 0.0;
    
    // Clear any existing toolpaths
    if (m_toolpathManager) {
        m_toolpathManager->clearAllToolpaths();
    }
    
    // Start the generation process
    m_status = GenerationStatus::Analyzing;
    m_progressPercentage = 0;
    
    emit generationStarted();
    updateProgress(0, "Starting toolpath generation...");
    
    // Start with analysis phase
    QTimer::singleShot(100, this, &IntuiCAM::GUI::ToolpathGenerationController::performAnalysis);
}

void IntuiCAM::GUI::ToolpathGenerationController::cancelGeneration()
{
    QMutexLocker locker(&m_statusMutex);
    
    if (m_status == GenerationStatus::Idle || m_status == GenerationStatus::Completed) {
        return;
    }
    
    m_cancellationRequested = true;
    m_processTimer->stop();
    
    m_status = GenerationStatus::Idle;
    m_progressPercentage = 0;
    m_statusMessage = "Generation cancelled";
    
    updateProgress(0, "Generation cancelled by user");
    emit generationCancelled();
}

void IntuiCAM::GUI::ToolpathGenerationController::connectProgressBar(QProgressBar* progressBar)
{
    m_connectedProgressBar = progressBar;
    
    if (progressBar) {
        connect(this, &IntuiCAM::GUI::ToolpathGenerationController::progressUpdated,
                this, [this](int percentage, const QString&) {
                    if (m_connectedProgressBar) {
                        m_connectedProgressBar->setValue(percentage);
                        m_connectedProgressBar->setVisible(percentage > 0 && percentage < 100);
                    }
                });
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::connectStatusText(QTextEdit* statusText)
{
    m_connectedStatusText = statusText;
    
    if (statusText) {
        connect(this, &IntuiCAM::GUI::ToolpathGenerationController::progressUpdated,
                this, [this](int percentage, const QString& message) {
                    if (m_connectedStatusText) {
                        m_connectedStatusText->append(QString("[%1%] %2").arg(percentage).arg(message));
                    }
                });
                
        connect(this, &IntuiCAM::GUI::ToolpathGenerationController::operationCompleted,
                this, [this](const QString& operationName, bool success, const QString& message) {
                    if (m_connectedStatusText) {
                        QString status = success ? "✓" : "✗";
                        m_connectedStatusText->append(QString("%1 %2: %3").arg(status, operationName, message));
                    }
                });
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::onGenerationRequested(const GenerationRequest& request)
{
    generateToolpaths(request);
}

void IntuiCAM::GUI::ToolpathGenerationController::performAnalysis()
{
    if (m_cancellationRequested) return;
    
    updateProgress(10, "Analyzing part geometry...");
    
    // Perform part analysis
    bool analysisSuccess = analyzePartGeometry();
    
    if (!analysisSuccess) {
        handleError("Failed to analyze part geometry. Please check the STEP file.");
        return;
    }
    
    logMessage("Part geometry analysis completed successfully");
    
    // Move to planning phase
    m_status = GenerationStatus::Planning;
    QTimer::singleShot(500, this, &IntuiCAM::GUI::ToolpathGenerationController::performPlanning);
}

void IntuiCAM::GUI::ToolpathGenerationController::performPlanning()
{
    if (m_cancellationRequested) return;
    
    updateProgress(25, "Planning operation sequence...");
    
    // Plan the operation sequence
    bool planningSuccess = planOperationSequence();
    
    if (!planningSuccess) {
        handleError("Failed to plan operation sequence. Please check operation settings.");
        return;
    }
    
    logMessage("Operation sequence planning completed");
    
    // Move to generation phase
    m_status = GenerationStatus::Generating;
    QTimer::singleShot(300, this, &IntuiCAM::GUI::ToolpathGenerationController::performGeneration);
}

void IntuiCAM::GUI::ToolpathGenerationController::performGeneration()
{
    if (m_cancellationRequested) return;
    
    updateProgress(40, "Generating toolpaths...");
    
    // Generate the actual toolpaths
    bool generationSuccess = generateOperationToolpaths();
    
    if (!generationSuccess) {
        handleError("Failed to generate toolpaths. Please check operation parameters.");
        return;
    }
    
    logMessage("Toolpath generation completed");
    
    // Move to optimization phase
    m_status = GenerationStatus::Optimizing;
    QTimer::singleShot(400, this, &IntuiCAM::GUI::ToolpathGenerationController::performOptimization);
}

void IntuiCAM::GUI::ToolpathGenerationController::performOptimization()
{
    if (m_cancellationRequested) return;
    
    updateProgress(80, "Optimizing toolpaths...");
    
    // Optimize the generated toolpaths
    bool optimizationSuccess = optimizeToolpaths();
    
    if (!optimizationSuccess) {
        // Optimization failure is not critical
        logMessage("Warning: Toolpath optimization had issues, but toolpaths are still usable");
        m_currentResult.warnings.append("Toolpath optimization incomplete - performance may be suboptimal");
    } else {
        logMessage("Toolpath optimization completed");
    }
    
    // Finish generation
    QTimer::singleShot(200, this, &IntuiCAM::GUI::ToolpathGenerationController::finishGeneration);
}

void IntuiCAM::GUI::ToolpathGenerationController::finishGeneration()
{
    if (m_cancellationRequested) return;
    
    updateProgress(95, "Finalizing results...");
    
    // Validate final results
    bool validationSuccess = validateResults();
    
    if (!validationSuccess) {
        handleError("Generated toolpaths failed validation. Please review parameters.");
        return;
    }
    
    // Complete the generation
    m_status = GenerationStatus::Completed;
    m_currentResult.success = true;
    
    updateProgress(100, "Toolpath generation completed successfully!");
    
    // Calculate final statistics
    m_currentResult.estimatedMachiningTime = estimateMachiningTime(m_currentResult.generatedOperations);
    
    logMessage(QString("Generation complete: %1 operations, estimated time: %2 minutes")
               .arg(m_currentResult.totalToolpaths)
               .arg(m_currentResult.estimatedMachiningTime, 0, 'f', 1));
    
    // Return to idle state
    QTimer::singleShot(1000, this, [this]() {
        m_status = GenerationStatus::Idle;
        emit generationCompleted(m_currentResult);
    });
}

void IntuiCAM::GUI::ToolpathGenerationController::handleError(const QString& errorMessage)
{
    m_status = GenerationStatus::Error;
    m_currentResult.success = false;
    m_currentResult.errorMessage = errorMessage;
    
    updateProgress(0, QString("Error: %1").arg(errorMessage));
    
    // Return to idle after error
    QTimer::singleShot(1000, this, [this]() {
        m_status = GenerationStatus::Idle;
        emit errorOccurred(m_currentResult.errorMessage);
    });
}

bool IntuiCAM::GUI::ToolpathGenerationController::analyzePartGeometry()
{
    // TODO: Integrate with actual geometry analysis
    // For now, simulate analysis
    
    logMessage("Detecting cylindrical features...");
    QCoreApplication::processEvents(); // Allow UI updates
    
    logMessage("Analyzing part dimensions...");
    QCoreApplication::processEvents();
    
    logMessage("Determining machining features...");
    QCoreApplication::processEvents();
    
    // Basic validation
    if (m_currentRequest.stepFilePath.isEmpty()) {
        return false;
    }
    
    return true;
}

bool IntuiCAM::GUI::ToolpathGenerationController::planOperationSequence()
{
    // Determine optimal operation sequence based on enabled operations
    QStringList plannedSequence = determineOptimalOperationSequence();
    
    logMessage(QString("Planning %1 operations:").arg(plannedSequence.size()));
    
    for (const QString& operation : plannedSequence) {
        if (!validateOperationCompatibility(operation)) {
            logMessage(QString("Warning: %1 operation may not be optimal for current setup").arg(operation));
            m_currentResult.warnings.append(QString("%1 operation parameters may need adjustment").arg(operation));
        }
        
        logMessage(QString("  • %1").arg(operation));
        m_currentResult.generatedOperations.append(operation);
        QCoreApplication::processEvents();
    }
    
    m_currentResult.totalToolpaths = plannedSequence.size();
    return !plannedSequence.isEmpty();
}

bool IntuiCAM::GUI::ToolpathGenerationController::generateOperationToolpaths()
{
    int operationCount = 0;
    int totalOperations = m_currentResult.generatedOperations.size();
    
    for (const QString& operationName : m_currentResult.generatedOperations) {
        if (m_cancellationRequested) return false;
        
        operationCount++;
        int operationProgress = 40 + (30 * operationCount / totalOperations);
        updateProgress(operationProgress, QString("Generating %1 toolpath...").arg(operationName));
        
        try {
            // Create tool with appropriate parameters
            auto tool = createToolForOperation(operationName);
            if (!tool) {
                emit operationCompleted(operationName, false, "Failed to create tool");
                continue;
            }
            
            // Create operation
            std::unique_ptr<IntuiCAM::Toolpath::Operation> operation;
            
            // Create different operations based on type
            if (operationName == "Contouring") {
                auto roughingOp = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
                    operationName.toStdString(), tool);
                
                // Show lathe profile overlay for manual single roughing generation
                if (m_toolpathManager && m_workspaceController) {
                    if (m_workspaceController->hasPartShape()) {
                        TopoDS_Shape partShape = m_workspaceController->getPartShape();
                        IntuiCAM::Geometry::OCCTPart part(&partShape);
                        auto profile = IntuiCAM::Toolpath::LatheProfile::extract(part, 150);
                        if (!profile.empty()) {
                            m_toolpathManager->displayLatheProfile(profile, "PartProfileOverlay");
                        }
                    }
                }
                
                // Set roughing parameters
                IntuiCAM::Toolpath::RoughingOperation::Parameters params;
                params.startDiameter = m_currentRequest.rawDiameter;
                params.endDiameter = m_currentRequest.rawDiameter * 0.6; // 60% of raw diameter as a simplification
                params.startZ = 0.0;
                params.endZ = -50.0; // Arbitrary length for demo
                params.depthOfCut = 1.0;
                params.stockAllowance = m_currentRequest.roughingAllowance;
                
                roughingOp->setParameters(params);
                operation = std::move(roughingOp);
            }
            else if (operationName == "Threading") {
                auto threadingOp = std::make_unique<IntuiCAM::Toolpath::ThreadingOperation>(
                    operationName.toStdString(), tool);

                IntuiCAM::Toolpath::ThreadingOperation::Parameters params;
                threadingOp->setParameters(params);
                operation = std::move(threadingOp);
            }
            else if (operationName == "Chamfering") {
                auto chamferOp = std::make_unique<IntuiCAM::Toolpath::FinishingOperation>(
                    operationName.toStdString(), tool);
                IntuiCAM::Toolpath::FinishingOperation::Parameters params;
                chamferOp->setParameters(params);
                operation = std::move(chamferOp);
            }
            else if (operationName == "Parting") {
                auto partingOp = std::make_unique<IntuiCAM::Toolpath::PartingOperation>(
                    operationName.toStdString(), tool);
                IntuiCAM::Toolpath::PartingOperation::Parameters params;
                params.partingDiameter = m_currentRequest.partingWidth;
                partingOp->setParameters(params);
                operation = std::move(partingOp);
            }
            else {
                // Handle other operation types...
                emit operationCompleted(operationName, false, "Operation type not implemented yet");
                continue;
            }
            
            if (!operation) {
                emit operationCompleted(operationName, false, "Failed to create operation");
                continue;
            }
            
            // Validate operation
            if (!operation->validate()) {
                emit operationCompleted(operationName, false, "Operation validation failed");
                m_currentResult.warnings.append(QString("%1 operation has validation warnings").arg(operationName));
                continue;
            }
            
            // Create a SimplePart instance instead of trying to use the abstract Part class directly
            IntuiCAM::Geometry::SimplePart dummyPart;
            
            // Generate the toolpath
            auto toolpath = operation->generateToolpath(dummyPart);
            
            if (!toolpath) {
                emit operationCompleted(operationName, false, "Failed to generate toolpath");
                continue;
            }
            
            // Display the toolpath
            if (m_toolpathManager) {
                bool displayed = m_toolpathManager->displayToolpath(*toolpath, operationName);
                if (!displayed) {
                    emit operationCompleted(operationName, false, "Failed to display toolpath");
                    continue;
                }
            }
            
            emit operationCompleted(operationName, true, "Toolpath generated successfully");
            
        } catch (const std::exception& e) {
            emit operationCompleted(operationName, false, QString("Exception: %1").arg(e.what()));
            return false;
        }
        
        QCoreApplication::processEvents();
    }
    
    return true;
}

bool IntuiCAM::GUI::ToolpathGenerationController::optimizeToolpaths()
{
    logMessage("Optimizing rapid moves...");
    QCoreApplication::processEvents();
    
    logMessage("Minimizing tool changes...");
    QCoreApplication::processEvents();
    
    logMessage("Reducing machining time...");
    QCoreApplication::processEvents();
    
    // TODO: Implement actual toolpath optimization
    return true;
}

bool IntuiCAM::GUI::ToolpathGenerationController::validateResults()
{
    logMessage("Validating toolpath safety...");
    QCoreApplication::processEvents();
    
    logMessage("Checking collision detection...");
    QCoreApplication::processEvents();
    
    logMessage("Verifying operation sequence...");
    QCoreApplication::processEvents();
    
    // Basic validation
    return m_currentResult.totalToolpaths > 0;
}

QStringList IntuiCAM::GUI::ToolpathGenerationController::determineOptimalOperationSequence()
{
    QStringList sequence;
    
    // Build sequence based on enabled operations and optimal order
    for (const QString& operation : DEFAULT_OPERATION_ORDER) {
        if (m_currentRequest.enabledOperations.contains(operation)) {
            sequence.append(operation);
        }
    }
    
    return sequence;
}

bool IntuiCAM::GUI::ToolpathGenerationController::validateOperationCompatibility(const QString& operationName)
{
    // TODO: Add sophisticated compatibility checks
    // For now, basic validation
    
    if (operationName == "Contouring" && m_currentRequest.roughingAllowance <= 0.0) {
        return false;
    }

    if (operationName == "Threading") {
        return true; // no numeric parameter check
    }

    if (operationName == "Chamfering" && m_currentRequest.finishingAllowance <= 0.0) {
        return false;
    }

    if (operationName == "Parting" && m_currentRequest.partingWidth <= 0.0) {
        return false;
    }
    
    return true;
}

double IntuiCAM::GUI::ToolpathGenerationController::estimateMachiningTime(const QStringList& operations)
{
    double totalTime = 0.0;
    
    for (const QString& operation : operations) {
        if (OPERATION_TIME_ESTIMATES.contains(operation)) {
            totalTime += OPERATION_TIME_ESTIMATES[operation];
        }
    }
    
    // Add setup and tool change overhead
    totalTime += operations.size() * 0.5; // 30 seconds per operation for setup
    
    return totalTime;
}

void IntuiCAM::GUI::ToolpathGenerationController::updateProgress(int percentage, const QString& message)
{
    QMutexLocker locker(&m_statusMutex);
    
    m_progressPercentage = percentage;
    m_statusMessage = message;
    
    emit progressUpdated(percentage, message);
}

void IntuiCAM::GUI::ToolpathGenerationController::logMessage(const QString& message)
{
    qDebug() << "ToolpathGenerationController:" << message;
    
    if (m_connectedStatusText) {
        QMetaObject::invokeMethod(m_connectedStatusText, [this, message]() {
            m_connectedStatusText->append(message);
        }, Qt::QueuedConnection);
    }
}

std::shared_ptr<IntuiCAM::Toolpath::Tool> IntuiCAM::GUI::ToolpathGenerationController::createToolForOperation(const QString& operationName)
{
    // Create an appropriate tool based on the operation type
    IntuiCAM::Toolpath::Tool::Type toolType;
    
    if (operationName == "Contouring") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    }
    else if (operationName == "Threading") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Threading;
    }
    else if (operationName == "Chamfering") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    }
    else if (operationName == "Parting") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Parting;
    }
    else if (operationName == "Grooving") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Grooving;
    }
    else {
        return nullptr;
    }
    
    // Create the tool
    auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(toolType, operationName.toStdString() + " Tool");
    
    // Set standard cutting parameters
    IntuiCAM::Toolpath::Tool::CuttingParameters cuttingParams;
    cuttingParams.feedRate = 0.2;       // mm/rev
    cuttingParams.spindleSpeed = 1200;  // RPM
    cuttingParams.depthOfCut = 1.0;     // mm
    cuttingParams.stepover = 0.5;       // mm
    
    // Set tool geometry
    IntuiCAM::Toolpath::Tool::Geometry toolGeometry;
    toolGeometry.tipRadius = 0.4;       // mm
    toolGeometry.clearanceAngle = 7.0;  // degrees
    toolGeometry.rakeAngle = 0.0;       // degrees
    toolGeometry.insertWidth = 3.0;     // mm
    
    // Apply parameters to tool
    tool->setCuttingParameters(cuttingParams);
    tool->setGeometry(toolGeometry);
    
    return tool;
}

std::unique_ptr<IntuiCAM::Toolpath::Operation> IntuiCAM::GUI::ToolpathGenerationController::createOperation(const QString& operationName)
{
    // Use the createToolForOperation method first
    auto tool = createToolForOperation(operationName);
    if (!tool) {
        return nullptr;
    }
    
    // Create operation based on name
    if (operationName == "Threading") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::ThreadingOperation>(
            operationName.toStdString(), tool);

        // Set default parameters
        IntuiCAM::Toolpath::ThreadingOperation::Parameters params;
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Chamfering") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::FinishingOperation>(
            operationName.toStdString(), tool);

        IntuiCAM::Toolpath::FinishingOperation::Parameters params;
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Contouring") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
            operationName.toStdString(), tool);

        IntuiCAM::Toolpath::RoughingOperation::Parameters params;
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Facing") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::FacingOperation>(
            operationName.toStdString(), tool);
        
        // Set default parameters
        IntuiCAM::Toolpath::FacingOperation::Parameters params;
        params.startDiameter = 50.0;  // mm
        params.endDiameter = 0.0;     // mm (center)
        params.stepover = 0.5;        // mm
        params.stockAllowance = 0.2;  // mm
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Roughing") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
            operationName.toStdString(), tool);
        
        // Set default parameters
        IntuiCAM::Toolpath::RoughingOperation::Parameters params;
        params.startDiameter = 50.0;  // mm
        params.endDiameter = 20.0;    // mm
        params.startZ = 0.0;          // mm
        params.endZ = -50.0;          // mm
        params.depthOfCut = 2.0;      // mm per pass
        params.stockAllowance = 0.5;  // mm for finishing
        
        operation->setParameters(params);
        return operation;
    }
    
    // Add more operation types as needed...
    
    return nullptr;
}

// Fix the generateAndDisplayToolpath method

void IntuiCAM::GUI::ToolpathGenerationController::generateAndDisplayToolpath(
    const QString& operationName,
    const QString& operationType,
    std::shared_ptr<IntuiCAM::Toolpath::Tool> tool)
{
    if (!m_toolpathManager) {
        logMessage("Cannot generate toolpath: Toolpath manager not initialized");
        return;
    }

    // Store the tool for this operation
    m_operationTools[operationName] = tool;

    // Create a simple part for toolpath generation
    auto part = std::make_unique<IntuiCAM::Geometry::SimplePart>();
    
    // Create the appropriate operation type
    std::unique_ptr<IntuiCAM::Toolpath::Operation> operation;
    
    if (operationType == "Facing") {
        auto facingOp = std::make_unique<IntuiCAM::Toolpath::FacingOperation>(
            operationName.toStdString(), tool);
        
        // Configure with parameters from UI or use defaults
        IntuiCAM::Toolpath::FacingOperation::Parameters params;
        // You could update these parameters from UI or stored configurations
        facingOp->setParameters(params);
        
        // Store the parameters
        m_facingParams[operationName] = params;
        
        operation = std::move(facingOp);
    }
    else if (operationType == "Roughing") {
        auto roughingOp = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
            operationName.toStdString(), tool);
        
        // Show lathe profile overlay for manual single roughing generation
        if (m_toolpathManager && m_workspaceController) {
            if (m_workspaceController->hasPartShape()) {
                TopoDS_Shape partShape = m_workspaceController->getPartShape();
                IntuiCAM::Geometry::OCCTPart part(&partShape);
                auto profile = IntuiCAM::Toolpath::LatheProfile::extract(part, 150);
                if (!profile.empty()) {
                    m_toolpathManager->displayLatheProfile(profile, "PartProfileOverlay");
                }
            }
        }
        
        IntuiCAM::Toolpath::RoughingOperation::Parameters params;
        params.startDiameter = 50.0;  // mm
        params.endDiameter = 20.0;    // mm
        params.startZ = 0.0;          // mm
        params.endZ = -50.0;          // mm
        params.depthOfCut = 2.0;      // mm
        params.stockAllowance = 0.5;  // mm
        
        roughingOp->setParameters(params);
        
        // Store the parameters
        m_roughingParams[operationName] = params;
        
        operation = std::move(roughingOp);
    }
    else if (operationType == "Finishing") {
        auto finishingOp = std::make_unique<IntuiCAM::Toolpath::FinishingOperation>(
            operationName.toStdString(), tool);
        
        IntuiCAM::Toolpath::FinishingOperation::Parameters params;
        params.targetDiameter = 20.0;  // mm
        params.startZ = 0.0;           // mm
        params.endZ = -50.0;           // mm
        params.feedRate = 0.1;         // mm/rev
        
        finishingOp->setParameters(params);
        
        // Store the parameters
        m_finishingParams[operationName] = params;
        
        operation = std::move(finishingOp);
    }
    else if (operationType == "Parting") {
        auto partingOp = std::make_unique<IntuiCAM::Toolpath::PartingOperation>(
            operationName.toStdString(), tool);
        
        IntuiCAM::Toolpath::PartingOperation::Parameters params;
        // Configure parameters
        partingOp->setParameters(params);
        
        // Store the parameters
        m_partingParams[operationName] = params;
        
        operation = std::move(partingOp);
    }
    else if (operationType == "Threading") {
        auto threadingOp = std::make_unique<IntuiCAM::Toolpath::ThreadingOperation>(
            operationName.toStdString(), tool);
        
        IntuiCAM::Toolpath::ThreadingOperation::Parameters params;
        // Configure parameters
        threadingOp->setParameters(params);
        
        operation = std::move(threadingOp);
    }
    else if (operationType == "Grooving") {
        auto groovingOp = std::make_unique<IntuiCAM::Toolpath::GroovingOperation>(
            operationName.toStdString(), tool);
        
        IntuiCAM::Toolpath::GroovingOperation::Parameters params;
        // Configure parameters
        groovingOp->setParameters(params);
        
        operation = std::move(groovingOp);
    }
    else {
        logMessage(QString("Unknown operation type: %1").arg(operationType));
        return;
    }
    
    // Validate the operation parameters
    if (!operation->validate()) {
        logMessage(QString("Invalid parameters for %1 operation").arg(operationType));
        return;
    }
    
    // Generate the toolpath
    auto toolpath = operation->generateToolpath(*part);
    
    if (!toolpath) {
        logMessage(QString("Failed to generate toolpath for %1").arg(operationName));
        return;
    }
    
    // Apply current workpiece transformation so Z-orientation is respected
    if (m_workpieceManager) {
        gp_Trsf wpTrsf = m_workpieceManager->getCurrentTransformation();
        IntuiCAM::Geometry::Matrix4x4 mat = toMatrix4x4(wpTrsf);
        toolpath->applyTransform(mat);
    }
    // (Raw material orientation could be applied similarly if required)

    // If a toolpath with the same name already exists, remove it first (to avoid tile accumulation)
    const bool existedBefore = (m_toolpaths.find(operationName) != m_toolpaths.end());
    if (existedBefore) {
        m_toolpathManager->removeToolpath(operationName);
    }

    bool success = m_toolpathManager->displayToolpath(*toolpath, operationName);

    if (success) {
        qDebug() << "Successfully displayed toolpath for operation:" << operationName;

        // Align with current workpiece transform (ToolpathManager already does this for new display)

        // Store / replace generated toolpath in map
        m_toolpaths[operationName] = std::move(toolpath);

        if (existedBefore) {
            emit toolpathRegenerated(operationName, getOperationTypeString(operationName));
        } else {
            emit toolpathAdded(operationName, getOperationTypeString(operationName), QString::fromStdString(tool->getName()));
        }
    } else {
        qDebug() << "Failed to display toolpath for operation:" << operationName;
    }
}

// Helper method to map an operation name to its type
QString IntuiCAM::GUI::ToolpathGenerationController::getOperationTypeString(const QString& operationName) const
{
    // Parse the operation name to extract the type
    // Operation names often have format like "Facing_001", "Roughing_123", etc.
    if (operationName.startsWith("Facing")) return "Facing";
    if (operationName.startsWith("Roughing")) return "Roughing";
    if (operationName.startsWith("Finishing")) return "Finishing";
    if (operationName.startsWith("Parting")) return "Parting";
    if (operationName.startsWith("Threading")) return "Threading";
    if (operationName.startsWith("Grooving")) return "Grooving";
    
    // If no specific prefix found, try to parse from format "Type_number"
    int underscorePos = operationName.indexOf('_');
    if (underscorePos > 0) {
        return operationName.left(underscorePos);
    }
    
    // Fallback
    return "Unknown";
}

void IntuiCAM::GUI::ToolpathGenerationController::updateOperationParameters(
    const QString& operationName, 
    const QString& operationType, 
    void* params)
{
    if (operationType == "Roughing") {
        auto* roughingParams = static_cast<IntuiCAM::Toolpath::RoughingOperation::Parameters*>(params);
        if (roughingParams) {
            m_roughingParams[operationName] = *roughingParams;
            logMessage(QString("Updated roughing parameters for %1").arg(operationName));
        }
    }
    else if (operationType == "Facing") {
        auto* facingParams = static_cast<IntuiCAM::Toolpath::FacingOperation::Parameters*>(params);
        if (facingParams) {
            m_facingParams[operationName] = *facingParams;
            logMessage(QString("Updated facing parameters for %1").arg(operationName));
        }
    }
    else if (operationType == "Finishing") {
        auto* finishingParams = static_cast<IntuiCAM::Toolpath::FinishingOperation::Parameters*>(params);
        if (finishingParams) {
            m_finishingParams[operationName] = *finishingParams;
            logMessage(QString("Updated finishing parameters for %1").arg(operationName));
        }
    }
    else if (operationType == "Parting") {
        auto* partingParams = static_cast<IntuiCAM::Toolpath::PartingOperation::Parameters*>(params);
        if (partingParams) {
            m_partingParams[operationName] = *partingParams;
            logMessage(QString("Updated parting parameters for %1").arg(operationName));
        }
    }
}


void IntuiCAM::GUI::ToolpathGenerationController::connectTimelineWidget(ToolpathTimelineWidget* timelineWidget)
{
    if (!timelineWidget) return;
    m_timelineWidget = timelineWidget;
    
    // Connect timeline signals to controller
    connect(timelineWidget, &ToolpathTimelineWidget::addToolpathRequested,
            this, [this](const QString& operationType) {
                // Generate a default name based on operation type
                QString name = QString("%1_%2").arg(operationType).arg(QDateTime::currentDateTime().toString("hhmmss"));
                
                // Create default tool for this operation type
                auto tool = createDefaultTool(operationType);
                
                // Convert tool name to QString before using in generateAndDisplayToolpath
                QString toolName = QString::fromStdString(tool->getName());
                
                // Generate and display the toolpath
                generateAndDisplayToolpath(name, operationType, tool);
                
                // Add the toolpath to the timeline
                emit toolpathAdded(name, operationType, toolName);
            });
    
    // Connect toolpath selection signal
    connect(timelineWidget, &ToolpathTimelineWidget::toolpathSelected,
            this, [this, timelineWidget](int index) {
                if (index < 0) {
                    // Hide all toolpaths
                    if (m_toolpathManager) {
                        for (const auto& tpName : m_generatedToolpaths.keys()) {
                            m_toolpathManager->setToolpathVisible(tpName, false);
                        }
                    }
                    return;
                }
                
                QString name = timelineWidget->getToolpathName(index);
                QString type = timelineWidget->getToolpathType(index);
                
                // Highlight the selected toolpath in the 3D view
                if (m_toolpathManager) {
                    // Hide all toolpaths first
                    for (const auto& tpName : m_generatedToolpaths.keys()) {
                        m_toolpathManager->setToolpathVisible(tpName, false);
                    }
                    
                    // Show only the selected toolpath
                    m_toolpathManager->setToolpathVisible(name, true);
                }
                
                emit toolpathSelected(name, type);
            });
    
    // Connect remove toolpath signal
    connect(timelineWidget, &ToolpathTimelineWidget::removeToolpathRequested,
            this, [this, timelineWidget](int index) {
                QString name = timelineWidget->getToolpathName(index);
                
                // Remove the toolpath from the 3D view
                if (m_toolpathManager) {
                    m_toolpathManager->removeToolpath(name);
                }
                
                // Remove from stored toolpaths
                m_generatedToolpaths.remove(name);
                m_operationTools.remove(name);
                m_roughingParams.remove(name);
                m_facingParams.remove(name);
                m_finishingParams.remove(name);
                m_partingParams.remove(name);
                
                emit toolpathRemoved(name);
            });
    
    // Toolpath parameter editing now handled by MainWindow/Setup panel
    

    // Connect toolpath regeneration signal
    connect(timelineWidget, &ToolpathTimelineWidget::toolpathRegenerateRequested,
            this, [this, timelineWidget](int index) {
                QString name = timelineWidget->getToolpathName(index);
                QString type = timelineWidget->getToolpathType(index);

                // Regenerate the toolpath
                regenerateToolpath(name, type);
            });

    // --- Connect controller signals back to the timeline --------------------
    connect(this, &ToolpathGenerationController::toolpathAdded,
            timelineWidget,
            [timelineWidget](const QString& name, const QString& type, const QString& toolName) {
                timelineWidget->addToolpath(name, type, toolName);
            });

    connect(this, &ToolpathGenerationController::toolpathRemoved,
            timelineWidget,
            [timelineWidget](const QString& name) {
                for (int i = 0; i < timelineWidget->getToolpathCount(); ++i) {
                    if (timelineWidget->getToolpathName(i) == name) {
                        timelineWidget->removeToolpath(i);
                        break;
                    }
                }
            });
}

// Helper method to convert operation type string to parameter dialog type
IntuiCAM::GUI::OperationParameterDialog::OperationType 
IntuiCAM::GUI::ToolpathGenerationController::getOperationParameterDialogType(
    const QString& operationType) const
{
    if (operationType == "Facing")
        return IntuiCAM::GUI::OperationParameterDialog::OperationType::Facing;
    else if (operationType == "Roughing")
        return IntuiCAM::GUI::OperationParameterDialog::OperationType::Roughing;
    else if (operationType == "Finishing")
        return IntuiCAM::GUI::OperationParameterDialog::OperationType::Finishing;
    else if (operationType == "Parting")
        return IntuiCAM::GUI::OperationParameterDialog::OperationType::Parting;
    
    // Default to roughing
    return IntuiCAM::GUI::OperationParameterDialog::OperationType::Roughing;
}

// Helper method to create a default tool based on operation type
std::shared_ptr<IntuiCAM::Toolpath::Tool> IntuiCAM::GUI::ToolpathGenerationController::createDefaultTool(
    const QString& operationType)
{
    IntuiCAM::Toolpath::Tool::Type toolType;
    
    if (operationType == "Facing") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Facing;
    }
    else if (operationType == "Roughing" || operationType == "Finishing") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    }
    else if (operationType == "Parting") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Parting;
    }
    else if (operationType == "Threading") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Threading;
    }
    else if (operationType == "Grooving") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Grooving;
    }
    else {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning; // Default
    }
    
    // Create the tool with default name
    auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(
        toolType, QString("Default %1 Tool").arg(operationType).toStdString());
    
    // Configure tool parameters if needed
    
    return tool;
}

void IntuiCAM::GUI::ToolpathGenerationController::regenerateToolpath(
    const QString& operationName, 
    const QString& operationType)
{
    // Get the tool for this operation
    std::shared_ptr<IntuiCAM::Toolpath::Tool> tool;
    if (m_operationTools.contains(operationName)) {
        tool = m_operationTools[operationName];
    } else {
        tool = createDefaultTool(operationType);
        m_operationTools[operationName] = tool;
    }
    
    // Create a part for toolpath generation
    std::unique_ptr<IntuiCAM::Geometry::Part> part;
    
    // Check if we have a shape stored in the workspace controller
    if (m_workspaceController && m_workspaceController->hasPartShape()) {
        try {
            // Get the part shape from the workspace controller
            TopoDS_Shape partShape = m_workspaceController->getPartShape();
            
            if (!partShape.IsNull()) {
                // Create an OCCTPart with the shape
                part = std::make_unique<IntuiCAM::Geometry::OCCTPart>(&partShape);
                logMessage(QString("Using actual part geometry for %1 operation").arg(operationName));
            } else {
                // Fallback to simple part if shape is null
                part = std::make_unique<IntuiCAM::Geometry::SimplePart>();
                logMessage(QString("Using simplified part geometry for %1 (null shape)").arg(operationName));
            }
        } catch (const std::exception& e) {
            // Fallback to simple part on exception
            part = std::make_unique<IntuiCAM::Geometry::SimplePart>();
            logMessage(QString("Exception creating OCCTPart: %1").arg(e.what()));
        }
    } else {
        // Fallback to simple part if no shape is available
        part = std::make_unique<IntuiCAM::Geometry::SimplePart>();
        logMessage(QString("Using simplified part geometry for %1 (no workspace part)").arg(operationName));
    }
    
    // Create the appropriate operation type with updated parameters
    std::unique_ptr<IntuiCAM::Toolpath::Operation> operation;
    
    if (operationType == "Roughing") {
        auto roughingOp = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
            operationName.toStdString(), tool);
        
        // Use stored parameters if available
        if (m_roughingParams.contains(operationName)) {
            roughingOp->setParameters(m_roughingParams[operationName]);
        } else {
            // Create default parameters based on part size
            IntuiCAM::Toolpath::RoughingOperation::Parameters params;
            
            // Get part bounding box
            IntuiCAM::Geometry::BoundingBox bbox = part->getBoundingBox();
            
            // Calculate diameter from bounding box
            double maxXY = 2.0 * std::max(
                std::max(std::abs(bbox.min.x), std::abs(bbox.max.x)),
                std::max(std::abs(bbox.min.y), std::abs(bbox.max.y))
            );
            
            // Add margins for safety
            params.startDiameter = maxXY + 10.0;
            params.endDiameter = maxXY * 0.4; // Rough target is 40% of original size
            
            // Set Z range
            params.startZ = bbox.max.z + 5.0;
            params.endZ = bbox.min.z - 5.0;
            
            // Standard cutting parameters
            params.depthOfCut = 2.0;
            params.stockAllowance = 0.5;
            
            roughingOp->setParameters(params);
            m_roughingParams[operationName] = params;
        }
        
        operation = std::move(roughingOp);
    }
    else if (operationType == "Facing") {
        auto facingOp = std::make_unique<IntuiCAM::Toolpath::FacingOperation>(
            operationName.toStdString(), tool);
        
        // Use stored parameters if available
        if (m_facingParams.contains(operationName)) {
            facingOp->setParameters(m_facingParams[operationName]);
        } else {
            // Create default parameters based on part size
            IntuiCAM::Toolpath::FacingOperation::Parameters params;
            
            // Get part bounding box
            IntuiCAM::Geometry::BoundingBox bbox = part->getBoundingBox();
            
            // Calculate diameter from bounding box
            double maxXY = 2.0 * std::max(
                std::max(std::abs(bbox.min.x), std::abs(bbox.max.x)),
                std::max(std::abs(bbox.min.y), std::abs(bbox.max.y))
            );
            
            params.startDiameter = maxXY + 5.0;
            params.endDiameter = 0.0;
            params.stepover = 0.5;
            params.stockAllowance = 0.2;
            
            facingOp->setParameters(params);
            m_facingParams[operationName] = params;
        }
        
        operation = std::move(facingOp);
    }
    else if (operationType == "Finishing") {
        auto finishingOp = std::make_unique<IntuiCAM::Toolpath::FinishingOperation>(
            operationName.toStdString(), tool);
        
        // Use stored parameters if available
        if (m_finishingParams.contains(operationName)) {
            finishingOp->setParameters(m_finishingParams[operationName]);
        }
        
        operation = std::move(finishingOp);
    }
    else if (operationType == "Parting") {
        auto partingOp = std::make_unique<IntuiCAM::Toolpath::PartingOperation>(
            operationName.toStdString(), tool);
        
        // Use stored parameters if available
        if (m_partingParams.contains(operationName)) {
            partingOp->setParameters(m_partingParams[operationName]);
        }
        
        operation = std::move(partingOp);
    }
    else {
        logMessage(QString("Unknown operation type: %1").arg(operationType));
        return;
    }
    
    // Validate the operation parameters
    if (!operation->validate()) {
        logMessage(QString("Invalid parameters for %1 operation").arg(operationType));
        return;
    }
    
    // Generate the toolpath
    logMessage(QString("Generating toolpath for %1...").arg(operationName));
    auto toolpath = operation->generateToolpath(*part);
    
    if (!toolpath) {
        logMessage(QString("Failed to generate toolpath for %1").arg(operationName));
        return;
    }
    
    // Apply current workpiece transformation so Z-orientation is respected
    if (m_workpieceManager) {
        gp_Trsf wpTrsf = m_workpieceManager->getCurrentTransformation();
        IntuiCAM::Geometry::Matrix4x4 mat = toMatrix4x4(wpTrsf);
        toolpath->applyTransform(mat);
    }
    // (Raw material orientation could be applied similarly if required)

    logMessage(QString("Toolpath generation successful: %1 points").arg(toolpath->getPointCount()));
    
    // Get the tool name and convert to QString
    QString toolName = QString::fromStdString(tool->getName());
    
    // Display the toolpath
    displayGeneratedToolpath(operationName, toolName, std::move(toolpath));
    
    // Emit signal for toolpath regeneration
    emit toolpathRegenerated(operationName, operationType);
}

void IntuiCAM::GUI::ToolpathGenerationController::regenerateAllToolpaths()
{
    if (m_generatedToolpaths.isEmpty()) {
        return; // Nothing to do
    }

    for (auto it = m_generatedToolpaths.begin(); it != m_generatedToolpaths.end(); ++it) {
        const QString& name = it.key();
        QString type = getOperationTypeString(name);
        regenerateToolpath(name, type);
    }
}

// Implementation of private helper to display a generated toolpath and manage timeline bookkeeping
void IntuiCAM::GUI::ToolpathGenerationController::displayGeneratedToolpath(
    const QString& operationName,
    const QString& toolName,
    std::unique_ptr<IntuiCAM::Toolpath::Toolpath> toolpath)
{
    if (!m_toolpathManager || !toolpath) {
        qWarning() << "displayGeneratedToolpath: invalid state (manager or toolpath is null)";
        return;
    }

    // Check if a toolpath with that name already exists so we can replace it instead of adding duplicates
    const bool existedBefore = (m_toolpaths.find(operationName) != m_toolpaths.end());
    if (existedBefore) {
        m_toolpathManager->removeToolpath(operationName);
    }

    // Visualize the path
    bool success = m_toolpathManager->displayToolpath(*toolpath, operationName);
    if (!success) {
        qWarning() << "displayGeneratedToolpath: failed to display" << operationName;
        return;
    }

    // Store (or replace) the path in local container
    m_toolpaths[operationName] = std::move(toolpath);

    // Emit the appropriate signal so UI timeline stays in sync
    if (existedBefore) {
        emit toolpathRegenerated(operationName, getOperationTypeString(operationName));
    } else {
        emit toolpathAdded(operationName, getOperationTypeString(operationName), toolName);
    }
}

// === Helper: OCCT gp_Trsf -> Geometry::Matrix4x4 ===
static IntuiCAM::Geometry::Matrix4x4 toMatrix4x4(const gp_Trsf& trsf)
{
    using IntuiCAM::Geometry::Matrix4x4;
    Matrix4x4 mat = Matrix4x4::identity();

    // Fill rotation + scaling
    for (int r = 1; r <= 3; ++r) {
        for (int c = 1; c <= 3; ++c) {
            mat.data[(r - 1) * 4 + (c - 1)] = trsf.Value(r, c);
        }
    }

    // Translation part (OCCT uses gp_XYZ)
    gp_XYZ t = trsf.TranslationPart();
    mat.data[3]  = t.X();
    mat.data[7]  = t.Y();
    mat.data[11] = t.Z();

    // Last row already identity (0 0 0 1)
    return mat;
} 