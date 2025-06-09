#include "workspacecontroller.h"
#include "chuckmanager.h"
#include "workpiecemanager.h"
#include "rawmaterialmanager.h"
#include "isteploader.h"

#include <QDebug>
#include <cmath>

// OpenCASCADE includes
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Circ.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <Precision.hxx>
#include <AIS_Shape.hxx>

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
        // Recalculate raw material with the newly selected axis
        bool rawMaterialSuccess = recalculateRawMaterial();
        
        if (rawMaterialSuccess) {
            // Get selected cylinder info for workflow completion signal
            CylinderInfo selectedCylinder = m_workpieceManager->getCylinderInfo(cylinderIndex);
            double rawMaterialDiameter = m_rawMaterialManager->getCurrentDiameter();
            
            emit workpieceWorkflowCompleted(selectedCylinder.diameter, rawMaterialDiameter);
            qDebug() << "WorkspaceController: Cylinder axis" << cylinderIndex << "selected and raw material recalculated";
        } else {
            qDebug() << "WorkspaceController: Cylinder axis selected but raw material recalculation failed";
        }
        
        return rawMaterialSuccess;
    }
    
    return false;
}

QVector<CylinderInfo> WorkspaceController::getDetectedCylinders() const
{
    if (m_workpieceManager) {
        return m_workpieceManager->getDetectedCylindersInfo();
    }
    return QVector<CylinderInfo>();
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
    qDebug() << "WorkspaceController: updateRawMaterialDiameter called with diameter:" << diameter << "mm";
    
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
        // Use centralized recalculation method
        bool success = recalculateRawMaterial(diameter);
        
        if (success) {
            qDebug() << "WorkspaceController: Raw material diameter successfully updated to" << diameter << "mm";
        } else {
            qDebug() << "WorkspaceController: Failed to update raw material diameter to" << diameter << "mm";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to update raw material diameter: %1").arg(e.what());
        qDebug() << errorMsg;
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::updateDistanceToChuck(double distance)
{
    if (!m_initialized || !m_workpieceManager) {
        emit errorOccurred("WorkspaceController", "Cannot update chuck distance - workspace not initialized");
        return false;
    }

    qDebug() << "WorkspaceController: Updating distance to chuck:" << distance << "mm";

    try {
        // Update position in workpiece manager
        bool success = m_workpieceManager->positionWorkpieceAlongAxis(distance);
        
        if (success) {
            qDebug() << "WorkspaceController: Workpiece positioned at" << distance << "mm from chuck";
            
            // If raw material is loaded, update it to match workpiece position
            if (m_rawMaterialManager && m_rawMaterialManager->isRawMaterialDisplayed()) {
                recalculateRawMaterial(-1.0); // Use current diameter
                qDebug() << "WorkspaceController: Recalculated raw material for new position";
            }
            
            // Make sure toolpaths are properly transformed
            qDebug() << "WorkspaceController: Emitting workpiecePositionChanged signal for toolpath updates";
            emit workpiecePositionChanged(distance);
            
            return true;
        } else {
            emit errorOccurred("WorkspaceController", "Failed to position workpiece");
            return false;
        }
    } catch (const std::exception& e) {
        emit errorOccurred("WorkspaceController", QString("Exception while updating chuck distance: %1").arg(e.what()));
        return false;
    }
}

bool WorkspaceController::flipWorkpieceOrientation(bool flipped)
{
    qDebug() << "WorkspaceController: flipWorkpieceOrientation called with flipped:" << flipped;
    
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
        bool flipSuccess = m_workpieceManager->flipWorkpieceOrientation(flipped);
        
        if (flipSuccess) {
            qDebug() << "WorkspaceController: Workpiece orientation" << (flipped ? "flipped" : "restored") << "successfully, now recalculating raw material";
            // Recalculate raw material to match the new workpiece orientation
            bool rawMaterialSuccess = recalculateRawMaterial();
            
            if (rawMaterialSuccess) {
                qDebug() << "WorkspaceController: Workpiece orientation" << (flipped ? "flipped" : "restored") << "and raw material updated successfully";
            } else {
                qDebug() << "WorkspaceController: Orientation updated but raw material recalculation failed";
            }
            
            return rawMaterialSuccess;
        } else {
            qDebug() << "WorkspaceController: Failed to" << (flipped ? "flip" : "restore") << "workpiece orientation";
        }
        
        return false;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to flip workpiece orientation: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::applyPartLoadingSettings(double distance, double diameter, bool flipped)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    bool success = true;
    
    // Apply orientation flip
    success &= flipWorkpieceOrientation(flipped);
    
    // Apply distance positioning
    success &= updateDistanceToChuck(distance);
    
    // Apply raw material diameter
    success &= updateRawMaterialDiameter(diameter);
    
    return success;
}

bool WorkspaceController::processManualAxisSelection(const TopoDS_Shape& selectedShape, const gp_Pnt& clickPoint)
{
    if (!m_initialized || selectedShape.IsNull()) {
        emit errorOccurred("WorkspaceController", "Invalid selection for axis extraction");
        return false;
    }

    try {
        gp_Ax1 extractedAxis;
        double extractedDiameter = 0.0;
        bool axisFound = false;

        // Try to extract cylindrical axis from the selected shape
        if (selectedShape.ShapeType() == TopAbs_FACE) {
            // Analyze cylindrical face
            TopoDS_Face face = TopoDS::Face(selectedShape);
            BRepAdaptor_Surface surface(face);
            
            if (surface.GetType() == GeomAbs_Cylinder) {
                gp_Cylinder cylinder = surface.Cylinder();
                extractedAxis = cylinder.Axis();
                extractedDiameter = cylinder.Radius() * 2.0;
                axisFound = true;
                qDebug() << "WorkspaceController: Extracted axis from cylindrical face - Diameter:" << extractedDiameter << "mm";
            }
        } else if (selectedShape.ShapeType() == TopAbs_EDGE) {
            // Analyze circular edge
            TopoDS_Edge edge = TopoDS::Edge(selectedShape);
            BRepAdaptor_Curve curve(edge);
            
            if (curve.GetType() == GeomAbs_Circle) {
                gp_Circ circle = curve.Circle();
                extractedAxis = circle.Axis();
                extractedDiameter = circle.Radius() * 2.0;
                axisFound = true;
                qDebug() << "WorkspaceController: Extracted axis from circular edge - Diameter:" << extractedDiameter << "mm";
            }
        }

        if (!axisFound) {
            emit errorOccurred("WorkspaceController", "Selected geometry is not cylindrical or circular. Please select a cylindrical face or circular edge.");
            return false;
        }

        // Create transformation to align the extracted axis with the Z-axis
        gp_Trsf alignmentTransform = createAxisAlignmentTransformation(extractedAxis);
        
        // Apply the transformation through WorkpieceManager for robust handling
        if (!m_workpieceManager->setAxisAlignmentTransformation(alignmentTransform)) {
            emit errorOccurred("WorkspaceController", "Failed to apply axis alignment transformation");
            return false;
        }

        // Update the workpiece manager with the new custom axis (now aligned with Z)
        gp_Ax1 alignedAxis(extractedAxis.Location(), gp_Dir(0, 0, 1));
        m_workpieceManager->setCustomAxis(alignedAxis, extractedDiameter);

        // Recalculate raw material with the new alignment
        bool rawMaterialSuccess = recalculateRawMaterial();
        
        if (rawMaterialSuccess) {
            // Create CylinderInfo for the manually selected axis
            CylinderInfo manualAxisInfo(alignedAxis, extractedDiameter, 100.0, "Manual Selection");
            
            // Emit signals for UI updates
            emit manualAxisSelected(extractedDiameter, alignedAxis);
            emit cylinderAxisSelected(-1, manualAxisInfo); // Use -1 to indicate manual selection
            emit workpieceWorkflowCompleted(extractedDiameter, m_rawMaterialManager->getCurrentDiameter());
            
            qDebug() << "WorkspaceController: Manual axis selection completed successfully";
            return true;
        } else {
            qDebug() << "WorkspaceController: Manual axis selection succeeded but raw material recalculation failed";
            return false;
        }

    } catch (const std::exception& e) {
        QString errorMsg = QString("Error processing manual axis selection: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

gp_Trsf WorkspaceController::createAxisAlignmentTransformation(const gp_Ax1& sourceAxis)
{
    gp_Trsf transform;
    
    try {
        // Target axis is Z-axis through origin
        gp_Ax1 targetAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        
        // Get the direction vectors
        gp_Dir sourceDir = sourceAxis.Direction();
        gp_Dir targetDir = targetAxis.Direction();
        
        // Check if axes are already aligned
        if (sourceDir.IsEqual(targetDir, Precision::Angular())) {
            // Only need translation to move to origin
            gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
            if (translation.Magnitude() > Precision::Confusion()) {
                transform.SetTranslation(translation);
            }
            return transform;
        }
        
        // Check if axes are opposite
        if (sourceDir.IsOpposite(targetDir, Precision::Angular())) {
            // Rotate 180 degrees around Y-axis and translate
            gp_Ax1 rotationAxis(sourceAxis.Location(), gp_Dir(0, 1, 0));
            transform.SetRotation(rotationAxis, M_PI);
            
            // Then translate to origin
            gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
            if (translation.Magnitude() > Precision::Confusion()) {
                gp_Trsf translationTransform;
                translationTransform.SetTranslation(translation);
                transform = translationTransform * transform;
            }
            return transform;
        }
        
        // Calculate rotation axis as cross product of source and target directions
        gp_Vec sourceVec(sourceDir);
        gp_Vec targetVec(targetDir);
        gp_Vec rotationVec = sourceVec.Crossed(targetVec);
        
        if (rotationVec.Magnitude() < Precision::Confusion()) {
            // Vectors are parallel, no rotation needed, just translation
            gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
            if (translation.Magnitude() > Precision::Confusion()) {
                transform.SetTranslation(translation);
            }
            return transform;
        }
        
        // Normalize rotation vector and calculate angle
        gp_Dir rotationDir(rotationVec);
        double angle = sourceVec.Angle(targetVec);
        
        // Create rotation around the calculated axis through the source axis location
        gp_Ax1 rotationAxis(sourceAxis.Location(), rotationDir);
        transform.SetRotation(rotationAxis, angle);
        
        // Apply translation to move to target location
        gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
        if (translation.Magnitude() > Precision::Confusion()) {
            gp_Trsf translationTransform;
            translationTransform.SetTranslation(translation);
            transform = translationTransform * transform;
        }
        
        qDebug() << "WorkspaceController: Created axis alignment transformation - Rotation angle:" 
                 << (angle * 180.0 / M_PI) << "degrees";
        
        return transform;
        
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Error creating axis alignment transformation:" << e.what();
        return gp_Trsf(); // Return identity transformation
    }
}

bool WorkspaceController::reprocessCurrentWorkpiece()
{
    if (!m_initialized || m_currentWorkpiece.IsNull()) {
        emit errorOccurred("WorkspaceController", "No workpiece available for reprocessing");
        return false;
    }

    try {
        // Clear current workpieces and raw material
        m_workpieceManager->clearWorkpieces();
        m_rawMaterialManager->clearRawMaterial();
        
        // Re-execute the complete workflow
        executeWorkpieceWorkflow(m_currentWorkpiece);
        
        qDebug() << "WorkspaceController: Workpiece reprocessed successfully";
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to reprocess workpiece: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::recalculateRawMaterial(double diameter)
{
    if (!m_initialized || m_currentWorkpiece.IsNull()) {
        qDebug() << "WorkspaceController: Cannot recalculate raw material - not initialized or no workpiece";
        return false;
    }

    try {
        // Get current settings - use Z-aligned axis if axis alignment is active
        gp_Ax1 currentAxis;
        if (m_workpieceManager->hasAxisAlignmentTransformation()) {
            // After manual axis selection, always use Z-axis for raw material
            currentAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
            qDebug() << "WorkspaceController: Using Z-aligned axis for raw material (manual selection active)";
        } else {
            // Use the detected/selected axis from workpiece manager
            currentAxis = m_workpieceManager->getMainCylinderAxis();
            qDebug() << "WorkspaceController: Using workpiece manager axis for raw material";
        }
        
        double currentDiameter = (diameter > 0.0) ? diameter : m_rawMaterialManager->getCurrentDiameter();
        
        // Ensure we have a valid diameter
        if (currentDiameter <= 0.0) {
            currentDiameter = m_rawMaterialManager->getNextStandardDiameter(
                m_workpieceManager->getDetectedDiameter());
        }
        
        // Get the current transformation from the workpiece manager
        gp_Trsf currentTransform = m_workpieceManager->getCurrentTransformation();
        
        // Debug transformation details for troubleshooting
        gp_XYZ translation = currentTransform.TranslationPart();
        qDebug() << "WorkspaceController: Complete transformation - Translation:" 
                 << translation.X() << "," << translation.Y() << "," << translation.Z();
        qDebug() << "WorkspaceController: Axis alignment active:" << m_workpieceManager->hasAxisAlignmentTransformation();
        qDebug() << "WorkspaceController: Workpiece flipped:" << m_workpieceManager->isWorkpieceFlipped();
        qDebug() << "WorkspaceController: Position offset:" << m_workpieceManager->getWorkpiecePositionOffset() << "mm";
        
        // Apply chuck alignment if available
        gp_Ax1 alignmentAxis = currentAxis;
        if (m_chuckManager->hasValidCenterline()) {
            alignmentAxis = alignWorkpieceWithChuckCenterline(currentAxis);
        }
        
        // Clear and regenerate raw material using transform-aware method
        m_rawMaterialManager->clearRawMaterial();
        qDebug() << "WorkspaceController: Recalculating raw material with diameter:" << currentDiameter << "mm";
        m_rawMaterialManager->displayRawMaterialForWorkpieceWithTransform(
            currentDiameter, m_currentWorkpiece, alignmentAxis, currentTransform);
        
        // The raw material manager already calls UpdateCurrentViewer, but ensure it's called
        if (!m_context.IsNull()) {
            m_context->UpdateCurrentViewer();
        }
        
        qDebug() << "WorkspaceController: Raw material recalculated successfully"
                 << "- Diameter:" << currentDiameter << "mm";
        
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to recalculate raw material: %1").arg(e.what());
        qDebug() << errorMsg;
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::hasPartShape() const
{
    // Check if the workpiece manager exists and has a part
    return m_workpieceManager && m_workpieceManager->hasWorkpiece();
}

TopoDS_Shape WorkspaceController::getPartShape() const
{
    // If we have a workpiece manager and it has a part, return it
    if (m_workpieceManager && m_workpieceManager->hasWorkpiece()) {
        return m_workpieceManager->getWorkpieceShape();
    }
    
    // Otherwise return a null shape
    return TopoDS_Shape();
}

 