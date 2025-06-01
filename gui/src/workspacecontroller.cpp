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
    
    // Clear stored workpiece
    m_currentWorkpiece = TopoDS_Shape();
    
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
    
    // Clear stored workpiece
    m_currentWorkpiece = TopoDS_Shape();
    
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
    
    // Connect chuck centerline detection
    connect(m_chuckManager, &ChuckManager::chuckCenterlineDetected,
            this, &WorkspaceController::handleChuckCenterlineDetected);
    
    // Connect multiple cylinders detection for manual selection
    connect(m_workpieceManager, &WorkpieceManager::multipleCylindersDetected,
            this, &WorkspaceController::handleMultipleCylindersDetected);
    
    // Connect manual cylinder axis selection
    connect(m_workpieceManager, &WorkpieceManager::cylinderAxisSelected,
            this, &WorkspaceController::handleCylinderAxisSelected);
    
    qDebug() << "WorkspaceController: Manager signal connections established";
}

void WorkspaceController::executeWorkpieceWorkflow(const TopoDS_Shape& workpiece)
{
    // Store the original workpiece shape for later re-processing
    m_currentWorkpiece = workpiece;
    
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
    gp_Ax1 mainAxis = m_workpieceManager->getMainCylinderAxis();
    double detectedDiameter = m_workpieceManager->getDetectedDiameter();
    
    if (detectedDiameter <= 0.0) {
        qDebug() << "WorkspaceController: Invalid diameter detected";
        return;
    }
    
    // Step 4: Align workpiece with chuck centerline if chuck is loaded
    gp_Ax1 alignmentAxis = mainAxis;
    if (m_chuckManager->hasValidCenterline()) {
        alignmentAxis = alignWorkpieceWithChuckCenterline(mainAxis);
        qDebug() << "WorkspaceController: Workpiece aligned with chuck centerline";
    }
    
    // Step 5: Calculate optimal raw material size
    double rawMaterialDiameter = m_rawMaterialManager->getNextStandardDiameter(detectedDiameter);
    
    // Step 6: Create and display raw material that encompasses the workpiece
    m_rawMaterialManager->displayRawMaterialForWorkpiece(rawMaterialDiameter, workpiece, alignmentAxis);
    
    // Step 7: Emit workflow completion signal
    emit workpieceWorkflowCompleted(detectedDiameter, rawMaterialDiameter);
    
    qDebug() << "WorkspaceController: Workpiece workflow completed successfully"
             << "- Detected diameter:" << detectedDiameter << "mm"
             << "- Raw material diameter:" << rawMaterialDiameter << "mm";
}

gp_Ax1 WorkspaceController::alignWorkpieceWithChuckCenterline(const gp_Ax1& workpieceAxis)
{
    if (!m_chuckManager->hasValidCenterline()) {
        qDebug() << "WorkspaceController: No valid chuck centerline for alignment";
        return workpieceAxis;
    }
    
    try {
        gp_Ax1 chuckCenterline = m_chuckManager->getChuckCenterlineAxis();
        
        // For now, we align the workpiece axis direction with the chuck centerline direction
        // while preserving the workpiece axis location
        gp_Ax1 alignedAxis(workpieceAxis.Location(), chuckCenterline.Direction());
        
        qDebug() << "WorkspaceController: Workpiece axis aligned with chuck centerline";
        return alignedAxis;
        
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Error aligning workpiece with chuck centerline:" << e.what();
        return workpieceAxis;
    }
}

bool WorkspaceController::selectWorkpieceCylinderAxis(int cylinderIndex)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    bool success = m_workpieceManager->selectCylinderAxis(cylinderIndex);
    if (success) {
        // Re-execute workflow with the newly selected axis
        CylinderInfo selectedCylinder = m_workpieceManager->getCylinderInfo(cylinderIndex);
        
        // Get current workpiece (assuming single workpiece for now)
        QVector<Handle(AIS_Shape)> workpieces = m_workpieceManager->getWorkpieces();
        if (!workpieces.isEmpty()) {
            // Clear raw material and regenerate with new axis
            m_rawMaterialManager->clearRawMaterial();
            
            gp_Ax1 alignmentAxis = selectedCylinder.axis;
            if (m_chuckManager->hasValidCenterline()) {
                alignmentAxis = alignWorkpieceWithChuckCenterline(selectedCylinder.axis);
            }
            
            double rawMaterialDiameter = m_rawMaterialManager->getNextStandardDiameter(selectedCylinder.diameter);
            
            // We need the workpiece shape, but AIS_Shape doesn't directly give us TopoDS_Shape
            // For now, we'll use the axis and diameter information
            // TODO: Store original workpiece shapes for re-processing
            
            emit workpieceWorkflowCompleted(selectedCylinder.diameter, rawMaterialDiameter);
            qDebug() << "WorkspaceController: Workpiece workflow updated with selected cylinder" << cylinderIndex;
        }
    }
    
    return success;
}

