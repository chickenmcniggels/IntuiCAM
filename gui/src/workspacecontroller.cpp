#include "workspacecontroller.h"
#include "chuckmanager.h"
#include "workpiecemanager.h"
#include "rawmaterialmanager.h"
#include "isteploader.h"

#include <QDebug>

WorkspaceController::WorkspaceController(QObject *parent)
    : QObject(parent)
    , m_chuckManager(nullptr)
    , m_workpieceManager(nullptr)
    , m_rawMaterialManager(nullptr)
    , m_stepLoader(nullptr)
    , m_initialized(false)
{
    // Create component managers
    m_chuckManager = new ChuckManager(this);
    m_workpieceManager = new WorkpieceManager(this);
    m_rawMaterialManager = new RawMaterialManager(this);
    
    // Set up signal connections
    setupManagerConnections();
    
    qDebug() << "WorkspaceController created with all component managers";
}

WorkspaceController::~WorkspaceController()
{
    // Qt handles cleanup of child objects automatically
}

void WorkspaceController::initialize(Handle(AIS_InteractiveContext) context, IStepLoader* stepLoader)
{
    if (context.IsNull() || !stepLoader) {
        qDebug() << "WorkspaceController: Invalid context or stepLoader provided";
        emit errorOccurred("WorkspaceController", "Invalid initialization parameters");
        return;
    }
    
    m_context = context;
    m_stepLoader = stepLoader;
    
    // Initialize all component managers
    m_chuckManager->initialize(context, stepLoader);
    m_workpieceManager->initialize(context);
    m_rawMaterialManager->initialize(context);
    
    m_initialized = true;
    qDebug() << "WorkspaceController initialized successfully";
}

bool WorkspaceController::initializeChuck(const QString& chuckFilePath)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    qDebug() << "WorkspaceController: Initializing chuck from" << chuckFilePath;
    
    bool success = m_chuckManager->loadChuck(chuckFilePath);
    if (success) {
        emit chuckInitialized();
        qDebug() << "WorkspaceController: Chuck initialization completed successfully";
    } else {
        qDebug() << "WorkspaceController: Chuck initialization failed";
    }
    
    return success;
}

bool WorkspaceController::addWorkpiece(const TopoDS_Shape& workpiece)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    if (workpiece.IsNull()) {
        emit errorOccurred("WorkspaceController", "Invalid workpiece shape provided");
        return false;
    }
    
    qDebug() << "WorkspaceController: Processing workpiece workflow";
    
    try {
        executeWorkpieceWorkflow(workpiece);
        return true;
    } catch (const std::exception& e) {
        QString errorMsg = QString("Workpiece workflow failed: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

void WorkspaceController::clearWorkpieces()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "WorkspaceController: Clearing workpieces";
    
    m_workpieceManager->clearWorkpieces();
    m_rawMaterialManager->clearRawMaterial();
    
    qDebug() << "WorkspaceController: Workpieces cleared";
}

void WorkspaceController::clearWorkspace()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "WorkspaceController: Clearing entire workspace";
    
    m_chuckManager->clearChuck();
    m_workpieceManager->clearWorkpieces();
    m_rawMaterialManager->clearRawMaterial();
    
    emit workspaceCleared();
    qDebug() << "WorkspaceController: Workspace cleared completely";
}

bool WorkspaceController::isInitialized() const
{
    return m_initialized;
}

bool WorkspaceController::isChuckLoaded() const
{
    return m_initialized && m_chuckManager && m_chuckManager->isChuckLoaded();
}

void WorkspaceController::setupManagerConnections()
{
    // Connect error signals from all managers
    connect(m_chuckManager, &ChuckManager::errorOccurred,
            this, &WorkspaceController::handleChuckError);
    
    connect(m_workpieceManager, &WorkpieceManager::errorOccurred,
            this, &WorkspaceController::handleWorkpieceError);
    
    connect(m_rawMaterialManager, &RawMaterialManager::errorOccurred,
            this, &WorkspaceController::handleRawMaterialError);
    
    // Connect cylinder detection for automated workflow
    connect(m_workpieceManager, &WorkpieceManager::cylinderDetected,
            this, &WorkspaceController::handleCylinderDetected);
    
    qDebug() << "WorkspaceController: Manager signal connections established";
}

void WorkspaceController::executeWorkpieceWorkflow(const TopoDS_Shape& workpiece)
{
    // Step 1: Add workpiece to scene
    bool workpieceAdded = m_workpieceManager->addWorkpiece(workpiece);
    if (!workpieceAdded) {
        emit errorOccurred("WorkspaceController", "Failed to add workpiece to scene");
        return;
    }
    
    // Step 2: Analyze workpiece geometry for cylinders
    QVector<gp_Ax1> cylinders = m_workpieceManager->detectCylinders(workpiece);
    if (cylinders.isEmpty()) {
        qDebug() << "WorkspaceController: No suitable cylinders detected in workpiece";
        // Still consider this a success - workpiece is displayed
        return;
    }
    
    // Step 3: Get the main cylinder information
    gp_Ax1 mainAxis = cylinders.first();
    double detectedDiameter = m_workpieceManager->getDetectedDiameter();
    
    if (detectedDiameter <= 0.0) {
        qDebug() << "WorkspaceController: Invalid diameter detected";
        return;
    }
    
    // Step 4: Calculate optimal raw material size
    double rawMaterialDiameter = m_rawMaterialManager->getNextStandardDiameter(detectedDiameter);
    
    // Step 5: Create and display raw material that encompasses the workpiece
    m_rawMaterialManager->displayRawMaterialForWorkpiece(rawMaterialDiameter, workpiece, mainAxis);
    
    // Step 6: Emit workflow completion signal
    emit workpieceWorkflowCompleted(detectedDiameter, rawMaterialDiameter);
    
    qDebug() << "WorkspaceController: Workpiece workflow completed successfully"
             << "- Detected diameter:" << detectedDiameter << "mm"
             << "- Raw material diameter:" << rawMaterialDiameter << "mm";
}

void WorkspaceController::handleChuckError(const QString& message)
{
    emit errorOccurred("ChuckManager", message);
}

void WorkspaceController::handleWorkpieceError(const QString& message)
{
    emit errorOccurred("WorkpieceManager", message);
}

void WorkspaceController::handleRawMaterialError(const QString& message)
{
    emit errorOccurred("RawMaterialManager", message);
}

void WorkspaceController::handleCylinderDetected(double diameter, double length, const gp_Ax1& axis)
{
    Q_UNUSED(axis) // Axis information is handled in the main workflow
    qDebug() << "WorkspaceController: Cylinder detected - diameter:" << diameter 
             << "mm, estimated length:" << length << "mm";
} 