#include "toolpathgenerationcontroller.h"
#include "setupconfigurationpanel.h"
#include "../include/toolpathmanager.h"
#include "../include/toolpathtimelinewidget.h"
#include "../include/workspacecontroller.h"
#include "../include/workpiecemanager.h"
#include "../include/rawmaterialmanager.h"
#include "../include/operationparameterdialog.h"
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
#include <QElapsedTimer>

// Core includes for actual toolpath generation
#include <IntuiCAM/Toolpath/Operations.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Toolpath/ContouringOperation.h>

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
        // Initialize a simple bounding box
        boundingBox_.min = Point3D(0.0, 0.0, 0.0);
        boundingBox_.max = Point3D(50.0, 50.0, 100.0);
    }
    
    virtual ~SimplePart() = default;
    
    double getVolume() const override { return volume_; }
    double getSurfaceArea() const override { return surfaceArea_; }
    
    BoundingBox getBoundingBox() const override { return boundingBox_; }
    
    std::unique_ptr<GeometricEntity> clone() const override {
        return std::make_unique<SimplePart>(volume_, surfaceArea_);
    }
    
    std::unique_ptr<Mesh> generateMesh(double tolerance = 0.1) const override {
        // Return a simple placeholder mesh
        return std::make_unique<Mesh>();
    }
    
    std::vector<Point3D> detectCylindricalFeatures() const override {
        // Return some sample cylindrical features
        return {Point3D(25.0, 25.0, 50.0)};
    }
    
    std::optional<double> getLargestCylinderDiameter() const override {
        return 50.0; // Return a reasonable diameter
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
    , m_realTimeUpdatesEnabled(true)
    , m_debounceTimer(new QTimer(this))
    , m_hasCachedRequest(false)
{
    // Setup process timer for step-by-step generation
    m_processTimer->setSingleShot(true);
    
    // Setup debounce timer for parameter changes
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(500); // 500ms debounce delay
    connect(m_debounceTimer, &QTimer::timeout, 
            this, &ToolpathGenerationController::processPendingParameterChanges);
    
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
    m_context = context;

    if (m_toolpathManager) {
        if (m_context.IsNull()) {
            emit errorOccurred("Invalid AIS context - toolpaths cannot be displayed");
        } else {
            m_toolpathManager->initialize(m_context);
        }
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::setWorkspaceController(WorkspaceController* workspaceController)
{
    m_workspaceController = workspaceController;
    if (workspaceController) {
        m_workpieceManager = workspaceController->getWorkpieceManager();
        m_rawMaterialManager = workspaceController->getRawMaterialManager();
        if (m_toolpathManager) {
            m_toolpathManager->setWorkpieceManager(m_workpieceManager);
        }
    } else {
        m_workpieceManager = nullptr;
        m_rawMaterialManager = nullptr;
        if (m_toolpathManager) {
            m_toolpathManager->setWorkpieceManager(nullptr);
        }
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::generateToolpaths(const GenerationRequest& request)
{
    QMutexLocker locker(&m_statusMutex);
    
    if (m_status != GenerationStatus::Idle) {
        emit errorOccurred("Generation already in progress. Please wait or cancel current operation.");
        return;
    }

    if (!m_workspaceController || !m_workspaceController->isInitialized()) {
        emit errorOccurred("Workspace not initialized - load a part before generating toolpaths");
        return;
    }

    if (!m_toolpathManager) {
        emit errorOccurred("Toolpath manager not initialized");
        return;
    }
    
    // Store the request and cache parameters
    m_currentRequest = request;
    cacheParameters(request);
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
    logMessage("Analyzing part geometry...");
    QCoreApplication::processEvents();
    
    // Check if we have valid input geometry
    if (m_currentRequest.stepFilePath.isEmpty() && m_currentRequest.partShape.IsNull()) {
        logMessage("ERROR: No part geometry provided for analysis");
        return false;
    }
    
    try {
        // Try to get part shape from workspace controller if available
        TopoDS_Shape analysisShape;
        if (m_workspaceController && m_workspaceController->hasPartShape()) {
            analysisShape = m_workspaceController->getPartShape();
            logMessage("Using part shape from workspace controller");
        } else if (!m_currentRequest.partShape.IsNull()) {
            analysisShape = m_currentRequest.partShape;
            logMessage("Using part shape from generation request");
        } else {
            logMessage("WARNING: No valid part shape available, using default analysis");
            // Continue with default analysis for compatibility
        }
        
        if (!analysisShape.IsNull()) {
            // Create OCCTPart for detailed analysis
            std::unique_ptr<IntuiCAM::Geometry::Part> part = 
                std::make_unique<IntuiCAM::Geometry::OCCTPart>(&analysisShape);
            
            // Analyze bounding box
            auto bbox = part->getBoundingBox();
            logMessage(QString("Part bounding box: X[%1, %2], Y[%3, %4], Z[%5, %6]")
                     .arg(bbox.min.x, 0, 'f', 2).arg(bbox.max.x, 0, 'f', 2)
                     .arg(bbox.min.y, 0, 'f', 2).arg(bbox.max.y, 0, 'f', 2)
                     .arg(bbox.min.z, 0, 'f', 2).arg(bbox.max.z, 0, 'f', 2));
            
            // Calculate maximum diameter
            double maxRadius = std::max(
                std::max(std::abs(bbox.min.x), std::abs(bbox.max.x)),
                std::max(std::abs(bbox.min.y), std::abs(bbox.max.y))
            );
            double maxDiameter = maxRadius * 2.0;
            
            logMessage(QString("Maximum part diameter: %1 mm").arg(maxDiameter, 0, 'f', 2));
            
            // Detect cylindrical features
            logMessage("Detecting cylindrical features...");
            auto cylindricalFeatures = part->detectCylindricalFeatures();
            logMessage(QString("Found %1 cylindrical features").arg(cylindricalFeatures.size()));
            
            // Get largest cylinder diameter
            auto largestDiameter = part->getLargestCylinderDiameter();
            if (largestDiameter.has_value()) {
                logMessage(QString("Largest cylinder diameter: %1 mm").arg(largestDiameter.value(), 0, 'f', 2));
                
                // Update request parameters if they seem unreasonable
                if (m_currentRequest.rawDiameter < largestDiameter.value()) {
                    logMessage(QString("WARNING: Raw material diameter (%1 mm) is smaller than part diameter (%2 mm)")
                             .arg(m_currentRequest.rawDiameter, 0, 'f', 2)
                             .arg(largestDiameter.value(), 0, 'f', 2));
                    m_currentResult.warnings.append("Raw material diameter may be insufficient");
                }
            }
            
            // Calculate part volume and surface area
            double volume = part->getVolume();
            double surfaceArea = part->getSurfaceArea();
            
            logMessage(QString("Part volume: %1 mm³").arg(volume, 0, 'f', 1));
            logMessage(QString("Part surface area: %1 mm²").arg(surfaceArea, 0, 'f', 1));
            
            // Analyze machining complexity
            double complexityFactor = 1.0;
            if (cylindricalFeatures.size() > 3) {
                complexityFactor += 0.2; // More complex if many features
            }
            if (bbox.size().z > bbox.size().x * 2) {
                complexityFactor += 0.1; // More complex if very long
            }
            
            logMessage(QString("Part complexity factor: %1").arg(complexityFactor, 0, 'f', 2));
            
            // Adjust time estimates based on complexity
            for (auto& estimate : OPERATION_TIME_ESTIMATES.toStdMap()) {
                m_currentResult.estimatedMachiningTime += estimate.second * complexityFactor;
            }
            
        } else {
            logMessage("Using simplified geometry analysis");
            
            // Basic fallback analysis using request parameters
            logMessage(QString("Raw material diameter: %1 mm").arg(m_currentRequest.rawDiameter, 0, 'f', 2));
            logMessage(QString("Distance to chuck: %1 mm").arg(m_currentRequest.distanceToChuck, 0, 'f', 2));
            
            if (m_currentRequest.rawDiameter <= 0.0) {
                logMessage("ERROR: Invalid raw material diameter");
                return false;
            }
        }
        
        // Material type analysis
        QString materialStr = "Steel"; // Default assumption
        // TODO: Implement material type enum handling when MaterialType is defined
        
        logMessage(QString("Material type: %1").arg(materialStr));
        
        // Surface finish requirements
        QString surfaceFinishStr = "Standard";
        // TODO: Implement surface finish enum handling when SurfaceFinish is defined
        
        logMessage(QString("Required surface finish: %1").arg(surfaceFinishStr));
        
        logMessage("Part geometry analysis completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        logMessage(QString("ERROR during geometry analysis: %1").arg(e.what()));
        return false;
    } catch (...) {
        logMessage("ERROR: Unknown exception during geometry analysis");
        return false;
    }
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

    if (totalOperations == 0) {
        logMessage("No operations scheduled for generation");
        return false;
    }

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
                // Use our new ContouringOperation for comprehensive toolpath generation
                try {
                    updateProgress(operationProgress + 5, "Extracting part profile...");
                    
                    // Extract part geometry for contouring
                    std::unique_ptr<IntuiCAM::Geometry::Part> part;
                    if (m_workspaceController && m_workspaceController->hasPartShape()) {
                        TopoDS_Shape partShape = m_workspaceController->getPartShape();
                        part = std::make_unique<IntuiCAM::Geometry::OCCTPart>(&partShape);
                    } else {
                        // Fallback to dummy part
                        part = std::make_unique<IntuiCAM::Geometry::SimplePart>();
                    }
                    
                    // Create contouring operation
                    IntuiCAM::Toolpath::ContouringOperation contouringOp;
                    
                    // Setup contouring parameters from request
                    IntuiCAM::Toolpath::ContouringOperation::Parameters contourParams;
                    contourParams.safetyHeight = 5.0;
                    contourParams.clearanceDistance = 1.0;
                    
                    // Configure sub-operations based on enabled operations in request
                    // If the user enables "Contouring", run all sub-operations
                    // (facing, roughing, finishing) by default. Individual
                    // operations can still be forced via explicit selections or
                    // non-zero allowances.
                    const bool contouringSelected =
                        m_currentRequest.enabledOperations.contains("Contouring");

                    contourParams.enableFacing = contouringSelected ||
                                               m_currentRequest.enabledOperations.contains("Facing") ||
                                               m_currentRequest.facingAllowance > 0.0;

                    contourParams.enableRoughing = contouringSelected ||
                                                 m_currentRequest.enabledOperations.contains("Roughing") ||
                                                 m_currentRequest.roughingAllowance > 0.0;

                    contourParams.enableFinishing = contouringSelected ||
                                                  m_currentRequest.enabledOperations.contains("Finishing") ||
                                                  m_currentRequest.finishingAllowance > 0.0;
                    
                    // Set operation-specific parameters
                    if (contourParams.enableFacing) {
                        // FacingOperation parameters: startDiameter, endDiameter, stepover, stockAllowance, roughingOnly
                        contourParams.facingParams.stockAllowance = m_currentRequest.facingAllowance;
                    }
                    
                    if (contourParams.enableRoughing) {
                        // RoughingOperation parameters: startDiameter, endDiameter, startZ, endZ, depthOfCut, stockAllowance
                        contourParams.roughingParams.depthOfCut = m_currentRequest.roughingAllowance > 0.0 ? 
                                                                m_currentRequest.roughingAllowance : 1.0;
                        contourParams.roughingParams.stockAllowance = m_currentRequest.finishingAllowance;
                    }
                    
                    if (contourParams.enableFinishing) {
                        // FinishingOperation parameters: targetDiameter, startZ, endZ, surfaceSpeed, feedRate
                        contourParams.finishingParams.feedRate = m_currentRequest.finishingAllowance > 0.0 ?
                                                                m_currentRequest.finishingAllowance * 0.1 : 0.05;
                        contourParams.finishingParams.surfaceSpeed = 150.0;  // m/min for good surface finish
                    }
                    
                    updateProgress(operationProgress + 10, "Generating contouring toolpaths...");
                    
                    // Generate the complete contouring operation
                    auto contourResult = contouringOp.generateToolpaths(*part, tool, contourParams);
                    
                    if (!contourResult.success) {
                        emit operationCompleted(operationName, false, QString("Contouring failed: %1")
                                              .arg(QString::fromStdString(contourResult.errorMessage)));
                        continue;
                    }
                    
                    updateProgress(operationProgress + 15, "Displaying profile and toolpaths...");
                    
                    // Display the extracted profile
                    if (m_toolpathManager && !contourResult.extractedProfile.empty()) {
                        m_toolpathManager->displayLatheProfile(contourResult.extractedProfile, "ContourProfile");
                        logMessage(QString("Extracted profile with %1 points").arg(contourResult.extractedProfile.size()));
                    }
                    
                    // Display generated toolpaths
                    int toolpathDisplayed = 0;
                    if (contourResult.facingToolpath && contourParams.enableFacing) {
                        if (m_toolpathManager->displayToolpath(*contourResult.facingToolpath, "Facing")) {
                            toolpathDisplayed++;
                            emit operationCompleted("Facing", true, "Facing toolpath generated successfully");
                        }
                    }
                    
                    if (contourResult.roughingToolpath && contourParams.enableRoughing) {
                        if (m_toolpathManager->displayToolpath(*contourResult.roughingToolpath, "Roughing")) {
                            toolpathDisplayed++;
                            emit operationCompleted("Roughing", true, "Roughing toolpath generated successfully");
                        }
                    }
                    
                    if (contourResult.finishingToolpath && contourParams.enableFinishing) {
                        if (m_toolpathManager->displayToolpath(*contourResult.finishingToolpath, "Finishing")) {
                            toolpathDisplayed++;
                            emit operationCompleted("Finishing", true, "Finishing toolpath generated successfully");
                        }
                    }
                    
                    if (toolpathDisplayed > 0) {
                        logMessage(QString("Successfully generated %1 contouring toolpaths").arg(toolpathDisplayed));
                        logMessage(QString("Estimated machining time: %1 minutes").arg(contourResult.estimatedTime, 0, 'f', 1));
                        logMessage(QString("Total moves: %1").arg(contourResult.totalMoves));
                        
                        // Update result statistics
                        m_currentResult.estimatedMachiningTime += contourResult.estimatedTime;
                        
                        emit operationCompleted(operationName, true, 
                            QString("Contouring completed: %1 toolpaths, %2 min estimated")
                            .arg(toolpathDisplayed).arg(contourResult.estimatedTime, 0, 'f', 1));
                    } else {
                        emit operationCompleted(operationName, false, "No toolpaths were generated");
                    }
                    
                    // Skip to next operation - don't create the old operation object
                    continue;
                    
                } catch (const std::exception& e) {
                    emit operationCompleted(operationName, false, 
                        QString("Contouring exception: %1").arg(e.what()));
                    continue;
                }
            }
            else {
                // Handle individual operations using standard operations
                operation = createOperation(operationName);
                
                if (!operation) {
                    logMessage(QString("Unsupported operation type: %1").arg(operationName));
                    emit operationCompleted(operationName, false, QString("Operation type %1 not supported").arg(operationName));
                    continue;
                }
                
                logMessage(QString("Created %1 operation").arg(operationName));
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
            
            // Get the actual part geometry
            std::unique_ptr<IntuiCAM::Geometry::Part> part;
            if (m_workspaceController && m_workspaceController->hasPartShape()) {
                TopoDS_Shape partShape = m_workspaceController->getPartShape();
                part = std::make_unique<IntuiCAM::Geometry::OCCTPart>(&partShape);
                logMessage(QString("Using actual part geometry for %1").arg(operationName));
            } else {
                // Fallback to dummy part
                part = std::make_unique<IntuiCAM::Geometry::SimplePart>();
                logMessage(QString("Using fallback geometry for %1").arg(operationName));
            }
            
            // Generate the toolpath
            auto toolpath = operation->generateToolpath(*part);
            
            if (!toolpath) {
                emit operationCompleted(operationName, false, "Failed to generate toolpath");
                continue;
            }
            
            // Store the toolpath for optimization and validation
            m_toolpaths[operationName] = std::move(toolpath);
            
            // Display the toolpath
            if (m_toolpathManager) {
                bool displayed = m_toolpathManager->displayToolpath(*m_toolpaths[operationName], operationName);
                if (!displayed) {
                    emit operationCompleted(operationName, false, "Failed to display toolpath");
                    continue;
                }
            }
            
            logMessage(QString("Successfully generated and displayed %1 toolpath").arg(operationName));
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
    logMessage("Starting toolpath optimization...");
    QCoreApplication::processEvents();
    
    if (m_toolpathManager && !m_toolpaths.empty()) {
        int optimizedCount = 0;
        
        // Optimize each generated toolpath
        for (auto it = m_toolpaths.begin(); it != m_toolpaths.end(); ++it) {
            const QString& operationName = it->first;
            auto& toolpath = it->second;
            
            if (toolpath) {
                logMessage(QString("Optimizing %1 toolpath...").arg(operationName));
                
                // Store original movement count for comparison
                size_t originalMovements = toolpath->getMovementCount();
                
                // Apply optimization algorithms
                toolpath->optimizeToolpath();
                
                // Calculate optimization results
                size_t optimizedMovements = toolpath->getMovementCount();
                double reduction = 0.0;
                if (originalMovements > 0) {
                    reduction = (double)(originalMovements - optimizedMovements) / originalMovements * 100.0;
                }
                
                if (reduction > 0.0) {
                    logMessage(QString("  Reduced %1 movements by %2%")
                             .arg(operationName).arg(reduction, 0, 'f', 1));
                    optimizedCount++;
                } else {
                    logMessage(QString("  No optimization needed for %1").arg(operationName));
                }
                
                QCoreApplication::processEvents();
            }
        }
        
        logMessage(QString("Optimization complete: %1 toolpaths optimized").arg(optimizedCount));
        
        // Update statistics in result
        for (const auto& operationName : m_currentResult.generatedOperations) {
            if (m_toolpaths.find(operationName) != m_toolpaths.end()) {
                auto& toolpath = m_toolpaths[operationName];
                if (toolpath) {
                    double operationTime = toolpath->estimateMachiningTime();
                    m_currentResult.estimatedMachiningTime += operationTime;
                }
            }
        }
        
        return true;
    } else {
        logMessage("No toolpaths available for optimization");
        return false;
    }
}

bool IntuiCAM::GUI::ToolpathGenerationController::validateResults()
{
    logMessage("Starting toolpath validation...");
    QCoreApplication::processEvents();
    
    bool allValid = true;
    int totalValidToolpaths = 0;
    int totalMovements = 0;
    double totalEstimatedTime = 0.0;
    
    // Validate each generated toolpath
    for (auto it = m_toolpaths.begin(); it != m_toolpaths.end(); ++it) {
        const QString& operationName = it->first;
        auto& toolpath = it->second;
        
        if (!toolpath) {
            logMessage(QString("ERROR: Null toolpath for operation %1").arg(operationName));
            allValid = false;
            continue;
        }
        
        // Basic toolpath validation
        logMessage(QString("Validating %1 toolpath...").arg(operationName));
        
        // Check for empty toolpaths
        if (toolpath->getMovementCount() == 0) {
            logMessage(QString("WARNING: Empty toolpath for %1").arg(operationName));
            m_currentResult.warnings.append(QString("%1 toolpath is empty").arg(operationName));
            continue;
        }
        
        // Validate bounding box
        auto bbox = toolpath->getBoundingBox();
        bool validBounds = (bbox.min.x != bbox.max.x || bbox.min.y != bbox.max.y || bbox.min.z != bbox.max.z);
        if (!validBounds) {
            logMessage(QString("WARNING: Invalid bounding box for %1").arg(operationName));
            m_currentResult.warnings.append(QString("%1 toolpath has invalid geometry").arg(operationName));
        }
        
        // Safety checks for toolpath positions
        bool hasNegativeRadius = false;
        bool hasExtremePositions = false;
        
        for (const auto& movement : toolpath->getMovements()) {
            // Check for negative radial positions (invalid for lathe)
            double radius = std::sqrt(movement.position.x * movement.position.x + 
                                    movement.position.y * movement.position.y);
            if (radius < 0.0) {
                hasNegativeRadius = true;
            }
            
            // Check for extreme positions that might indicate calculation errors
            if (std::abs(movement.position.x) > 1000.0 || 
                std::abs(movement.position.y) > 1000.0 || 
                std::abs(movement.position.z) > 1000.0) {
                hasExtremePositions = true;
            }
        }
        
        if (hasNegativeRadius) {
            logMessage(QString("WARNING: %1 has negative radial positions").arg(operationName));
            m_currentResult.warnings.append(QString("%1 toolpath contains invalid radial positions").arg(operationName));
        }
        
        if (hasExtremePositions) {
            logMessage(QString("WARNING: %1 has extreme position values").arg(operationName));
            m_currentResult.warnings.append(QString("%1 toolpath contains extreme position values").arg(operationName));
        }
        
        // Calculate statistics
        totalMovements += static_cast<int>(toolpath->getMovementCount());
        totalEstimatedTime += toolpath->estimateMachiningTime();
        totalValidToolpaths++;
        
        logMessage(QString("  %1: %2 movements, %3 min estimated")
                 .arg(operationName)
                 .arg(toolpath->getMovementCount())
                 .arg(toolpath->estimateMachiningTime(), 0, 'f', 2));
        
        QCoreApplication::processEvents();
    }
    
    // Collision detection between toolpaths (basic check)
    logMessage("Checking for potential collisions...");
    if (m_toolpaths.size() > 1) {
        // Check for overlapping bounding boxes between different toolpaths
        // Store raw pointers to avoid issues with references in containers
        std::vector<std::pair<QString, IntuiCAM::Toolpath::Toolpath*>> toolpathsVec;
        toolpathsVec.reserve(m_toolpaths.size());
        for (auto& pair : m_toolpaths) {
            toolpathsVec.emplace_back(pair.first, pair.second.get());
        }
        
        bool collisionFound = false;
        for (size_t i = 0; i < toolpathsVec.size(); ++i) {
            for (size_t j = i + 1; j < toolpathsVec.size(); ++j) {
                IntuiCAM::Toolpath::Toolpath* toolpath1 = toolpathsVec[i].second;
                IntuiCAM::Toolpath::Toolpath* toolpath2 = toolpathsVec[j].second;
                
                if (toolpath1 && toolpath2) {
                    auto bbox1 = toolpath1->getBoundingBox();
                    auto bbox2 = toolpath2->getBoundingBox();
                    
                    // Check for bounding box overlap
                    bool overlap = !(bbox1.max.x < bbox2.min.x || bbox2.max.x < bbox1.min.x ||
                                   bbox1.max.y < bbox2.min.y || bbox2.max.y < bbox1.min.y ||
                                   bbox1.max.z < bbox2.min.z || bbox2.max.z < bbox1.min.z);
                    
                    if (overlap) {
                        logMessage(QString("Potential collision detected between %1 and %2")
                                 .arg(toolpathsVec[i].first).arg(toolpathsVec[j].first));
                        m_currentResult.warnings.append(QString("Potential collision: %1 and %2")
                                                       .arg(toolpathsVec[i].first).arg(toolpathsVec[j].first));
                        collisionFound = true;
                    }
                }
            }
        }
        
        if (!collisionFound) {
            logMessage("Collision detection completed - no conflicts found");
        }
    } else {
        logMessage("Single toolpath - no collision check needed");
    }
    
    // Update final result statistics
    m_currentResult.totalToolpaths = totalValidToolpaths;
    m_currentResult.estimatedMachiningTime = totalEstimatedTime;
    
    // Sequence validation
    logMessage("Verifying operation sequence...");
    bool sequenceValid = true;
    
    // Check that operations follow a logical sequence for turning
    QStringList idealSequence = {"Facing", "Roughing", "Finishing", "Parting"};
    for (int i = 0; i < m_currentResult.generatedOperations.size() - 1; ++i) {
        QString current = m_currentResult.generatedOperations[i];
        QString next = m_currentResult.generatedOperations[i + 1];
        
        int currentIndex = idealSequence.indexOf(current);
        int nextIndex = idealSequence.indexOf(next);
        
        if (currentIndex >= 0 && nextIndex >= 0 && currentIndex > nextIndex) {
            logMessage(QString("WARNING: Non-optimal operation sequence: %1 after %2")
                     .arg(next).arg(current));
            m_currentResult.warnings.append(QString("Operation sequence may not be optimal"));
            sequenceValid = false;
        }
    }
    
    if (sequenceValid) {
        logMessage("Operation sequence validation passed");
    }
    
    // Final validation summary
    if (allValid && totalValidToolpaths > 0) {
        logMessage(QString("Validation complete: %1 valid toolpaths, %2 total movements, %3 min estimated time")
                 .arg(totalValidToolpaths)
                 .arg(totalMovements)
                 .arg(totalEstimatedTime, 0, 'f', 2));
        return true;
    } else {
        logMessage(QString("Validation failed: %1 valid toolpaths out of %2 attempted")
                 .arg(totalValidToolpaths)
                 .arg(m_currentResult.generatedOperations.size()));
        return false;
    }
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

    // Append any additional enabled operations that are not part of the
    // predefined default order. This ensures custom or future operations
    // are still executed even if they are unknown to DEFAULT_OPERATION_ORDER.
    for (const QString& operation : m_currentRequest.enabledOperations) {
        if (!sequence.contains(operation)) {
            sequence.append(operation);
        }
    }

    return sequence;
}

bool IntuiCAM::GUI::ToolpathGenerationController::validateOperationCompatibility(const QString& operationName)
{
    // Sophisticated compatibility checks based on part geometry and material
    
    // Basic parameter validation
    if (operationName == "Contouring") {
        if (m_currentRequest.roughingAllowance <= 0.0 && m_currentRequest.finishingAllowance <= 0.0) {
            logMessage(QString("WARNING: %1 operation needs roughing or finishing allowance").arg(operationName));
            return false;
        }
        
        // Check part geometry compatibility
        if (m_workspaceController && m_workspaceController->hasPartShape()) {
            try {
                TopoDS_Shape partShape = m_workspaceController->getPartShape();
                auto part = std::make_unique<IntuiCAM::Geometry::OCCTPart>(&partShape);
                
                auto largestDiameter = part->getLargestCylinderDiameter();
                if (largestDiameter.has_value()) {
                    if (m_currentRequest.rawDiameter < largestDiameter.value() + 5.0) {
                        logMessage(QString("WARNING: Raw material diameter may be insufficient for %1").arg(operationName));
                        m_currentResult.warnings.append(QString("Raw material diameter should be at least %1 mm for proper %2")
                                                      .arg(largestDiameter.value() + 5.0, 0, 'f', 1).arg(operationName));
                    }
                }
            } catch (const std::exception& e) {
                logMessage(QString("Could not analyze part geometry for %1: %2").arg(operationName).arg(e.what()));
            }
        }
        
        return true;
    }

    if (operationName == "Threading") {
        // Threading requires specific conditions
        if (m_currentRequest.rawDiameter < 10.0) {
            logMessage(QString("WARNING: Threading not recommended for diameter < 10mm"));
            return false;
        }
        return true;
    }

    if (operationName == "Chamfering") {
        if (m_currentRequest.finishingAllowance <= 0.0) {
            logMessage(QString("WARNING: %1 needs finishing allowance").arg(operationName));
            return false;
        }
        
        // Chamfering typically follows other operations
        if (!m_currentRequest.enabledOperations.contains("Facing") && 
            !m_currentRequest.enabledOperations.contains("Roughing")) {
            logMessage(QString("WARNING: %1 is typically done after facing or roughing").arg(operationName));
            m_currentResult.warnings.append("Chamfering is usually performed after primary operations");
        }
        
        return true;
    }

    if (operationName == "Parting") {
        if (m_currentRequest.partingWidth <= 0.0) {
            logMessage(QString("WARNING: %1 needs valid parting width").arg(operationName));
            return false;
        }
        
        // Parting is typically the last operation
        if (m_currentRequest.enabledOperations.contains("Finishing") &&
            m_currentResult.generatedOperations.indexOf("Finishing") > 
            m_currentResult.generatedOperations.indexOf(operationName)) {
            logMessage(QString("WARNING: %1 should typically be the last operation").arg(operationName));
            m_currentResult.warnings.append("Parting should usually be performed last");
        }
        
        // Check if part is long enough for stable parting
        if (m_currentRequest.distanceToChuck < m_currentRequest.rawDiameter * 0.5) {
            logMessage(QString("WARNING: Part may be too short for stable parting operation"));
            m_currentResult.warnings.append("Short parts may require special considerations for parting");
        }
        
        return true;
    }

    if (operationName == "Facing") {
        // Facing is typically first operation
        if (m_currentRequest.facingAllowance < 0.0) {
            logMessage(QString("WARNING: Negative facing allowance not recommended"));
            return false;
        }
        
        // Check if facing makes sense given the part geometry
        if (m_currentRequest.orientationFlipped) {
            logMessage(QString("INFO: Facing operation adjusted for flipped part orientation"));
        }
        
        return true;
    }

    if (operationName == "Roughing") {
        if (m_currentRequest.roughingAllowance <= 0.0) {
            logMessage(QString("WARNING: %1 needs positive roughing allowance").arg(operationName));
            return false;
        }
        
        // Roughing should leave material for finishing
        if (m_currentRequest.enabledOperations.contains("Finishing") &&
            m_currentRequest.finishingAllowance >= m_currentRequest.roughingAllowance) {
            logMessage(QString("WARNING: Finishing allowance should be less than roughing allowance"));
            m_currentResult.warnings.append("Check allowance values for roughing and finishing operations");
        }
        
        return true;
    }

    if (operationName == "Finishing") {
        if (m_currentRequest.finishingAllowance < 0.0) {
            logMessage(QString("WARNING: Negative finishing allowance not recommended"));
            return false;
        }
        
        // Finishing typically follows roughing
        if (!m_currentRequest.enabledOperations.contains("Roughing")) {
            logMessage(QString("INFO: Finishing without roughing - using lighter cuts recommended"));
        }
        
        return true;
    }
    
    // Default case for unknown operations
    logMessage(QString("WARNING: Unknown operation type: %1").arg(operationName));
    return false;
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

        // Set parameters based on current request
        IntuiCAM::Toolpath::ThreadingOperation::Parameters params;
        params.majorDiameter = m_currentRequest.rawDiameter * 0.8; // Typical thread diameter
        params.pitch = 1.5; // Standard metric pitch
        params.isMetric = true;
        params.numberOfPasses = 3; // Multi-pass threading
        params.startZ = 0.0;
        params.threadLength = std::min(20.0, m_currentRequest.distanceToChuck * 0.3);
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Chamfering") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::FinishingOperation>(
            operationName.toStdString(), tool);

        // Set chamfering parameters (using finishing operation for simplicity)
        IntuiCAM::Toolpath::FinishingOperation::Parameters params;
        params.targetDiameter = m_currentRequest.rawDiameter - (m_currentRequest.finishingAllowance * 2.0);
        params.startZ = m_currentRequest.finishingAllowance;
        params.endZ = -m_currentRequest.finishingAllowance;
        params.feedRate = 0.02; // Slow feed for good finish
        params.surfaceSpeed = 200.0; // Higher speed for finishing
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Contouring") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
            operationName.toStdString(), tool);

        // Set contouring parameters
        IntuiCAM::Toolpath::RoughingOperation::Parameters params;
        params.startDiameter = m_currentRequest.rawDiameter + 2.0; // Start outside material
        params.endDiameter = m_currentRequest.rawDiameter * 0.4; // Reasonable end diameter
        params.startZ = 5.0; // Start above part
        params.endZ = -m_currentRequest.distanceToChuck + 5.0; // Don't cut into chuck
        params.depthOfCut = std::min(3.0, m_currentRequest.roughingAllowance);
        params.stockAllowance = m_currentRequest.finishingAllowance;
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Facing") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::FacingOperation>(
            operationName.toStdString(), tool);
        
        // Set facing parameters based on request
        IntuiCAM::Toolpath::FacingOperation::Parameters params;
        params.startDiameter = m_currentRequest.rawDiameter + 2.0; // Start outside
        params.endDiameter = 0.0; // Face to center
        params.stepover = std::min(1.0, m_currentRequest.rawDiameter * 0.05); // 5% of diameter
        params.stockAllowance = m_currentRequest.facingAllowance;
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Roughing") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(
            operationName.toStdString(), tool);
        
        // Set roughing parameters
        IntuiCAM::Toolpath::RoughingOperation::Parameters params;
        params.startDiameter = m_currentRequest.rawDiameter;
        params.endDiameter = m_currentRequest.rawDiameter * 0.5; // Remove 50% of material
        params.startZ = 0.0;
        params.endZ = -m_currentRequest.distanceToChuck + 10.0; // Leave clearance
        params.depthOfCut = std::min(2.5, m_currentRequest.roughingAllowance);
        params.stockAllowance = m_currentRequest.finishingAllowance;
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Finishing") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::FinishingOperation>(
            operationName.toStdString(), tool);
        
        // Set finishing parameters
        IntuiCAM::Toolpath::FinishingOperation::Parameters params;
        params.targetDiameter = m_currentRequest.rawDiameter * 0.5 - m_currentRequest.finishingAllowance;
        params.startZ = 0.0;
        params.endZ = -m_currentRequest.distanceToChuck + 10.0;
        params.feedRate = 0.05; // Slower feed for better finish
        params.surfaceSpeed = 180.0;
        
        operation->setParameters(params);
        return operation;
    }
    else if (operationName == "Parting") {
        auto operation = std::make_unique<IntuiCAM::Toolpath::PartingOperation>(
            operationName.toStdString(), tool);
        
        // Set parting parameters
        IntuiCAM::Toolpath::PartingOperation::Parameters params;
        params.partingDiameter = m_currentRequest.rawDiameter;
        params.centerHoleDiameter = 0.0; // Part off completely
        params.partingZ = -m_currentRequest.distanceToChuck + m_currentRequest.partingWidth;
        params.feedRate = 0.01; // Very slow for parting
        params.retractDistance = 5.0;
        
        operation->setParameters(params);
        return operation;
    }
    
    // Unknown operation type
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
    // Extract operation type from operation name
    if (operationName.contains("Facing", Qt::CaseInsensitive)) {
        return "Facing";
    }
    else if (operationName.contains("Roughing", Qt::CaseInsensitive)) {
        return "Roughing";
    }
    else if (operationName.contains("Finishing", Qt::CaseInsensitive)) {
        return "Finishing";
    }
    else if (operationName.contains("Parting", Qt::CaseInsensitive)) {
        return "Parting";
    }
    else if (operationName.contains("Threading", Qt::CaseInsensitive)) {
        return "Threading";
    }
    else if (operationName.contains("Grooving", Qt::CaseInsensitive)) {
        return "Grooving";
    }
    else if (operationName.contains("Contouring", Qt::CaseInsensitive)) {
        return "Contouring";
    }
    else if (operationName.contains("Chamfering", Qt::CaseInsensitive)) {
        return "Chamfering";
    }
    else {
        return "Unknown";
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::updateOperationParameters(
    const QString& operationName, 
    const QString& operationType, 
    void* params)
{
    if (!params) {
        logMessage(QString("ERROR: Null parameters for %1 operation").arg(operationName));
        return;
    }
    
    try {
        if (operationType == "Facing") {
            auto* facingParams = static_cast<IntuiCAM::Toolpath::FacingOperation::Parameters*>(params);
            m_facingParams[operationName] = *facingParams;
            
            logMessage(QString("Updated facing parameters for %1").arg(operationName));
        }
        else if (operationType == "Roughing") {
            auto* roughingParams = static_cast<IntuiCAM::Toolpath::RoughingOperation::Parameters*>(params);
            m_roughingParams[operationName] = *roughingParams;
            
            logMessage(QString("Updated roughing parameters for %1").arg(operationName));
        }
        else if (operationType == "Finishing") {
            auto* finishingParams = static_cast<IntuiCAM::Toolpath::FinishingOperation::Parameters*>(params);
            m_finishingParams[operationName] = *finishingParams;
            
            logMessage(QString("Updated finishing parameters for %1").arg(operationName));
        }
        else if (operationType == "Parting") {
            auto* partingParams = static_cast<IntuiCAM::Toolpath::PartingOperation::Parameters*>(params);
            m_partingParams[operationName] = *partingParams;
            
            logMessage(QString("Updated parting parameters for %1").arg(operationName));
        }
        else {
            logMessage(QString("WARNING: Unknown operation type for parameter update: %1").arg(operationType));
        }
        
        // Trigger regeneration if auto-update is enabled
        if (m_realTimeUpdatesEnabled) {
            regenerateToolpath(operationName, operationType);
        }
        
    } catch (const std::exception& e) {
        logMessage(QString("ERROR updating operation parameters: %1").arg(e.what()));
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::regenerateAllToolpaths()
{
    if (!m_workspaceController) {
        logMessage("ERROR: Cannot regenerate toolpaths - no workspace controller");
        return;
    }
    
    logMessage("Regenerating all toolpaths with updated part position...");
    
    // Store the current toolpath names and types
    QStringList operationNames;
    QStringList operationTypes;
    
    for (auto it = m_toolpaths.begin(); it != m_toolpaths.end(); ++it) {
        operationNames.append(it->first);
        operationTypes.append(getOperationTypeString(it->first));
    }
    
    // Clear existing toolpaths from display
    if (m_toolpathManager) {
        m_toolpathManager->clearAllToolpaths();
    }
    
    // Regenerate each toolpath with updated geometry
    for (int i = 0; i < operationNames.size(); ++i) {
        const QString& name = operationNames[i];
        const QString& type = operationTypes[i];
        
        logMessage(QString("Regenerating %1 (%2)...").arg(name).arg(type));
        regenerateToolpath(name, type);
    }
    
    logMessage(QString("Regenerated %1 toolpaths").arg(operationNames.size()));
}

static IntuiCAM::Geometry::Matrix4x4 toMatrix4x4(const gp_Trsf& trsf)
{
    IntuiCAM::Geometry::Matrix4x4 mat;
    
    // Extract rotation matrix (3x3) - OpenCASCADE uses 1-based indexing
    // Matrix4x4 uses data[16] in row-major order
    for (int i = 1; i <= 3; ++i) {
        for (int j = 1; j <= 3; ++j) {
            mat.data[(i-1)*4 + (j-1)] = trsf.Value(i, j);
        }
    }
    
    // Extract translation vector
    gp_XYZ translation = trsf.TranslationPart();
    // Matrix4x4 stores translation in the last column (indices 12,13,14)
    mat.data[12] = translation.X();
    mat.data[13] = translation.Y();
    mat.data[14] = translation.Z();

    // Ensure homogeneous coordinate row is valid
    mat.data[3] = 0.0;
    mat.data[7] = 0.0;
    mat.data[11] = 0.0;
    mat.data[15] = 1.0;
    
    return mat;
}

void IntuiCAM::GUI::ToolpathGenerationController::displayGeneratedToolpath(
    const QString& operationName,
    const QString& toolName,
    std::unique_ptr<IntuiCAM::Toolpath::Toolpath> toolpath)
{
    if (!toolpath || !m_toolpathManager) {
        logMessage(QString("Cannot display toolpath %1 - invalid toolpath or manager").arg(operationName));
        return;
    }
    
    // Apply workpiece transformation to align with current part position
    if (m_workpieceManager) {
        gp_Trsf transformation = m_workpieceManager->getCurrentTransformation();
        IntuiCAM::Geometry::Matrix4x4 matrix = toMatrix4x4(transformation);
        toolpath->applyTransform(matrix);
    }
    
    // Display the toolpath
    bool displayed = m_toolpathManager->displayToolpath(*toolpath, operationName);
    
    if (displayed) {
        // Store the toolpath
        m_toolpaths[operationName] = std::move(toolpath);
        
        // Emit signals for UI updates
        emit toolpathRegenerated(operationName, getOperationTypeString(operationName));
        emit toolpathAdded(operationName, getOperationTypeString(operationName), toolName);
        
        logMessage(QString("Successfully displayed toolpath: %1").arg(operationName));
    } else {
        logMessage(QString("Failed to display toolpath: %1").arg(operationName));
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::performIncrementalUpdate(const UpdateStrategy& strategy)
{
    QElapsedTimer timer;
    timer.start();
    
    logMessage("Performing incremental toolpath update...");
    
    // Update visual properties first (fastest)
    if (!strategy.visualOnlyUpdates.isEmpty()) {
        updateVisualProperties(strategy.visualOnlyUpdates);
        logMessage(QString("Updated %1 visual properties").arg(strategy.visualOnlyUpdates.size()));
    }
    
    // Regenerate specific operations if needed
    if (!strategy.operationsToRegenerate.isEmpty()) {
        for (const QString& operationName : strategy.operationsToRegenerate) {
            QString operationType = getOperationTypeString(operationName);
            regenerateToolpath(operationName, operationType);
        }
        logMessage(QString("Regenerated %1 operations").arg(strategy.operationsToRegenerate.size()));
    }
    
    // Full profile regeneration if required (slowest)
    if (strategy.needsProfileRegeneration) {
        regenerateContouringOperation();
        logMessage("Regenerated part profile and contouring operations");
    }
    
    int duration = timer.elapsed();
    logMessage(QString("Incremental update completed in %1 ms").arg(duration));
    
    emit incrementalUpdateCompleted(strategy.operationsToRegenerate, duration);
}

IntuiCAM::GUI::ToolpathGenerationController::UpdateStrategy 
IntuiCAM::GUI::ToolpathGenerationController::analyzeParameterChanges(const QList<ParameterChange>& changes)
{
    UpdateStrategy strategy;
    strategy.needsProfileRegeneration = false;
    
    for (const ParameterChange& change : changes) {
        switch (change.type) {
            case ParameterChangeType::Geometry:
                // Geometry changes require full profile regeneration
                strategy.needsProfileRegeneration = true;
                break;
                
            case ParameterChangeType::Tool:
                // Tool changes affect all toolpaths
                for (const auto& pair : m_toolpaths) {
                    if (!strategy.operationsToRegenerate.contains(pair.first)) {
                        strategy.operationsToRegenerate.append(pair.first);
                    }
                }
                break;
                
            case ParameterChangeType::Operation:
                // Operation changes affect specific operations
                if (!change.affectedOperations.isEmpty()) {
                    for (const QString& operation : change.affectedOperations) {
                        if (!strategy.operationsToRegenerate.contains(operation)) {
                            strategy.operationsToRegenerate.append(operation);
                        }
                    }
                } else {
                    // If no specific operations listed, determine from parameter name
                    QString operationName = change.parameterName.split('_').first();
                    if (m_toolpaths.find(operationName) != m_toolpaths.end()) {
                        if (!strategy.operationsToRegenerate.contains(operationName)) {
                            strategy.operationsToRegenerate.append(operationName);
                        }
                    }
                }
                break;
                
            case ParameterChangeType::Visual:
                // Visual changes only require display updates
                strategy.visualOnlyUpdates.append(change.parameterName);
                break;
        }
    }
    
    return strategy;
}

void IntuiCAM::GUI::ToolpathGenerationController::updateVisualProperties(const QStringList& visualParameters)
{
    if (!m_toolpathManager) {
        return;
    }
    
    for (const QString& parameter : visualParameters) {
        if (parameter.contains("color", Qt::CaseInsensitive)) {
            // Update toolpath colors
            for (const auto& pair : m_toolpaths) {
                m_toolpathManager->setToolpathVisible(pair.first, true);
            }
        }
        else if (parameter.contains("visibility", Qt::CaseInsensitive)) {
            // Update toolpath visibility
            for (const auto& pair : m_toolpaths) {
                m_toolpathManager->setToolpathVisible(pair.first, true);
            }
        }
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::cacheParameters(const GenerationRequest& request)
{
    QMutexLocker locker(&m_parameterMutex);
    
    m_cachedRequest = request;
    m_hasCachedRequest = true;
    
    logMessage("Parameters cached for incremental updates");
}

QList<IntuiCAM::GUI::ToolpathGenerationController::ParameterChange> 
IntuiCAM::GUI::ToolpathGenerationController::detectParameterChanges(const GenerationRequest& newRequest)
{
    QList<ParameterChange> changes;
    
    if (!m_hasCachedRequest) {
        return changes; // No previous request to compare
    }
    
    const GenerationRequest& cached = m_cachedRequest;
    
    // Check geometry changes
    if (newRequest.stepFilePath != cached.stepFilePath ||
        newRequest.rawDiameter != cached.rawDiameter ||
        newRequest.distanceToChuck != cached.distanceToChuck ||
        newRequest.orientationFlipped != cached.orientationFlipped) {
        
        changes.append(ParameterChange(ParameterChangeType::Geometry, "geometry", 
                                     QVariant(), QVariant()));
    }
    
    // Check operation parameter changes
    if (newRequest.facingAllowance != cached.facingAllowance) {
        changes.append(ParameterChange(ParameterChangeType::Operation, "facingAllowance",
                                     cached.facingAllowance, newRequest.facingAllowance));
    }
    
    if (newRequest.roughingAllowance != cached.roughingAllowance) {
        changes.append(ParameterChange(ParameterChangeType::Operation, "roughingAllowance",
                                     cached.roughingAllowance, newRequest.roughingAllowance));
    }
    
    if (newRequest.finishingAllowance != cached.finishingAllowance) {
        changes.append(ParameterChange(ParameterChangeType::Operation, "finishingAllowance",
                                     cached.finishingAllowance, newRequest.finishingAllowance));
    }
    
    if (newRequest.partingWidth != cached.partingWidth) {
        changes.append(ParameterChange(ParameterChangeType::Operation, "partingWidth",
                                     cached.partingWidth, newRequest.partingWidth));
    }
    
    // Check tool changes
    if (newRequest.tool != cached.tool) {
        changes.append(ParameterChange(ParameterChangeType::Tool, "tool",
                                     QVariant(), QVariant()));
    }
    
    // Check material type changes
    if (newRequest.materialType != cached.materialType) {
        changes.append(ParameterChange(ParameterChangeType::Operation, "materialType",
                                     QVariant(), QVariant()));
    }
    
    return changes;
}

void IntuiCAM::GUI::ToolpathGenerationController::regenerateContouringOperation()
{
    logMessage("Regenerating contouring operation with updated part geometry...");
    
    // This would regenerate any contouring operations that depend on part profile
    for (const auto& pair : m_toolpaths) {
        QString operationType = getOperationTypeString(pair.first);
        if (operationType == "Contouring") {
            regenerateToolpath(pair.first, operationType);
        }
    }
}

void IntuiCAM::GUI::ToolpathGenerationController::processPendingParameterChanges()
{
    if (!m_realTimeUpdatesEnabled || !m_hasCachedRequest) {
        return;
    }
    
    logMessage("Processing pending parameter changes...");
    
    // This would process any queued parameter changes
    // For now, we'll just ensure all toolpaths are up to date
    regenerateAllToolpaths();
}

void IntuiCAM::GUI::ToolpathGenerationController::updateParameter(
    ParameterChangeType changeType, 
    const QString& parameterName, 
    const QVariant& newValue,
    const QString& operationName)
{
    QMutexLocker locker(&m_parameterMutex);
    
    // Validate the parameter value first
    QString validationError = validateParameterValue(parameterName, newValue);
    if (!validationError.isEmpty()) {
        emit parameterValidated(parameterName, false, validationError);
        return;
    }
    
    // Store old value for change tracking
    QVariant oldValue;
    
    // Create parameter change record
    ParameterChange change(changeType, parameterName, oldValue, newValue);
    
    // Apply the parameter change based on type
    if (changeType == ParameterChangeType::Operation && !operationName.isEmpty()) {
        // Update operation-specific parameters
        if (parameterName.contains("facing", Qt::CaseInsensitive)) {
            // Update facing parameters
            emit parameterCacheUpdated(parameterName, newValue);
        }
        else if (parameterName.contains("roughing", Qt::CaseInsensitive)) {
            // Update roughing parameters
            emit parameterCacheUpdated(parameterName, newValue);
        }
        else if (parameterName.contains("finishing", Qt::CaseInsensitive)) {
            // Update finishing parameters
            emit parameterCacheUpdated(parameterName, newValue);
        }
        else if (parameterName.contains("parting", Qt::CaseInsensitive)) {
            // Update parting parameters
            emit parameterCacheUpdated(parameterName, newValue);
        }
    }
    
    emit parameterValidated(parameterName, true, QString());
    
    // Schedule incremental update if real-time updates are enabled
    if (m_realTimeUpdatesEnabled && !m_debounceTimer->isActive()) {
        m_debounceTimer->start();
    }
}

QString IntuiCAM::GUI::ToolpathGenerationController::validateParameterValue(
    const QString& parameterName, 
    const QVariant& value)
{
    // Basic parameter validation
    if (parameterName.contains("diameter", Qt::CaseInsensitive)) {
        bool ok;
        double diameterValue = value.toDouble(&ok);
        if (!ok || diameterValue <= 0.0) {
            return "Diameter must be a positive number";
        }
    }
    else if (parameterName.contains("feed", Qt::CaseInsensitive)) {
        bool ok;
        double feedValue = value.toDouble(&ok);
        if (!ok || feedValue <= 0.0) {
            return "Feed rate must be a positive number";
        }
    }
    else if (parameterName.contains("speed", Qt::CaseInsensitive)) {
        bool ok;
        double speedValue = value.toDouble(&ok);
        if (!ok || speedValue <= 0.0) {
            return "Speed must be a positive number";
        }
    }
    else if (parameterName.contains("tolerance", Qt::CaseInsensitive)) {
        bool ok;
        double toleranceValue = value.toDouble(&ok);
        if (!ok || toleranceValue <= 0.0) {
            return "Tolerance must be a positive number";
        }
    }
    
    // Parameter is valid
    return QString();
}

// Additional helper methods for parameter synchronization

void IntuiCAM::GUI::ToolpathGenerationController::updateParameters(const QList<ParameterChange>& changes)
{
    if (changes.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_parameterMutex);
    
    logMessage(QString("Updating %1 parameters").arg(changes.size()));
    
    // Start timing the update
    QElapsedTimer updateTimer;
    updateTimer.start();
    
    // Analyze what needs to be updated
    UpdateStrategy strategy = analyzeParameterChanges(changes);
    
    // Apply changes to cached parameters
    for (const auto& change : changes) {
        m_cachedParameters[change.parameterName] = change.newValue;
        emit parameterCacheUpdated(change.parameterName, change.newValue);
    }
    
    // Perform the incremental update
    performIncrementalUpdate(strategy);
    
    // Record timing
    qint64 duration = updateTimer.elapsed();
    QStringList affectedOps;
    affectedOps << strategy.operationsToRegenerate << strategy.visualOnlyUpdates;
    
    emit incrementalUpdateCompleted(affectedOps, static_cast<int>(duration));
    
    logMessage(QString("Parameter update completed in %1ms").arg(duration));
}



void IntuiCAM::GUI::ToolpathGenerationController::onParameterChanged(
    const QString& parameterName, 
    const QVariant& newValue, 
    const QString& operationName)
{
    // Delegate to updateParameter with appropriate change type
    ParameterChangeType changeType = ParameterChangeType::Operation;
    if (parameterName.contains("geometry", Qt::CaseInsensitive)) {
        changeType = ParameterChangeType::Geometry;
    } else if (parameterName.contains("tool", Qt::CaseInsensitive)) {
        changeType = ParameterChangeType::Tool;
    } else if (parameterName.contains("visual", Qt::CaseInsensitive)) {
        changeType = ParameterChangeType::Visual;
    }
    
    updateParameter(changeType, parameterName, newValue, operationName);
}

void IntuiCAM::GUI::ToolpathGenerationController::onParametersChanged(const QMap<QString, QVariant>& parameters)
{
    // Convert to parameter changes and apply
    QList<ParameterChange> changes;
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        ParameterChangeType changeType = ParameterChangeType::Operation; // Default
        if (it.key().contains("geometry", Qt::CaseInsensitive)) {
            changeType = ParameterChangeType::Geometry;
        } else if (it.key().contains("tool", Qt::CaseInsensitive)) {
            changeType = ParameterChangeType::Tool;
        } else if (it.key().contains("visual", Qt::CaseInsensitive)) {
            changeType = ParameterChangeType::Visual;
        }
        
        changes.append(ParameterChange(changeType, it.key(), QVariant(), it.value()));
    }
    
    updateParameters(changes);
}

void IntuiCAM::GUI::ToolpathGenerationController::connectTimelineWidget(ToolpathTimelineWidget* timelineWidget)
{
    if (!timelineWidget) {
        return;
    }
    
    // Connect signals for timeline updates with proper signature mapping
    connect(this, &ToolpathGenerationController::toolpathAdded,
            [timelineWidget](const QString& name, const QString& type, const QString& toolName) {
                timelineWidget->addToolpath(name, type, toolName, QString()); // Empty icon
            });
    
    connect(this, &ToolpathGenerationController::toolpathRemoved,
            [timelineWidget](const QString& name) {
                // Find the toolpath index by name and remove it
                for (int i = 0; i < timelineWidget->getToolpathCount(); ++i) {
                    if (timelineWidget->getToolpathName(i) == name) {
                        timelineWidget->removeToolpath(i);
                        break;
                    }
                }
            });
    
    connect(this, &ToolpathGenerationController::toolpathRegenerated,
            [timelineWidget](const QString& name, const QString& type) {
                // Find the toolpath index by name and update it
                for (int i = 0; i < timelineWidget->getToolpathCount(); ++i) {
                    if (timelineWidget->getToolpathName(i) == name) {
                        timelineWidget->updateToolpath(i, name, type, "Tool", QString()); // Generic tool name
                        break;
                    }
                }
            });
    
    logMessage("Timeline widget connected successfully");
}

std::shared_ptr<IntuiCAM::Toolpath::Tool> IntuiCAM::GUI::ToolpathGenerationController::createDefaultTool(const QString& operationType)
{
    // Create tool based on operation type
    IntuiCAM::Toolpath::Tool::Type toolType;
    
    if (operationType == "Facing") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    } else if (operationType == "Roughing") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    } else if (operationType == "Finishing") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    } else if (operationType == "Threading") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Threading;
    } else if (operationType == "Parting") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Parting;
    } else if (operationType == "Grooving") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Grooving;
    } else if (operationType == "Contouring") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    } else if (operationType == "Chamfering") {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning;
    } else {
        toolType = IntuiCAM::Toolpath::Tool::Type::Turning; // Default
    }
    
    auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(toolType, operationType.toStdString() + "_Tool");
    
    // Set default cutting parameters
    IntuiCAM::Toolpath::Tool::CuttingParameters params;
    params.feedRate = 0.2;      // mm/rev
    params.spindleSpeed = 1200; // RPM
    params.depthOfCut = 1.0;    // mm
    params.stepover = 0.5;      // mm
    
    tool->setCuttingParameters(params);
    
    return tool;
}

void IntuiCAM::GUI::ToolpathGenerationController::regenerateToolpath(const QString& operationName, const QString& operationType)
{
    logMessage(QString("Regenerating toolpath for %1 (%2)").arg(operationName).arg(operationType));
    
    try {
        // Create tool for this operation
        auto tool = createDefaultTool(operationType);
        if (!tool) {
            logMessage(QString("Failed to create tool for %1").arg(operationType));
            return;
        }
        
        // Generate and display the toolpath
        generateAndDisplayToolpath(operationName, operationType, tool);
        
    } catch (const std::exception& e) {
        logMessage(QString("Error regenerating toolpath %1: %2").arg(operationName).arg(e.what()));
    }
}