QVector<CylinderInfo> WorkspaceController::getDetectedCylinders() const
{
    if (m_workpieceManager) {
        return m_workpieceManager->getDetectedCylindersInfo();
    }
    return QVector<CylinderInfo>();
}

int WorkspaceController::getSelectedCylinderIndex() const
{
    if (m_workpieceManager) {
        return m_workpieceManager->getSelectedCylinderIndex();
    }
    return -1;
}

bool WorkspaceController::hasChuckCenterline() const
{
    return m_chuckManager && m_chuckManager->hasValidCenterline();
}

gp_Ax1 WorkspaceController::getChuckCenterlineAxis() const
{
    if (m_chuckManager && m_chuckManager->hasValidCenterline()) {
        return m_chuckManager->getChuckCenterlineAxis();
    }
    return gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Default axis
}

void WorkspaceController::handleChuckCenterlineDetected(const gp_Ax1& axis)
{
    emit chuckCenterlineDetected(axis);
    qDebug() << "WorkspaceController: Chuck centerline detected and forwarded to UI";
}

void WorkspaceController::handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders)
{
    emit multipleCylindersDetected(cylinders);
    qDebug() << "WorkspaceController: Multiple cylinders detected (" << cylinders.size() << "), manual selection available";
}

void WorkspaceController::handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo)
{
    emit cylinderAxisSelected(index, cylinderInfo);
    qDebug() << "WorkspaceController: Cylinder axis" << index << "selected:" << cylinderInfo.description;
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

bool WorkspaceController::updateRawMaterialDiameter(double diameter)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    if (diameter <= 0.0) {
        emit errorOccurred("WorkspaceController", "Invalid diameter specified");
        return false;
    }

    // Get current workpiece and axis information
    QVector<Handle(AIS_Shape)> workpieces = m_workpieceManager->getWorkpieces();
    if (workpieces.isEmpty()) {
        emit errorOccurred("WorkspaceController", "No workpiece loaded");
        return false;
    }

    try {
        // Get current axis from workpiece manager
        gp_Ax1 currentAxis = m_workpieceManager->getMainCylinderAxis();
        
        // Store current workpiece shape (we need to get it from the original shape)
        // For now, we'll use the current axis and regenerate the raw material
        
        // Clear existing raw material
        m_rawMaterialManager->clearRawMaterial();
        
        // Apply chuck alignment if available
        gp_Ax1 alignmentAxis = currentAxis;
        if (m_chuckManager->hasValidCenterline()) {
            alignmentAxis = alignWorkpieceWithChuckCenterline(currentAxis);
        }
        
        // Use the stored workpiece shape for proper re-processing
        if (!m_currentWorkpiece.IsNull()) {
            m_rawMaterialManager->displayRawMaterialForWorkpiece(diameter, m_currentWorkpiece, alignmentAxis);
        } else {
            // Fallback: create with estimated length if no workpiece stored
            double estimatedLength = 100.0; // Default length - should be calculated properly
            m_rawMaterialManager->displayRawMaterial(diameter, estimatedLength, alignmentAxis);
        }
        
        qDebug() << "WorkspaceController: Raw material diameter updated to" << diameter << "mm";
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to update raw material diameter: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::updateDistanceToChuck(double distance)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    if (!m_chuckManager->hasValidCenterline()) {
        qDebug() << "WorkspaceController: No chuck centerline available for distance adjustment";
        // This is not an error - just means chuck positioning isn't applicable
        return true;
    }

    try {
        // Use WorkpieceManager's positioning capabilities
        bool success = m_workpieceManager->positionWorkpieceAlongAxis(distance);
        
        if (success) {
            // Regenerate raw material to match the new workpiece position
            if (!m_currentWorkpiece.IsNull()) {
                // Get current settings
                gp_Ax1 currentAxis = m_workpieceManager->getMainCylinderAxis();
                double currentDiameter = m_rawMaterialManager->getCurrentDiameter();
                
                // Get the current transformation from the workpiece manager
                gp_Trsf currentTransform = m_workpieceManager->getCurrentTransformation();
                
                // Apply chuck alignment if available
                gp_Ax1 alignmentAxis = currentAxis;
                if (m_chuckManager->hasValidCenterline()) {
                    alignmentAxis = alignWorkpieceWithChuckCenterline(currentAxis);
                }
                
                // Clear and regenerate raw material using transform-aware method
                m_rawMaterialManager->clearRawMaterial();
                m_rawMaterialManager->displayRawMaterialForWorkpieceWithTransform(
                    currentDiameter, m_currentWorkpiece, alignmentAxis, currentTransform);
            }
            
            qDebug() << "WorkspaceController: Distance to chuck set to" << distance << "mm and raw material updated with correct transform";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to update distance to chuck: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::flipWorkpieceOrientation(bool flipped)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    QVector<Handle(AIS_Shape)> workpieces = m_workpieceManager->getWorkpieces();
    if (workpieces.isEmpty()) {
        emit errorOccurred("WorkspaceController", "No workpiece loaded");
        return false;
    }

    try {
        // Use WorkpieceManager's transformation capabilities
        bool success = m_workpieceManager->flipWorkpieceOrientation(flipped);
        
        if (success) {
            // Regenerate raw material to match the new workpiece orientation using transform
            if (!m_currentWorkpiece.IsNull()) {
                // Get current settings
                gp_Ax1 currentAxis = m_workpieceManager->getMainCylinderAxis();
                double currentDiameter = m_rawMaterialManager->getCurrentDiameter();
                
                // Get the current transformation from the workpiece manager
                gp_Trsf currentTransform = m_workpieceManager->getCurrentTransformation();
                
                // Apply chuck alignment if available
                gp_Ax1 alignmentAxis = currentAxis;
                if (m_chuckManager->hasValidCenterline()) {
                    alignmentAxis = alignWorkpieceWithChuckCenterline(currentAxis);
                }
                
                // Clear and regenerate raw material using transform-aware method
                m_rawMaterialManager->clearRawMaterial();
                m_rawMaterialManager->displayRawMaterialForWorkpieceWithTransform(
                    currentDiameter, m_currentWorkpiece, alignmentAxis, currentTransform);
            }
            
            qDebug() << "WorkspaceController: Workpiece orientation" << (flipped ? "flipped" : "restored") << "and raw material updated with correct transform";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to flip workpiece orientation: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::applyPartLoadingSettings(double distance, double diameter, bool flipped, int cylinderIndex)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    bool allSuccessful = true;

    try {
        // Apply cylinder selection first if changed
        if (cylinderIndex >= 0) {
            bool cylinderSuccess = selectWorkpieceCylinderAxis(cylinderIndex);
            if (!cylinderSuccess) {
                qDebug() << "WorkspaceController: Failed to apply cylinder selection, continuing with other settings";
                allSuccessful = false;
            }
        }

        // Apply diameter change
        bool diameterSuccess = updateRawMaterialDiameter(diameter);
        if (!diameterSuccess) {
            qDebug() << "WorkspaceController: Failed to apply diameter change";
            allSuccessful = false;
        }

        // Apply distance change
        bool distanceSuccess = updateDistanceToChuck(distance);
        if (!distanceSuccess) {
            qDebug() << "WorkspaceController: Failed to apply distance change";
            allSuccessful = false;
        }

        // Apply orientation flip
        bool orientationSuccess = flipWorkpieceOrientation(flipped);
        if (!orientationSuccess) {
            qDebug() << "WorkspaceController: Failed to apply orientation flip";
            allSuccessful = false;
        }

        if (allSuccessful) {
            qDebug() << "WorkspaceController: All part loading settings applied successfully";
        } else {
            qDebug() << "WorkspaceController: Some part loading settings failed to apply";
        }

        return allSuccessful;

    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to apply part loading settings: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::reprocessCurrentWorkpiece()
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    if (m_currentWorkpiece.IsNull()) {
        emit errorOccurred("WorkspaceController", "No workpiece available for reprocessing");
        return false;
    }

    try {
        qDebug() << "WorkspaceController: Starting workpiece reprocessing";
        
        // Clear existing workpieces and raw material while preserving chuck
        clearWorkpieces();
        
        // Reprocess the stored workpiece through the complete workflow
        executeWorkpieceWorkflow(m_currentWorkpiece);
        
        qDebug() << "WorkspaceController: Workpiece reprocessing completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to reprocess workpiece: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
} 